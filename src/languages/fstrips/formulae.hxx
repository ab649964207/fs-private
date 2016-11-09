
#pragma once

#include <iostream>

#include <fs_types.hxx>

namespace fs0 {
class State;
class ProblemInfo;
class Binding;
}

namespace fs0 { namespace language { namespace fstrips {

class Term;
class BoundVariable;
class AtomicFormula;
class Conjunction;
class ExistentiallyQuantifiedFormula;
class Tautology;
class Contradiction;

//! The base interface for a logic formula
class Formula {
public:
	Formula() {}
	virtual ~Formula() {}
	
	//! Clone idiom
	virtual Formula* clone() const = 0;
	
	//! Processes a formula possibly containing bound variables and non-consolidated state variables,
	//! consolidating all possible state variables and performing the bindings according to the given variable binding
	virtual const Formula* bind(const Binding& binding, const ProblemInfo& info) const = 0;
	
	//! Return the boolean interpretation of the current formula under the given assignment and binding.
	virtual bool interpret(const PartialAssignment& assignment, const Binding& binding) const = 0;
	virtual bool interpret(const State& state, const Binding& binding) const = 0;
	bool interpret(const PartialAssignment& assignment) const;
	bool interpret(const State& state) const;
	
	//! The level of nestedness of the formula
	virtual unsigned nestedness() const = 0;
	
	//! Prints a representation of the object to the given stream.
	friend std::ostream& operator<<(std::ostream &os, const Formula& o) { return o.print(os); }
	std::ostream& print(std::ostream& os) const;
	virtual std::ostream& print(std::ostream& os, const fs0::ProblemInfo& info) const = 0;
	
	//! Returns a vector with all the terms involved in the current formula
	std::vector<const Term*> all_terms() const;
	
	//! Returns a vector with all the subformulae involved in the current formula
	virtual std::vector<const Formula*> all_formulae() const = 0;
	
	//! A small helper - returns a vector with all the atomic formulae involved in the current formula
	std::vector<const AtomicFormula*> all_atoms() const;
	
	//! By default, formulae are not tautology nor contradiction
	virtual bool is_tautology() const { return false; }
	virtual bool is_contradiction() const { return false; }
	
	//! Logical operations - ugly, but simple
	virtual Formula* conjunction(const Formula* 						other) const = 0;
	virtual Formula* conjunction(const AtomicFormula* 					other) const = 0;
	virtual Formula* conjunction(const Conjunction* 					other) const = 0;
	virtual Formula* conjunction(const ExistentiallyQuantifiedFormula*	other) const = 0;
	virtual Formula* conjunction(const Tautology* 						other) const { return clone(); }
	virtual Formula* conjunction(const Contradiction* 					other) const;
};

//! An atomic formula, implicitly understood to be static (fluent atoms are considered terms with Boolean codomain)
class AtomicFormula : public Formula {
public:
	AtomicFormula(const std::vector<const Term*>& subterms) : _subterms(subterms), _interpreted_subterms(subterms.size(), -1) {}
	
	virtual ~AtomicFormula();
	
	//! Clone the type of formula assigning the given subterms
	virtual AtomicFormula* clone(const std::vector<const Term*>& subterms) const = 0;
	AtomicFormula* clone() const;
	
	const Formula* bind(const fs0::Binding& binding, const fs0::ProblemInfo& info) const;

	const std::vector<const Term*>& getSubterms() const { return _subterms; }
	
	bool interpret(const PartialAssignment& assignment, const Binding& binding) const;
	bool interpret(const State& state, const Binding& binding) const;
	using Formula::interpret;
	
	unsigned nestedness() const;
	
	std::vector<const Formula*> all_formulae() const { return std::vector<const Formula*>(1, this); }
	
	//! Prints a representation of the object to the given stream.
	virtual std::ostream& print(std::ostream& os, const fs0::ProblemInfo& info) const = 0;
	
	std::vector<const Term*> all_terms() const;
	
	virtual Formula* conjunction(const Formula* 							other) const { return other->conjunction(this); }
	virtual Formula* conjunction(const AtomicFormula* 						other) const { throw std::runtime_error("Unimplemented"); }
	virtual Formula* conjunction(const Conjunction* 						other) const { throw std::runtime_error("Unimplemented"); }
	virtual Formula* conjunction(const ExistentiallyQuantifiedFormula*		other) const { throw std::runtime_error("Unimplemented"); }
	
	//! A helper to recursively evaluate the formula - must be subclassed
	virtual bool _satisfied(const ObjectIdxVector& values) const = 0;
	
protected:
	//! The formula subterms
	std::vector<const Term*> _subterms;
	
	//! The last interpretation of the subterms (acts as a cache)
	mutable std::vector<ObjectIdx> _interpreted_subterms;
};


//! The True truth value
class Tautology : public Formula {
public:
	Tautology* bind(const Binding& binding, const ProblemInfo& info) const { return new Tautology; }
	Tautology* clone() const { return new Tautology; }
	
	unsigned nestedness() const { return 0; }
	
