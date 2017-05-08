#pragma once

#include <memory>
#include <lapkt/tools/logging.hxx>
#include <heuristics/novelty/features.hxx>
#include <unordered_map>
#include <utils/external.hxx>
#include <problem_info.hxx>

namespace fs0 { class Config; }

namespace fs0 { namespace language { namespace fstrips { class Formula; } }}
namespace fs = fs0::language::fstrips;

namespace fs0 { namespace drivers {

//! A custom heuristic for the CTMP problem
//! h(s)= number of goal objects that still need to be picked up and moved in s  * 2
//! +  1; if goal object being held
class hMHeuristic {
public:
	hMHeuristic(const fs::Formula* goal);
	~hMHeuristic() = default;
	hMHeuristic(const hMHeuristic&) = default;

	void setup_goal_confs();
	unsigned evaluate(const State& s, const std::vector<bool>& is_path_to_goal_atom_clear) const;

protected:
	// The two following vectors are sync'd, i.e. _all_objects_conf[i] is the config of object _all_objects_ids[i]
	std::vector<ObjectIdx> _all_objects_ids; // The Ids of all objects
	std::vector<VariableIdx> _all_objects_conf; // The state variables of the configurations of all objects
	std::unordered_map<ObjectIdx, ObjectIdx> _all_objects_goal; // The configuration in the goal of each object, if any
	
	VariableIdx _holding_var;
	const ExternalI& _external;
	std::vector<unsigned> _idx_goal_atom; // The index of the goal atom in which a certain object_id appears
};

} } // namespaces