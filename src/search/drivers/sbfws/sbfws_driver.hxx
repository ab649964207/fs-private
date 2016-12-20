
#pragma once

#include <memory>

#include <search/drivers/sbfws/iw_run.hxx>
#include <search/drivers/registry.hxx>
#include <search/drivers/smart_effect_driver.hxx>
#include <search/nodes/bfws_node.hxx>
#include <search/components/unsat_goals_novelty.hxx>
#include <search/algorithms/aptk/events.hxx>
#include <search/algorithms/aptk/best_first_search.hxx>
#include <heuristics/relaxed_plan/smart_rpg.hxx>
#include <aptk2/search/components/stl_unsorted_fifo_open_list.hxx>
#include <search/drivers/bfws/iw_novelty_evaluator.hxx>


namespace fs0 { namespace bfws {

class BFWSStats {
public:
	BFWSStats() : _expanded(0), _generated(0), _evaluated(0) {}
	
	void expansion() { ++_expanded; }
	void generation() { ++_generated; }
	void evaluation() { ++_evaluated; }
	void simulation() { ++_simulations; }

	unsigned long expanded() const { return _expanded; }
	unsigned long generated() const { return _generated; }
	unsigned long evaluated() const { return _evaluated; }
	unsigned int simulated() const { return _simulations; }
	
	void set_initial_reachable_subgoals(unsigned num) { _initial_reachable_subgoals = num; }
	void set_initial_relevant_atoms(unsigned num) { _initial_relevant_atoms = num; }
	
	using DataPointT = std::tuple<std::string, std::string, std::string>;
	std::vector<DataPointT> dump() const {
		return {
			std::make_tuple("expanded", "Expansions", std::to_string(expanded())),
			std::make_tuple("generated", "Generations", std::to_string(generated())),
			std::make_tuple("evaluated", "Evaluations", std::to_string(evaluated())),
			
			std::make_tuple("simulations", "Simulations", std::to_string(simulated())),
			std::make_tuple("reachable_0", "Subreachable goals in initial state", std::to_string(_initial_reachable_subgoals)),
			std::make_tuple("relevant_atoms_0", "|R|_0", std::to_string(_initial_relevant_atoms)),
		};
	}
	
protected:
	unsigned long _expanded;
	unsigned long _generated;
	unsigned long _evaluated;
	unsigned long _simulations;
	unsigned int _initial_reachable_subgoals; // The number of subgoals that are reachable on the initial simulation
	unsigned int _initial_relevant_atoms; // The size of |R| on the initial state
};

//! The node type we'll use for the Simulated BFWS search, parametrized by type of state and action action
template <typename StateT, typename ActionT>
class SBFWSNode {
public:
	using ptr_t = std::shared_ptr<SBFWSNode<StateT, ActionT>>;
	using action_t = typename ActionT::IdType;

	//! The state corresponding to the search node
	StateT state;

	//! The action that led to the state in this search node
	action_t action;

	//! The parent search node
	ptr_t parent;

	//! Accummulated cost
	unsigned g;

	//! The (cached) feature valuation corresponding to the state in this node
	FeatureValuation feature_valuation;

	//! The novelty "type", i.e. the values wrt which the novelty is computed, e.g. <#g, #r>
	unsigned _type;

	//! The novelty of the state
	unsigned novelty;

	//! The number of unachieved goals (#g)
	unsigned unachieved;

	//! The number of atoms in the last relaxed plan computed in the way to the current state that have been
	//! made true along the path (#r)
	RelevantAtomSet _relevant_atoms;

public:
	SBFWSNode() = delete;
	~SBFWSNode() = default;

	SBFWSNode(const SBFWSNode& other) = delete;
	SBFWSNode(SBFWSNode&& other) = delete;
	SBFWSNode& operator=(const SBFWSNode& rhs) = delete;
	SBFWSNode& operator=(SBFWSNode&& rhs) = delete;

	//! Constructor with full copying of the state (expensive)
	SBFWSNode(const StateT& s) : SBFWSNode(StateT(s), ActionT::invalid_action_id, nullptr) {}

	//! Constructor with move of the state (cheaper)
	SBFWSNode(StateT&& _state, action_t action_, ptr_t parent_) :
		state(std::move(_state)), action(action_), parent(parent_), g(parent ? parent->g+1 : 0),
		novelty(std::numeric_limits<unsigned>::max()),
		unachieved(std::numeric_limits<unsigned>::max()),
		_relevant_atoms(nullptr)
	{}

