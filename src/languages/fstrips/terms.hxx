
#pragma once

#include <fs_types.hxx>
#include <problem_info.hxx>

namespace fs0 { class State; class Binding; }

namespace fs0 { namespace language { namespace fstrips {

//! A logical term in FSTRIPS
class Term {
public:
	Term() {}
	virtual ~Term() {}

	//! Clone idiom
	virtual Term* clone() const = 0;

	//! Processes a term possibly containing bound variables and non-consolidated state variables,
	//! consolidating all possible state variables and performing the bindings according to the given variable binding
	virtual const Term* bind(const Binding& binding, const ProblemInfo& info) const = 0;
	
	//! Returns the level of nestedness of the term.
	virtual unsigned nestedness() const = 0;

	//! Returns true if the element is flat, i.e. is a state variable or a constant
	virtual bool flat() const = 0;

	// Returns a list with all terms contained in this term's tree, including itself (possibly with repetitions)
	virtual std::vector<const Term*> all_terms() const = 0;

	//! Returns the value of the current term under the given (possibly partial) interpretation
	virtual ObjectIdx interpret(const PartialAssignment& assignment, const Binding& binding) const = 0;
	virtual ObjectIdx interpret(const State& state, const Binding& binding) const = 0;
	ObjectIdx interpret(const PartialAssignment& assignment) const;
	ObjectIdx interpret(const State& state) const;

	//! Returns the index of the state variable to which the current term resolves under the given state.
	virtual VariableIdx interpretVariable(const PartialAssignment& assignment, const Binding& binding) const = 0;
	virtual VariableIdx interpretVariable(const State& state, const Binding& binding) const = 0;
	VariableIdx interpretVariable(const PartialAssignment& assignment) const;
	VariableIdx interpretVariable(const State& state) const;

	virtual TypeIdx getType() const = 0;
	
	virtual std::pair<int, int> getBounds() const = 0;

	//! Prints a representation of the object to the given stream.
	friend std::ostream& operator<<(std::ostream &os, const Term& o) { return o.print(os); }
	std::ostream& print(std::ostream& os) const;
	virtual std::ostream& print(std::ostream& os, const ProblemInfo& info) const;

	virtual bool operator==(const Term& other) const = 0;
	inline bool operator!=(const Term& rhs) const { return !this->operator==(rhs); }
	virtual std::size_t hash_code() const = 0;
};

//! A nested logical term in FSTRIPS, i.e. a term of the form f(t_1, ..., t_n)
//! The class is abstract and intended to have two possible subclasses, depending on whether
//! the functional symbol 'f' is fluent or not.
class NestedTerm : public Term {
public:
	//! Factory method to create a nested term of the appropriate type
	static const Term* create(const std::string& symbol, const std::vector<const Term*>& subterms);

	NestedTerm(unsigned symbol_id, const std::vector<const Term*>& subterms)
		: _symbol_id(symbol_id), _subterms(subterms), _interpreted_subterms(subterms.size(), -1)
	{}

	virtual ~NestedTerm() {
		for (const Term* term:_subterms) delete term;
	}
	
	NestedTerm(const NestedTerm& term);
	
	virtual const Term* bind(const Binding& binding, const ProblemInfo& info) const;

	bool flat() const { return false; }

	std::vector<const Term*> all_terms() const;
	
	virtual TypeIdx getType() const;

	//! Prints a representation of the object to the given stream.
	virtual std::ostream& print(std::ostream& os, const fs0::ProblemInfo& info) const;

	//! A small helper
	template <typename T>
	static std::ostream& printFunction(std::ostream& os, const fs0::ProblemInfo& info, unsigned symbol_id, const std::vector<T*>& subterms) {
		os << info.getSymbolName(symbol_id) << "(";
		for (unsigned i = 0; i < subterms.size(); ++i) {
			os << *subterms[i];
			if (i < subterms.size() - 1) os << ", ";
		}
		os << ")";
		return os;
	}

