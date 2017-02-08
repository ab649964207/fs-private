
#include <search/drivers/sbfws/sgbfs.hxx>
#include <search/utils.hxx>
#include <models/simple_state_model.hxx>
#include <aptk2/tools/logging.hxx>
#include <heuristics/novelty/features.hxx>

namespace fs0 { namespace bfws {


//! Factory method
template <typename StateModelT, typename FeatureEvaluatorT, typename NoveltyEvaluatorT>
std::unique_ptr<LazyBFWS<StateModelT, FeatureEvaluatorT, NoveltyEvaluatorT>>
create(const Config& config, SBFWSConfig& conf, const StateModelT& model, BFWSStats& stats) {
	
	// Engine types
	using EngineT = LazyBFWS<StateModelT, FeatureEvaluatorT, NoveltyEvaluatorT>;
	
	auto search_evaluator = create_novelty_evaluator<NoveltyEvaluatorT>(model.getTask(), conf.evaluator_t, conf.search_width);
	
	return std::unique_ptr<EngineT>(new EngineT(model, search_evaluator, stats, config, conf));
}

template <>
ExitCode
LazyBFWSDriver<SimpleStateModel>::search(Problem& problem, const Config& config, const std::string& out_dir, float start_time) {
	return do_search(drivers::GroundingSetup::fully_ground_simple_model(problem), config, out_dir, start_time);
}


template <typename StateModelT>
ExitCode
LazyBFWSDriver<StateModelT>::do_search(const StateModelT& model, const Config& config, const std::string& out_dir, float start_time) {
	const StateAtomIndexer& indexer = model.getTask().getStateAtomIndexer();
	
	if (indexer.is_fully_binary()) { // The state is fully binary
		LPT_INFO("cout", "FEATURE EVALUATION: Using the specialized StraightBinaryFeatureSetEvaluator");
		using FeatureEvaluatorT = lapkt::novelty::StraightBinaryFeatureSetEvaluator<StateT>;
		return do_search2<FSBinaryNoveltyEvaluatorI, FeatureEvaluatorT>(model, config, out_dir, start_time);
		
	} else if (indexer.is_fully_multivalued()) { // The state is fully multivalued
		LPT_INFO("cout", "FEATURE EVALUATION: Using the specialized StraightMultivaluedFeatureSetEvaluator");
		using FeatureEvaluatorT = lapkt::novelty::StraightMultivaluedFeatureSetEvaluator<StateT>;
		return do_search2<FSMultivaluedNoveltyEvaluatorI, FeatureEvaluatorT>(model, config, out_dir, start_time);
		
	} else { // We have a hybrid state and cannot thus apply optimizations
		LPT_INFO("cout", "FEATURE EVALUATION: Using a generic StraightHybridFeatureSetEvaluator");
		using FeatureEvaluatorT = lapkt::novelty::StraightHybridFeatureSetEvaluator<StateT>;
		return do_search2<FSMultivaluedNoveltyEvaluatorI, FeatureEvaluatorT>(model, config, out_dir, start_time);
	}
}

/*
template <typename StateModelT>
template <typename NoveltyEvaluatorT>
ExitCode
LazyBFWSDriver<StateModelT>::do_search1(const StateModelT& model, const Config& config, const std::string& out_dir, float start_time) {

	if (true) {
		return do_search2<NoveltyEvaluatorT, lapkt::novelty::StraightBinaryFeatureSetEvaluator<StateT>>(model, config, out_dir, start_time);
	} else {
		
	}
}
*/

template <typename StateModelT>
template <typename NoveltyEvaluatorT, typename FeatureEvaluatorT>
ExitCode
LazyBFWSDriver<StateModelT>::do_search2(const StateModelT& model, const Config& config, const std::string& out_dir, float start_time) {
	SBFWSConfig bfws_config(config);
	
	
	auto engine = create<StateModelT, FeatureEvaluatorT, NoveltyEvaluatorT>(config, bfws_config, model, _stats);
	
	
	LPT_INFO("cout", "Simulated BFWS Configuration:");
// 	LPT_INFO("cout", "\tMaximum search novelty: " << bfws_config.search_width);
// 	LPT_INFO("cout", "\tMaximum simulation novelty: " << bfws_config.simulation_width);
	LPT_INFO("cout", "\tMark as relevant negative propositional atoms?: " << bfws_config.mark_negative_propositions);
// 	LPT_INFO("cout", "\tFeature extraction: " << feature_configuration);
	return drivers::Utils::do_search(*engine, model, out_dir, start_time, _stats);
}





} } // namespaces