	std::vector<const Formula*> all_formulae() const { return std::vector<const Formula*>(1, this); }
	
	bool interpret(const PartialAssignment& assignment, const Binding& binding) const { return true; }
	bool interpret(const State& state, const Binding& binding) const { return true; }
	
	bool is_tautology() const { return true; }
	
	//! Prints a representation of the object to the given stream.
	std::ostream& print(std::ostream& os, const fs0::ProblemInfo& info) const { os << "True"; return os; }
	
	Formula* conjunction(const Formula* 							other) const;
	Formula* conjunction(const AtomicFormula* 						other) const;
	Formula* conjunction(const Conjunction* 						other) const;
	Formula* conjunction(const ExistentiallyQuantifiedFormula*		other) const;
};

//! The False truth value
class Contradiction : public Formula {
public:
	Contradiction* bind(const Binding& binding, const ProblemInfo& info) const { return new Contradiction; }
	Contradiction* clone() const { return new Contradiction; }
	
	unsigned nestedness() const { return 0; }
	
	std::vector<const Formula*> all_formulae() const { return std::vector<const Formula*>(1, this); }
	
	bool interpret(const PartialAssignment& assignment, const Binding& binding) const { return false; }
	bool interpret(const State& state, const Binding& binding) const { return false; }
	
	bool is_contradiction() const { return true; }
	
	//! Prints a representation of the object to the given stream.
	std::ostream& print(std::ostream& os, const fs0::ProblemInfo& info) const { os << "False"; return os; }

	Formula* conjunction(const Formula* 							other) const;
	Formula* conjunction(const AtomicFormula* 						other) const;
	Formula* conjunction(const Conjunction* 						other) const;
	Formula* conjunction(const ExistentiallyQuantifiedFormula*		other) const;
};

//! A logical conjunction
class Conjunction : public Formula {
public:
	friend class LogicalOperations;
	
	Conjunction(const std::vector<const AtomicFormula*>& conjuncts) : _conjuncts(conjuncts) {}
	
	Conjunction(const Conjunction& conjunction) {
		for (const AtomicFormula* conjunct:conjunction._conjuncts) {
			_conjuncts.push_back(conjunct->clone());
		}
	}
	
	virtual ~Conjunction() {
		for (const auto ptr:_conjuncts) delete ptr;
	}
	
	Conjunction* clone() const { return new Conjunction(*this); }
	
	const Formula* bind(const Binding& binding, const fs0::ProblemInfo& info) const;
	
	const std::vector<const AtomicFormula*>& getConjuncts() const { return _conjuncts; }
	
	bool interpret(const PartialAssignment& assignment, const Binding& binding) const;
	bool interpret(const State& state, const Binding& binding) const;
	
	unsigned nestedness() const;
	
	std::vector<const Formula*> all_formulae() const;
	
	//! Prints a representation of the object to the given stream.
	virtual std::ostream& print(std::ostream& os, const fs0::ProblemInfo& info) const;
	
	Formula* conjunction(const fs0::language::fstrips::Formula* other) const;
	Formula* conjunction(const fs0::language::fstrips::AtomicFormula* other) const;
	Conjunction* conjunction(const Conjunction* 						other) const;
	Formula* conjunction(const fs0::language::fstrips::ExistentiallyQuantifiedFormula* other) const;
	
protected:
	//! The formula subterms
	std::vector<const AtomicFormula*> _conjuncts;
};


//! An atomic formula, implicitly understood to be static (fluent atoms are considered terms with Boolean codomain)
class ExistentiallyQuantifiedFormula : public Formula {
public:
	friend class LogicalOperations;
	
	ExistentiallyQuantifiedFormula(const std::vector<const BoundVariable*>& variables, const Conjunction* subformula) : _variables(variables), _subformula(subformula) {}
	
	virtual ~ExistentiallyQuantifiedFormula() {
		delete _subformula;
	}
	
	ExistentiallyQuantifiedFormula(const ExistentiallyQuantifiedFormula& other);
	
	ExistentiallyQuantifiedFormula* clone() const { return new ExistentiallyQuantifiedFormula(*this); }
	
	const Formula* bind(const Binding& binding, const fs0::ProblemInfo& info) const;
	
	const Conjunction* getSubformula() const { return _subformula; }
	
	bool interpret(const PartialAssignment& assignment, const Binding& binding) const;
	bool interpret(const State& state, const Binding& binding) const;
	
	unsigned nestedness() const { return _subformula->nestedness(); }
	
	std::vector<const Formula*> all_formulae() const;
	
	//! Prints a representation of the object to the given stream.
	std::ostream& print(std::ostream& os, const fs0::ProblemInfo& info) const;
	
	Formula* conjunction(const Formula* 							other) const;
	Formula* conjunction(const AtomicFormula* 						other) const;
	Formula* conjunction(const Conjunction* 						other) const;
	Formula* conjunction(const ExistentiallyQuantifiedFormula*		other) const;
	
protected:
	//! The binding IDs of the existentially quantified variables
	std::vector<const BoundVariable*> _variables;
	
