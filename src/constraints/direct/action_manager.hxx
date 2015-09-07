
#pragma once

#include <heuristics/relaxed_plan/action_managers/base_action_manager.hxx>
#include <constraints/direct/csp_handler.hxx>
#include <constraints/direct/effect.hxx>

namespace fs0 {

class GroundAction;


/**
 * A constraint manager capable only of dealing with actions whose constraints are at most unary,
 * in which case the handling is much simpler and more efficient.
 * Note that this restriction in particular excludes nested-fluent terms.
 */
class DirectActionManager : public BaseActionManager
{
public:
	//! Constructs a manager handling the given set of constraints
	static DirectActionManager* create(const GroundAction& action);
	
	~DirectActionManager();
	
	const GroundAction& getAction() const { return _action; }

	virtual void process(unsigned actionIdx, const RelaxedState& layer, RPGData& rpg);

	//!
	virtual bool checkPreconditionApplicability(const DomainMap& domains) const;

protected:
	
	//! Private constructor
	DirectActionManager(const GroundAction& action, std::vector<DirectConstraint::cptr>&& constraints, std::vector<DirectEffect::cptr>&& effects);
	
	//!
	void processEffects(unsigned actionIdx, const DomainMap& actionProjection, RPGData& rpg) const;
	
	//! The action being managed
	const GroundAction& _action;
	
	const std::vector<DirectConstraint::cptr> _constraints;
	
	const std::vector<DirectEffect::cptr> _effects;
	
		
	//! The indexes of only those variables relevant to the applicability procedures of the action.
	const VariableIdxVector _scope;
	
	//! The indexes of all (direct) state variables relevant to at least one of the effect or applicability procedures of the action.
	const std::vector<VariableIdx> _allRelevant;
	
	const DirectCSPHandler _handler;
	
	//!
	void completeAtomSupport(const VariableIdxVector& actionScope, const DomainMap& actionProjection, const VariableIdxVector& effectScope, Atom::vctrp support) const;
	
	//! Extracts all the (direct) state variables that are relevant to the action
	VariableIdxVector extractAllRelevant() const;
	
	std::ostream& print(std::ostream& os) const;
};


} // namespaces