	//! A helper to interpret a vector of terms
	template <typename T>
	static std::vector<ObjectIdx>
	interpret_subterms(const std::vector<const Term*>& subterms, const T& assignment, const Binding& binding) {
		std::vector<ObjectIdx> interpreted(subterms.size());
		interpret_subterms(subterms, assignment, binding);
		return interpreted;
	}

	template <typename T>
	static void
	interpret_subterms(const std::vector<const Term*>& subterms, const T& assignment, const Binding& binding, std::vector<ObjectIdx>& interpreted) {
		assert(interpreted.size() == subterms.size());
		for (unsigned i = 0, sz = subterms.size(); i < sz; ++i) {
			interpreted[i] = subterms[i]->interpret(assignment, binding);
		}
	}

	unsigned getSymbolId() const { return _symbol_id; }

	const std::vector<const Term*>& getSubterms() const { return _subterms; }

	bool operator==(const Term& other) const;
	virtual std::size_t hash_code() const;

	//! A helper to process lists of subterms
	static std::vector<const Term*> bind_subterms(const std::vector<const Term*>& subterms, const Binding& binding, const ProblemInfo& info, std::vector<ObjectIdx>& constants);


protected:
	//! The ID of the function or predicate symbol, e.g. in the state variable loc(A), the id of 'loc'
	unsigned _symbol_id;

	//! The tuple of fixed, constant symbols of the state variable, e.g. {A, B} in the state variable 'on(A,B)'
	// TODO This should be const
	std::vector<const Term*> _subterms;
	
	//! The last interpretation of the subterms (acts as a cache)
	mutable std::vector<ObjectIdx> _interpreted_subterms;

	unsigned maxSubtermNestedness() const {
		unsigned max = 0;
		for (const Term* subterm:_subterms) max = std::max(max, subterm->nestedness());
		return max;
	}
};


//! A nested term headed by a static functional symbol
class StaticHeadedNestedTerm : public NestedTerm {
public:
	StaticHeadedNestedTerm(unsigned symbol_id, const std::vector<const Term*>& subterms);

	virtual ObjectIdx interpret(const PartialAssignment& assignment, const Binding& binding) const = 0;
	virtual ObjectIdx interpret(const State& state, const Binding& binding) const = 0;

	VariableIdx interpretVariable(const PartialAssignment& assignment, const Binding& binding) const { throw std::runtime_error("static-headed terms cannot resolve to an state variable"); }
	VariableIdx interpretVariable(const State& state, const Binding& binding) const { throw std::runtime_error("static-headed terms cannot resolve to an state variable"); }
	
	// A nested term headed by a static symbol has as many levels of nestedness as the maximum of its subterms
	unsigned nestedness() const { return maxSubtermNestedness(); }
};

//! A statically-headed term that performs some arithmetic operation to its two subterms
class ArithmeticTerm : public StaticHeadedNestedTerm {
public:
	ArithmeticTerm(const std::vector<const Term*>& subterms);
	
	ArithmeticTerm* clone() const = 0;
	
	const Term* bind(const Binding& binding, const ProblemInfo& info) const;
	
	//! Creates an arithmetic term of the same type than the current one but with the given subterms
	// TODO - This is ATM somewhat inefficient because of the redundancy of cloning the whole array of subterms only to delete it.
	const Term* create(const std::vector<const Term*>& subterms) const {
		ArithmeticTerm* term = clone();
		for (const auto ptr:term->_subterms) delete ptr;
		term->_subterms = subterms;
		return term;
	}
};

//! A statically-headed term defined extensionally or otherwise by the concrete planning instance
class UserDefinedStaticTerm : public StaticHeadedNestedTerm {
public:
	UserDefinedStaticTerm(unsigned symbol_id, const std::vector<const Term*>& subterms);

	UserDefinedStaticTerm* clone() const { return new UserDefinedStaticTerm(*this); }

	virtual TypeIdx getType() const;
	virtual std::pair<int, int> getBounds() const;