	//! ATM we only allow quantification of conjunctions
	const Conjunction* _subformula;
	
	//! A naive recursive implementation of the interpretation routine
	template <typename T>
	bool interpret_rec(const T& assignment, const Binding& binding, unsigned i) const;
};

/*
//! A formula such as 'clear(b)', where clear is one of the problem's fluents.
class FluentAtom : public AtomicFormula {
public:
	FluentAtom(unsigned symbol_id, const std::vector<const Term*>& subterms) : AtomicFormula(subterms), _symbol_id(symbol_id)
	{}

	FluentAtom* clone(const std::vector<const Term*>& subterms) const = 0;
	
	//! Prints a representation of the object to the given stream.
	virtual std::ostream& print(std::ostream& os, const fs0::ProblemInfo& info) const;
	
protected:
	bool _satisfied(const ObjectIdxVector& values) const;
	
	unsigned _symbol_id;
};
*/


//! A formula of the form t_1 <op> t_2, where t_i are terms and <op> is a basic relational
//! operator such as =, !=, >, etc.
class RelationalFormula : public AtomicFormula {
public:
	enum class Symbol {EQ, NEQ, LT, LEQ, GT, GEQ};
	
	RelationalFormula(const std::vector<const Term*>& subterms) : AtomicFormula(subterms) {
		assert(subterms.size() == 2);
	}
	
	virtual Symbol symbol() const = 0;

	virtual RelationalFormula* clone(const std::vector<const Term*>& subterms) const = 0;
	
	//! Prints a representation of the object to the given stream.
	virtual std::ostream& print(std::ostream& os, const fs0::ProblemInfo& info) const;
	
	
	const static std::map<RelationalFormula::Symbol, std::string> symbol_to_string;
// 	const static std::map<std::string, RelationalFormula::Symbol> string_to_symbol;
	
	const Term* lhs() const { return _subterms[0]; }
	const Term* rhs() const { return _subterms[1]; }
	
protected:
	inline bool _satisfied(const ObjectIdxVector& values) const { return _satisfied(values[0], values[1]); }
	virtual bool _satisfied(ObjectIdx o1, ObjectIdx o2) const = 0;
};

class EQAtomicFormula : public RelationalFormula {
public:
	EQAtomicFormula(const std::vector<const Term*>& subterms) : RelationalFormula(subterms) {}
	
	EQAtomicFormula* clone(const std::vector<const Term*>& subterms) const { return new EQAtomicFormula(subterms); }
	
	inline bool _satisfied(ObjectIdx o1, ObjectIdx o2) const { return o1 == o2; }
	
	virtual Symbol symbol() const { return Symbol::EQ; }
};

class NEQAtomicFormula : public RelationalFormula {
public:
	NEQAtomicFormula(const std::vector<const Term*>& subterms) : RelationalFormula(subterms) {}
	
	NEQAtomicFormula* clone(const std::vector<const Term*>& subterms) const { return new NEQAtomicFormula(subterms); }
	
	inline bool _satisfied(ObjectIdx o1, ObjectIdx o2) const { return o1 != o2; }
	
	Symbol symbol() const { return Symbol::NEQ; }
};

class LTAtomicFormula : public RelationalFormula {
public:
	LTAtomicFormula(const std::vector<const Term*>& subterms) : RelationalFormula(subterms) {}
	
	LTAtomicFormula* clone(const std::vector<const Term*>& subterms) const { return new LTAtomicFormula(subterms); }
		
	bool _satisfied(ObjectIdx o1, ObjectIdx o2) const { return o1 < o2; }
	
	Symbol symbol() const { return Symbol::LT; }
};

class LEQAtomicFormula : public RelationalFormula {
public:
	LEQAtomicFormula(const std::vector<const Term*>& subterms) : RelationalFormula(subterms) {}
	
	LEQAtomicFormula* clone(const std::vector<const Term*>& subterms) const { return new LEQAtomicFormula(subterms); }
		
	inline bool _satisfied(ObjectIdx o1, ObjectIdx o2) const { return o1 <= o2; }
	
	Symbol symbol() const { return Symbol::LEQ; }
};

class GTAtomicFormula : public RelationalFormula {
public:
	GTAtomicFormula(const std::vector<const Term*>& subterms) : RelationalFormula(subterms) {}
	
	GTAtomicFormula* clone(const std::vector<const Term*>& subterms) const { return new GTAtomicFormula(subterms); }
		
	inline bool _satisfied(ObjectIdx o1, ObjectIdx o2) const { return o1 > o2; }
	
	Symbol symbol() const { return Symbol::GT; }
};

class GEQAtomicFormula : public RelationalFormula {
public:
	GEQAtomicFormula(const std::vector<const Term*>& subterms) : RelationalFormula(subterms) {}
	
	GEQAtomicFormula* clone(const std::vector<const Term*>& subterms) const { return new GEQAtomicFormula(subterms); }
		
	inline bool _satisfied(ObjectIdx o1, ObjectIdx o2) const { return o1 >= o2; }
	
	Symbol symbol() const { return Symbol::GEQ; }
};

} } } // namespaces