	//! The novelty type (for the IWRun node, will always be 0)
	unsigned type() const { return _type; }

	bool has_parent() const { return parent != nullptr; }

	//! Required for the interface of some algorithms that might prioritise helpful actions.
	bool is_helpful() const { return false; }

	bool operator==( const SBFWSNode<StateT, ActionT>& o ) const { return state == o.state; }

	std::size_t hash() const { return state.hash(); }

	//! Print the node into the given stream
	friend std::ostream& operator<<(std::ostream &os, const SBFWSNode<StateT, ActionT>& object) { return object.print(os); }
	std::ostream& print(std::ostream& os) const {
		return os << "{@ = " << this << ", s = " << state << ", g = " << g << ", w = " << novelty << ", #g=" << unachieved << ", #r=" << _relevant_atoms.num_reached() << ", t=" << _type << ", parent = " << parent << "}";
	}

	template <typename HeuristicT>
	void evaluate_with(HeuristicT& heuristic) {
		// The order of the operations below matters.

		// Update the number of unachieved goal atoms
		unachieved = heuristic.compute_unachieved(this->state);

		// Update the set of relevant atoms
		heuristic.update_relevant_atoms(*this, _relevant_atoms);

		// Finally, compute the novelty of the node wrt both #g (unachieved) and #r.
		novelty = heuristic.novelty(*this, unachieved, _relevant_atoms.num_reached());
	}

	void inherit_heuristic_estimate() {
		throw std::runtime_error("This shouldn't be invoked");
	}

	//! What to do when an 'other' node is found during the search while 'this' node is already in
	//! the open list
	void update_in_open_list(ptr_t other) {
		if (other->g < this->g) {
			this->g = other->g;
			this->action = other->action;
			this->parent = other->parent;
			this->novelty = other->novelty;
			unachieved = other->unachieved;
			_relevant_atoms = other->_relevant_atoms;
		}
	}

	bool dead_end() const { return false; }
};


//! TODO - TOO HACKY?
inline unsigned _index(unsigned unachieved, unsigned relaxed_achieved) {
	return (unachieved<<16) | relaxed_achieved;
}

// Index the novelty tables by <#g, #r>
struct SBFWSNoveltyIndexer {
	unsigned operator()(unsigned unachieved, unsigned relaxed_achieved) const {
		return _index(unachieved, relaxed_achieved);
	}

	std::tuple<unsigned, unsigned> relevant(unsigned unachieved, unsigned relaxed_achieved) const {
		return std::make_tuple(unachieved, relaxed_achieved);
	}
};


//! The main Simulated BFWS heuristic object, which is in charge of computing the novelty of nodes
//! _and_, whenever necessary, of computing the set R of atoms that are relevant to the achievement
//! of the problem goal from the node state.
template <typename StateModelT, typename NoveltyIndexerT>
class SBFWSHeuristic {
public:
	SBFWSHeuristic(const StateModelT& model, const IWNoveltyEvaluator& search_evaluator, const IWNoveltyEvaluator& simulation_evaluator, BFWSStats& stats, bool mark_negative_propositions) :
		_model(model),
		_problem(model.getTask()),
		_search_evaluator(search_evaluator),
		_simulation_evaluator(simulation_evaluator),
		_novelty_evaluators(),
		_unsat_goal_atoms_heuristic(model),
		_mark_negative_propositions(mark_negative_propositions),
		_stats(stats)
	{}

	~SBFWSHeuristic() = default;