	ObjectIdx interpret(const PartialAssignment& assignment, const Binding& binding) const;
	ObjectIdx interpret(const State& state, const Binding& binding) const;
	
	const Term* bind(const Binding& binding, const ProblemInfo& info) const;

protected:
	// The (static) logical function implementation
	const SymbolData& _function;
};


//! A nested term headed by a fluent functional symbol
class FluentHeadedNestedTerm : public NestedTerm {
public:
	FluentHeadedNestedTerm(unsigned symbol_id, const std::vector<const Term*>& subterms)
		: NestedTerm(symbol_id, subterms) {}

	FluentHeadedNestedTerm* clone() const { return new FluentHeadedNestedTerm(*this); }

	ObjectIdx interpret(const PartialAssignment& assignment, const Binding& binding) const;
	ObjectIdx interpret(const State& state, const Binding& binding) const;

	VariableIdx interpretVariable(const PartialAssignment& assignment, const Binding& binding) const;
	VariableIdx interpretVariable(const State& state, const Binding& binding) const;

	virtual std::pair<int, int> getBounds() const;
	
	const Term* bind(const Binding& binding, const ProblemInfo& info) const;

	// A nested term headed by a fluent symbol has as many levels of nestedness as the maximum of its subterms plus one (standing for itself)
	unsigned nestedness() const { return maxSubtermNestedness() + 1; }
};

//! A logical variable bound to some existential or universal quantifier
class BoundVariable : public Term {
public:
	BoundVariable(unsigned id, TypeIdx type) : _id(id), _type(type) {}

	BoundVariable* clone() const { return new BoundVariable(*this); }
	
	const Term* bind(const Binding& binding, const ProblemInfo& info) const;

	virtual unsigned nestedness() const { return 1; }

	bool flat() const { return true; }
	
	virtual TypeIdx getType() const;

	std::vector<const Term*> all_terms() const { return std::vector<const Term*>(1, this); }

	//! Returns the unique quantified variable ID
	unsigned getVariableId() const { return _id; }

	ObjectIdx interpret(const PartialAssignment& assignment, const Binding& binding) const;
	ObjectIdx interpret(const State& state, const Binding& binding) const;

	VariableIdx interpretVariable(const PartialAssignment& assignment, const Binding& binding) const { throw std::runtime_error("Bound variables cannot resolve to an state variable"); }
	VariableIdx interpretVariable(const State& state, const Binding& binding) const { throw std::runtime_error("Bound variables terms cannot resolve to an state variable"); }

	std::pair<int, int> getBounds() const;

	//! Prints a representation of the object to the given stream.
	std::ostream& print(std::ostream& os, const fs0::ProblemInfo& info) const;

	bool operator==(const Term& other) const;
	virtual std::size_t hash_code() const;

protected:
	//! The ID of the variable, which will be unique throughout the whole binding unit.
	unsigned _id;
	
	TypeIdx _type;
};

//! A state variable is a term 'f(t)', where f is a fluent symbol and t is a tuple of fixed constant symbols.
//! 'loc(a)', with a being an object, for instance, is a state variable
class StateVariable : public Term {
public:
	StateVariable(VariableIdx variable_id, const FluentHeadedNestedTerm* origin) 
		: _variable_id(variable_id), _origin(origin)
	{}
	
	virtual ~StateVariable() {
		delete _origin;
	}

	StateVariable(const StateVariable& variable)
		: _variable_id(variable._variable_id), 
		  _origin(variable._origin->clone())
	{}

	StateVariable* clone() const { return new StateVariable(*this); }
	
	//! Nothing to be done for binding, simply return a clone of the element
	const Term* bind(const Binding& binding, const ProblemInfo& info) const { return clone(); }

	virtual unsigned nestedness() const { return 0; }

	bool flat() const { return true; }
	
	virtual TypeIdx getType() const;

	std::vector<const Term*> all_terms() const { return std::vector<const Term*>(1, this); }

