
#include <limits>
#include <sstream>

#include <fs/core/actions/actions.hxx>
#include <fs/core/problem_info.hxx>
#include <fs/core/utils/printers/binding.hxx>
#include <fs/core/utils/printers/actions.hxx>
#include <fs/core/utils/utils.hxx>
#include <fs/core/languages/fstrips/language.hxx>
#include <fs/core/state.hxx>

#include <fs/core/constraints/registry.hxx>

namespace fs0 {

ActionData::ActionData(unsigned id, const std::string& name, const Signature& signature, const std::vector<std::string>& parameter_names, const fs::BindingUnit& bunit,
					   const fs::Formula* precondition, const std::vector<const fs::ActionEffect*>& effects, ActionData::Type type)
	: _id(id), _name(name), _signature(signature), _parameter_names(parameter_names),
    _bunit(bunit), _precondition(precondition), _effects(effects), _type(type)
{
	assert(parameter_names.size() == signature.size());
}

ActionData::~ActionData() {
	delete _precondition;
	for (const auto pointer:_effects) delete pointer;
}

ActionData::ActionData(const ActionData& other) :
	_id(other._id),
	_name(other._name),
	_signature(other._signature),
	_parameter_names(other._parameter_names),
	_bunit(other._bunit),
	_precondition(other._precondition->clone()),
	_effects(Utils::copy(other._effects)),
    _type(other._type)
{}

std::ostream& ActionData::print(std::ostream& os) const {
	os <<  print::action_data_name(*this);
    if ( _type == Type::Control ) os << "[control]";
    if ( _type == Type::Exogenous ) os << "[exogenous]";
    if ( _type == Type::Natural ) os << "[natural]";
	return os;
}

bool
ActionData::has_empty_parameter() const {
	const ProblemInfo& info = ProblemInfo::getInstance();
	for (TypeIdx type:_signature) {
		if (info.getTypeObjects(type).empty()) return true;
	}
	return false;
}

ActionBase::ActionBase(const ActionData& action_data, const Binding& binding, const fs::Formula* precondition, const std::vector<const fs::ActionEffect*>& effects) :
	_data(action_data), _binding(binding), _precondition(precondition), _effects(effects) {}

ActionBase::~ActionBase() {
	delete _precondition;
	for (const auto pointer:_effects) delete pointer;
}

ActionBase::ActionBase(const ActionBase& o) :
	_data(o._data), _binding(o._binding), _precondition(o._precondition->clone()), _effects(Utils::clone(o._effects))
{}

std::ostream& ActionBase::print(std::ostream& os) const {
	os << print::strips_action_header(*this);
	return os;
}


PartiallyGroundedAction::PartiallyGroundedAction(const ActionData& action_data, const Binding& binding, const fs::Formula* precondition, const std::vector<const fs::ActionEffect*>& effects) :
	ActionBase(action_data, binding, precondition, effects)
{}

void
PartiallyGroundedAction::apply( const State& s, std::vector<Atom>& atoms ) const {
    throw std::runtime_error("Runtime Error: Lifted actions cannot be executed!");
}

GroundAction::GroundAction(unsigned id, const ActionData& action_data, const Binding& binding, const fs::Formula* precondition, const std::vector<const fs::ActionEffect*>& effects) :
	ActionBase(action_data, binding, precondition, effects), _id(id)
{}

void
GroundAction::apply( const State& s, std::vector<Atom>& atoms ) const {
    NaiveApplicabilityManager::computeEffects(s, *this, atoms);
}

const ActionIdx GroundAction::invalid_action_id = std::numeric_limits<unsigned int>::max();

ProceduralAction::ProceduralAction(unsigned id, const ActionData& action_data, const Binding& binding, const fs::Formula* precondition, const std::vector<const fs::ActionEffect*>& effects)
	: GroundAction(id, action_data, binding, precondition, effects), _proc_effect(nullptr)
{
	if (!hasProceduralEffects())
		throw std::runtime_error("ProceduralAction::ProceduralAction() : procedural action was created but it has no procedural effects attached");
	_proc_effect = LogicalComponentRegistry::instance().instantiate_effect( getName() );
	_proc_effect->bind(binding);
}

ProceduralAction::ProceduralAction( unsigned id, const ActionData& action_data, const Binding& binding)
    : GroundAction( id, action_data, binding, new fs::Tautology, {} ), _proc_effect(nullptr)
{
}

ProceduralAction::~ProceduralAction() {
    delete _proc_effect;
}

void
ProceduralAction::apply( const State& s, std::vector<Atom>& atoms ) const {
	// 1. Apply declarative effects if any
	NaiveApplicabilityManager::computeEffects(s, *this, atoms);
	// 2. Apply the effects of the procedural effect
	if (_proc_effect == nullptr)
		throw std::runtime_error( "ProceduralAction::apply() : no externally defined effect procedural action was set!");
	if (_proc_effect->applicable(s))
		_proc_effect->apply(s, atoms);
}

std::pair<VariableIdx, object_id> unpack_atom(const fs::Term* lhs, const fs::Term* rhs) {
    auto sv = dynamic_cast<const fs::StateVariable*>(lhs);
    if (!sv) throw std::runtime_error("Cannot compile given ground action into plain operator");
    auto c = dynamic_cast<const fs::Constant*>(rhs);
    if (!c) throw std::runtime_error("Cannot compile given ground action into plain operator");
    return std::make_pair(sv->getValue(), c->getValue());
}

PlainOperator compile_action_to_plan_operator(const GroundAction& action) {
    std::vector<std::pair<VariableIdx, object_id>> precondition;
    std::vector<std::pair<VariableIdx, object_id>> effects;

    // Compile precondition
    const auto* conjunction = dynamic_cast<const fs::Conjunction*>(action.getPrecondition());
    const auto* taut = dynamic_cast<const fs::Tautology*>(action.getPrecondition());
    const auto* atom = dynamic_cast<const fs::EQAtomicFormula*>(action.getPrecondition());

    if (conjunction) { // Wrap out the conjuncts
        for (const auto& sub:conjunction->getSubformulae()) {
            const auto* sub_atom = dynamic_cast<const fs::EQAtomicFormula*>(sub);
            if (!sub_atom) throw std::runtime_error("Cannot compile given ground action into plain operator");
            precondition.emplace_back(unpack_atom(sub_atom->lhs(), sub_atom->rhs()));
        }

    } else if (atom) {
        precondition.emplace_back(unpack_atom(atom->lhs(), atom->rhs()));

    } else if (taut) {
        // No need to do anything - the empty vector will be the right precondition
    } else {
        throw std::runtime_error("Cannot compile given ground action into plain operator");
    }

    // Compile effects
    for (const auto& eff:action.getEffects()) {
        effects.emplace_back(unpack_atom(eff->lhs(), eff->rhs()));
    }

    return PlainOperator(precondition, effects);
}


} // namespaces
