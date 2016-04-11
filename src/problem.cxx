
#include <problem.hxx>
#include <state.hxx>
#include <actions/actions.hxx>
#include <utils/logging.hxx>
#include <utils/printers/language.hxx>
#include <utils/printers/actions.hxx>
#include <applicability/formula_interpreter.hxx>

namespace fs0 {

std::unique_ptr<Problem> Problem::_instance = nullptr;
std::unique_ptr<ProblemInfo> Problem::_info = nullptr;


Problem::Problem(State* init, const std::vector<const ActionData*>& action_data, const fs::Formula* goal, const fs::Formula* state_constraints, TupleIndex&& tuple_index) :
	_tuple_index(std::move(tuple_index)),
	_init(init),
	_action_data(action_data),
	_ground(),
	_partials(),
	_state_constraint_formula(state_constraints),
	_goal_formula(goal),
	_goal_sat_manager(FormulaInterpreter::create(_goal_formula, get_tuple_index()))
{}

Problem::~Problem() {
	for (const auto pointer:_action_data) delete pointer;
	for (const auto pointer:_ground) delete pointer;
	for (const auto pointer:_partials) delete pointer;
	delete _state_constraint_formula;
	delete _goal_formula;
}

std::ostream& Problem::print(std::ostream& os) const { 
	const fs0::ProblemInfo& info = getProblemInfo();
	os << "Planning Problem [domain: " << info.getDomainName() << ", instance: " << info.getInstanceName() <<  "]" << std::endl;
	
	os << "Goal Conditions:" << std::endl << "------------------" << std::endl;
	os << "\t" << print::formula(*getGoalConditions()) << std::endl;
	os << std::endl;
	
	os << "State Constraints:" << std::endl << "------------------" << std::endl;
	os << "\t" << print::formula(*getStateConstraints()) << std::endl;
	os << std::endl;
	
	os << "Action data" << std::endl << "------------------" << std::endl;
	for (const ActionData* data:_action_data) {
		os << print::action_data(*data) << std::endl;
	}
	os << std::endl;
	
	os << "Ground Actions: " << _ground.size();
	// os << std::endl << "------------------" << std::endl;
	// for (const const GroundAction* elem:_ground) {
	// 	os << *elem << std::endl;
	// }
	os << std::endl;
	
	return os;
}

} // namespaces