	//! Return a newly-computed set of atoms which are relevant to reach the goal from the given state, with
	//! all those atoms marked as "unreached", and the rest as irrelevant.
	RelevantAtomSet compute_relevant(const State& state, bool log_stats) const {
		using ActionT = typename StateModelT::ActionType;
		using NodeT = IWRunNode<State, ActionT>;
		using IWAlgorithm = IWRun<NodeT, StateModelT>;
		
		if (_simulation_evaluator.max_novelty() == 0) { // No need to run anything
			return RelevantAtomSet(&(_problem.get_tuple_index()));
		}
		
		_stats.simulation();
		
		auto iw = std::unique_ptr<IWAlgorithm>(IWAlgorithm::build(_model, _simulation_evaluator, _mark_negative_propositions));
		
		//BFWSStats stats;
		//StatsObserver<NodeT, BFWSStats> st_obs(stats, false);
		//iw->subscribe(st_obs);
		
		iw->run(state);
		unsigned reachable = 0;
		RelevantAtomSet relevant = iw->retrieve_relevant_atoms(state, reachable);
		
		//LPT_INFO("cout", "IW Simulation: Node expansions: " << stats.expanded());
		//LPT_INFO("cout", "IW Simulation: Node generations: " << stats.generated());

		if (log_stats) {
			_stats.set_initial_reachable_subgoals(reachable);
			_stats.set_initial_relevant_atoms(relevant.num_unreached());
		}

		return relevant;
	}


	//! Compute the novelty of the state wrt all the states with the same heuristic value.
	template <typename NodeT>
	unsigned novelty(NodeT& node, unsigned unachieved, unsigned relaxed_achieved) {
		auto ind = _indexer(unachieved, relaxed_achieved);
#ifdef DEBUG
		// Let's make sure that either it's the first time we see this index, or, if was already there,
		// it corresponds to the same combination of <unachieved, relaxed_achieved>
		auto tuple = _indexer.relevant(unachieved, relaxed_achieved);
		auto __it =  __novelty_idx_values.find(ind);
		if (__it == __novelty_idx_values.end()) {
			__novelty_idx_values.insert(std::make_pair(ind, tuple));
		} else {
			assert(__it->second == tuple);
		}
#endif
		auto it = _novelty_evaluators.find(ind);
		if (it == _novelty_evaluators.end()) {
			auto inserted = _novelty_evaluators.insert(std::make_pair(ind, _search_evaluator));
			it = inserted.first;
		}

		IWNoveltyEvaluator& evaluator = it->second;

		node._type = ind;
		node.feature_valuation = evaluator.compute_valuation(node.state);
		return evaluator.evaluate(node);
	}

	template <typename NodeT>
	void update_relevant_atoms(NodeT& node, RelevantAtomSet& atomset) const {
		// Only for the root node _or_ whenever the number of unachieved nodes decreases
		// do we recompute the set of relevant atoms.
		if (!node.has_parent() || node.unachieved < node.parent->unachieved) {
			node._relevant_atoms = compute_relevant(node.state, !node.has_parent());
		} else {
			// We copy the map of reached values from the parent node
			node._relevant_atoms = node.parent->_relevant_atoms;
		}

		// In both cases, we update the set of relevant nodes with those that have been reached.
		node._relevant_atoms.mark(node.state, nullptr, RelevantAtomSet::STATUS::REACHED, _mark_negative_propositions, true);
	}

	unsigned compute_unachieved(const State& state) {
		return _unsat_goal_atoms_heuristic.evaluate(state);
	}

protected:
#ifdef DEBUG
	// Just for sanity check purposes
	std::map<unsigned, std::tuple<unsigned,unsigned>> __novelty_idx_values;
#endif

	const StateModelT& _model;

	const Problem& _problem;

	// We keep a base evaluator to be cloned each time a new one is needed, so that there's no need
	// to perform all the feature selection, etc. anew.
	const IWNoveltyEvaluator& _search_evaluator;
	const IWNoveltyEvaluator& _simulation_evaluator;

	//! We have one different novelty evaluators for each actual heuristic value that a node might have.
	std::unordered_map<long, IWNoveltyEvaluator> _novelty_evaluators;

	//! An UnsatisfiedGoalAtomsHeuristic to count the number of unsatisfied goals
	UnsatisfiedGoalAtomsHeuristic<StateModelT> _unsat_goal_atoms_heuristic;

	NoveltyIndexerT _indexer;
	bool _mark_negative_propositions;
	
	BFWSStats& _stats;
};

template <typename NodeT>
struct SBFWSNodeComparer {
	using NodePtrT = std::shared_ptr<NodeT>;
	bool operator()(const NodePtrT& n1, const NodePtrT& n2) const {
		if (n1->novelty > n2->novelty ) return true;
		if (n1->novelty < n2->novelty ) return false;
		if (n1->unachieved > n2->unachieved) return true;
		if (n1->unachieved < n2->unachieved) return false;
		return n1->g > n2->g;
	}
};

struct SBFWSConfig {
	SBFWSConfig(const Config&);
	SBFWSConfig(const SBFWSConfig&) = default;
	SBFWSConfig& operator=(const SBFWSConfig&) = default;
	SBFWSConfig(SBFWSConfig&&) = default;
	SBFWSConfig& operator=(SBFWSConfig&&) = default;

