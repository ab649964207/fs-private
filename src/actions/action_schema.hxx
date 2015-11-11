
#pragma once

#include <fs0_types.hxx>
#include <languages/fstrips/language.hxx>


using namespace fs0::language::fstrips;

namespace fs0 {

class ProblemInfo; class GroundAction;

class ActionSchema {
protected:
	const std::string _name;
	const Signature _signature;
	const std::vector<std::string> _parameters;
	const Formula::cptr _precondition;
	const std::vector<ActionEffect::cptr> _effects;

public:
	typedef const ActionSchema* cptr;
	ActionSchema(const std::string& name,
				 const Signature& signature, const std::vector<std::string>& parameters,
			     const Formula::cptr precondition, const std::vector<ActionEffect::cptr>& effects);
	~ActionSchema();
	
	inline const std::string& getName() const { return _name; }
	inline const Signature& getSignature() const { return _signature; }
	inline const std::vector<std::string>& getParameters() const { return _parameters; }

	//! Prints a representation of the object to the given stream.
	friend std::ostream& operator<<(std::ostream &os, const ActionSchema& o) { return o.print(os); }
	std::ostream& print(std::ostream& os) const;
	std::ostream& print(std::ostream& os, const fs0::ProblemInfo& info) const;
	
	//! Process the action schema with a given parameter binding and return the corresponding GroundAction
	//! A nullptr is returned if the action is detected to be statically non-applicable
	GroundAction* bind(const Binding& binding, const ProblemInfo& info) const;
};


} // namespaces