	//! Returns the index of the state variable
	VariableIdx getValue() const { return _variable_id; }

	ObjectIdx interpret(const PartialAssignment& assignment, const Binding& binding) const { return assignment.at(_variable_id); }
	ObjectIdx interpret(const State& state, const Binding& binding) const;

	VariableIdx interpretVariable(const PartialAssignment& assignment, const Binding& binding) const { return _variable_id; }
	VariableIdx interpretVariable(const State& state, const Binding& binding) const { return _variable_id; }
	
	const FluentHeadedNestedTerm* getOrigin() const { return _origin; }
	
	unsigned getSymbolId() const { return _origin->getSymbolId(); }
 
	virtual const std::vector<const Term*>& getSubterms() const { return _origin->getSubterms(); }

	virtual std::pair<int, int> getBounds() const;

	//! Prints a representation of the object to the given stream.
	virtual std::ostream& print(std::ostream& os, const fs0::ProblemInfo& info) const;

	bool operator==(const Term& other) const;
	virtual std::size_t hash_code() const;

protected:
	//! The ID of the state variable
	VariableIdx _variable_id;

	//! The originating symbol ID and subterms
	const FluentHeadedNestedTerm* _origin;
};


//! A simple constant term.
class Constant : public Term {
public:
	Constant(ObjectIdx value)  : _value(value) {}

	Constant* clone() const { return new Constant(*this); }
	
	//! Nothing to be done for binding, simply return a clone of the element
	const Term* bind(const Binding& binding, const ProblemInfo& info) const { return clone(); }

	virtual unsigned nestedness() const { return 0; }

	bool flat() const { return true; }
	
	virtual TypeIdx getType() const {
		throw std::runtime_error("Unimplemented");
	}

	std::vector<const Term*> all_terms() const { return std::vector<const Term*>(1, this); }

	//! Returns the actual value of the constant
	ObjectIdx getValue() const { return _value; }

	// The value of a constant is independent of the assignment
	ObjectIdx interpret(const PartialAssignment& assignment, const Binding& binding) const { return _value; }
	ObjectIdx interpret(const State& state, const Binding& binding) const { { return _value; }}

	VariableIdx interpretVariable(const PartialAssignment& assignment, const Binding& binding) const { throw std::runtime_error("Constant terms cannot resolve to an state variable"); }
	VariableIdx interpretVariable(const State& state, const Binding& binding) const { throw std::runtime_error("Constant terms cannot resolve to an state variable"); }

	virtual std::pair<int, int> getBounds() const { return std::make_pair(_value, _value); }

	//! Prints a representation of the object to the given stream.
	virtual std::ostream& print(std::ostream& os, const fs0::ProblemInfo& info) const;

	bool operator==(const Term& other) const;
	virtual std::size_t hash_code() const;

protected:
	//! The actual value of the constant
	ObjectIdx _value;
};


//! An integer constant
class IntConstant : public Constant {
public:
	IntConstant(ObjectIdx value)  : Constant(value) {}

	IntConstant* clone() const { return new IntConstant(*this); }

	virtual std::pair<int, int> getBounds() const { return std::make_pair(_value, _value); }

	//! Prints a representation of the object to the given stream.
	virtual std::ostream& print(std::ostream& os, const fs0::ProblemInfo& info) const;
};


} } } // namespaces


// std specializations for terms and term pointers that will allow us to use them in hash-table-like structures
namespace fs = fs0::language::fstrips;
namespace std {
    template<> struct hash<fs::Term> {
        std::size_t operator()(const fs::Term& term) const { return term.hash_code(); }
    };

    template<> struct hash<const fs::Term*> {
        std::size_t operator()(const fs::Term* term) const { return hash<fs::Term>()(*term); }
    };
	
    template<> struct equal_to<const fs::Term*> {
        std::size_t operator()(const fs::Term* t1, const fs::Term* t2) const { return equal_to<fs::Term>()(*t1, *t2); }
    };
}