	//! The maximum levels of width for search and simulation
	const unsigned search_width;
	const unsigned simulation_width;
	const bool mark_negative_propositions;
};


//! The main Simulated BFWS driver, sets everything up and runs the search.
//! The basic setup is:
//!
//! - Perform a (ATM naive) (novelty-) feature selection.
//!
//! - Create a "base" novelty evaluatorfor the search and a different one for the simulation.
//!   They might e.g. have different max_widths, or different features.
//!
//! - Instantiate the SBFWSHeuristic heuristic object, which will compute each node novelty,
//!   number of unachieved nodes #g and (if necessary) set of relevant atoms R.
//!
//! - Instantiate the (GBFS) search object, and start the search with it.
//!
template <typename StateModelT>
class SimulatedBFWSDriver : public drivers::Driver {
public:
	using ActionT = typename StateModelT::ActionType;
	using NodeT = SBFWSNode<fs0::State, ActionT>;
	using NodeCompareT = SBFWSNodeComparer<NodeT>;
	using HeuristicEnsembleT = SBFWSHeuristic<StateModelT, SBFWSNoveltyIndexer>;
	using RawEngineT = lapkt::StlBestFirstSearch<NodeT, HeuristicEnsembleT, StateModelT, std::shared_ptr<NodeT>, NodeCompareT>;
	using EngineT = std::unique_ptr<RawEngineT>;

	//! Factory method
	EngineT create(const Config& config, SBFWSConfig& conf, const NoveltyFeaturesConfiguration& feature_configuration, const StateModelT& model) {

		// Create here one instance to be copied around, so that no need to keep reanalysing which features are relevant
		_featureset = selectFeatures(feature_configuration);

		_search_evaluator = std::unique_ptr<IWNoveltyEvaluator>(new IWNoveltyEvaluator(conf.search_width, _featureset));
		_simulation_evaluator = std::unique_ptr<IWNoveltyEvaluator>(new IWNoveltyEvaluator(conf.simulation_width, _featureset));


		_heuristic = std::unique_ptr<HeuristicEnsembleT>(new HeuristicEnsembleT(model, *_search_evaluator, *_simulation_evaluator, _stats, conf.mark_negative_propositions));

		auto engine = EngineT(new RawEngineT(model, *_heuristic));

		drivers::EventUtils::setup_stats_observer<NodeT>(_stats, _handlers);
		drivers::EventUtils::setup_evaluation_observer<NodeT, HeuristicEnsembleT>(config, *_heuristic, _stats, _handlers);
		lapkt::events::subscribe(*engine, _handlers);

		return engine;
	}

	const BFWSStats& getStats() const { return _stats; }

	ExitCode search(Problem& problem, const Config& config, const std::string& out_dir, float start_time) override;

protected:
	//!
	std::unique_ptr<HeuristicEnsembleT> _heuristic;

	//!
	std::vector<std::unique_ptr<lapkt::events::EventHandler>> _handlers;

	//!
	BFWSStats _stats;

	//! The feature set used for the novelty computations
	FeatureSet _featureset;

	//! We keep a "base" novelty evaluator with the appropriate features, which (ATM)
	//! we will use for both search and heuristic simulation
	std::unique_ptr<IWNoveltyEvaluator> _search_evaluator;
	std::unique_ptr<IWNoveltyEvaluator> _simulation_evaluator;

	// ATM we don't perform any particular feature selection
	bfws::FeatureSet selectFeatures(const NoveltyFeaturesConfiguration& feature_configuration) {
		const ProblemInfo& info = ProblemInfo::getInstance();

		FeatureSet features;

		// Add all state variables
		for (VariableIdx var = 0; var < info.getNumVariables(); ++var) {
			features.push_back(std::unique_ptr<NoveltyFeature>(new StateVariableFeature(var)));
		}

		LPT_INFO("cout", "Number of features from which state novelty will be computed: " << features.size());
		return features;
	}

	//! Helper
	ExitCode do_search(const StateModelT& model, const Config& config, const std::string& out_dir, float start_time);
};


} } // namespaces
