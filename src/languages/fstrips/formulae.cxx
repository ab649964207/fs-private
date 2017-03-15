
#include <problem_info.hxx>
#include <languages/fstrips/formulae.hxx>
#include <languages/fstrips/terms.hxx>
#include <problem.hxx>
#include <utils/utils.hxx>
#include <state.hxx>
#include <lapkt/tools/logging.hxx>
#include <utils/binding.hxx>


namespace fs0 { namespace language { namespace fstrips {


// A small workaround to circumvent the fact that boost containers do not seem to allow initializer lists
typedef RelationalFormula::Symbol AFSymbol;
const std::map<AFSymbol, std::string> RelationalFormula::symbol_to_string{
	{AFSymbol::EQ, "="}, {AFSymbol::NEQ, "!="}, {AFSymbol::LT, "<"}, {AFSymbol::LEQ, "<="}, {AFSymbol::GT, ">"}, {AFSymbol::GEQ, ">="}
};
// const std::map<std::string, AFSymbol> RelationalFormula::string_to_symbol(Utils::flip_map(symbol_to_string));


bool Formula::interpret(const PartialAssignment& assignment) const { return interpret(assignment, Binding::EMPTY_BINDING); }
bool Formula::interpret(const State& state) const  { return interpret(state, Binding::EMPTY_BINDING); }


AtomicFormula::~AtomicFormula() {
	for (const auto ptr:_subterms) delete ptr;
}

AtomicFormula* AtomicFormula::clone() const { return clone(Utils::clone(_subterms)); }

bool AtomicFormula::interpret(const PartialAssignment& assignment, const Binding& binding) const {
	NestedTerm::interpret_subterms(_subterms, assignment, binding, _interpreted_subterms);
	return _satisfied(_interpreted_subterms);
}

bool AtomicFormula::interpret(const State& state, const Binding& binding) const {
	NestedTerm::interpret_subterms(_subterms, state, binding, _interpreted_subterms);
	return _satisfied(_interpreted_subterms);
}

std::ostream& RelationalFormula::print(std::ostream& os, const fs0::ProblemInfo& info) const {
	os << *_subterms[0] << " " << RelationalFormula::symbol_to_string.at(symbol()) << " " << *_subterms[1];
	return os;
}


std::ostream& ExternallyDefinedFormula::print(std::ostream& os, const fs0::ProblemInfo& info) const {
       os << name() << "(";
       for (const auto ptr:_subterms) os << *ptr << ", ";
       os << ")";
       return os;
}


std::ostream& AxiomaticFormula::print(std::ostream& os, const fs0::ProblemInfo& info) const {
	os << name() << "(";
	for (const auto ptr:_subterms) os << *ptr << ", ";
	os << ")";
	return os;
}

bool AxiomaticFormula::interpret(const PartialAssignment& assignment, const Binding& binding) const {
	throw std::runtime_error("UNIMPLEMENTED");
}

bool AxiomaticFormula::interpret(const State& state, const Binding& binding) const {
	NestedTerm::interpret_subterms(_subterms, state, binding, _interpreted_subterms);
	return compute(state, _interpreted_subterms);
}

OpenFormula::OpenFormula(const OpenFormula& other) :
	_subformulae(Utils::clone(other._subformulae))
{}


std::ostream& OpenFormula::
print(std::ostream& os, const fs0::ProblemInfo& info) const {
	os << name() << " ( ";
	for (unsigned i = 0; i < _subformulae.size(); ++i) {
		os << *_subformulae.at(i);
		if (i < _subformulae.size() - 1) os << ", ";
	}
	os << " ) ";
	return os;
}

bool Conjunction::
interpret(const PartialAssignment& assignment, const Binding& binding) const {
	for (auto elem:_subformulae) {
		if (!elem->interpret(assignment, binding)) return false;
	}
	return true;
}

bool Conjunction::
interpret(const State& state, const Binding& binding) const {
	for (auto elem:_subformulae) {
		if (!elem->interpret(state, binding)) return false;
	}
	return true;
}


bool AtomConjunction::
interpret(const State& state) const {
	for (const auto& atom:_atoms) {
		if ( state.getValue(atom.first) != atom.second) return false;
	}
	return true;
}

bool Disjunction::
interpret(const PartialAssignment& assignment, const Binding& binding) const {
	for (auto elem:_subformulae) {
		if (elem->interpret(assignment, binding)) return true;
	}
	return false;
}

bool Disjunction::
interpret(const State& state, const Binding& binding) const {
	for (auto elem:_subformulae) {
		if (elem->interpret(state, binding)) return true;
	}
	return false;
}


bool Negation::
interpret(const PartialAssignment& assignment, const Binding& binding) const {
	return !_subformulae[0]->interpret(assignment, binding);
}

bool Negation::
interpret(const State& state, const Binding& binding) const {
	return !_subformulae[0]->interpret(state, binding);
}


QuantifiedFormula::QuantifiedFormula(const QuantifiedFormula& other) :
_variables(Utils::clone(other._variables)), _subformula(other._subformula->clone())
{}

//! Prints a representation of the object to the given stream.
std::ostream& QuantifiedFormula::print(std::ostream& os, const fs0::ProblemInfo& info) const {
	os << name() << " ";
	for (const BoundVariable* var:_variables) {
		os << *var << ": " << info.getTypename(var->getType()) << " s.t. ";
	}
	os << "(" << *_subformula << ")";
	return os;
}

bool ExistentiallyQuantifiedFormula::interpret(const PartialAssignment& assignment, const Binding& binding) const {
	assert(binding.size()==0); // ATM we do not allow for nested quantifications
	return interpret_rec(assignment, Binding(_variables.size()), 0);
}

bool ExistentiallyQuantifiedFormula::interpret(const State& state, const Binding& binding) const {
	assert(binding.size()==0); // ATM we do not allow for nested quantifications
	return interpret_rec(state, Binding(_variables.size()), 0);
}

template <typename T>
bool ExistentiallyQuantifiedFormula::interpret_rec(const T& assignment, const Binding& binding, unsigned i) const {
	// Base case - all existentially quantified variables have been bound
	if (i == _variables.size()) return _subformula->interpret(assignment, binding);

	const ProblemInfo& info = ProblemInfo::getInstance();
	const BoundVariable* variable = _variables.at(i);
	Binding copy(binding);
	//! Otherwise, iterate through all possible assignments to the currently analyzed variable 'i'
	for (ObjectIdx elem:info.getTypeObjects(variable->getType())) {
		copy.set(variable->getVariableId(), elem);
		if (interpret_rec(assignment, copy, i + 1)) return true;
	}
	return false;
}


bool UniversallyQuantifiedFormula::interpret(const PartialAssignment& assignment, const Binding& binding) const {
	//assert(binding.size()==0); // ATM we do not allow for nested quantifications
	return interpret_rec(assignment, Binding(_variables.size()), 0);
}

bool UniversallyQuantifiedFormula::interpret(const State& state, const Binding& binding) const {
	//assert(binding.size()==0); // ATM we do not allow for nested quantifications
	return interpret_rec(state, Binding(_variables.size()), 0);
}

template <typename T>
bool UniversallyQuantifiedFormula::interpret_rec(const T& assignment, const Binding& binding, unsigned i) const {
	// Base case - all existentially quantified variables have been bound
	if (i == _variables.size()) return _subformula->interpret(assignment, binding);

	const ProblemInfo& info = ProblemInfo::getInstance();
	const BoundVariable* variable = _variables.at(i);
	Binding copy(binding);
	//! Otherwise, iterate through all possible assignments to the currently analyzed variable 'i'
	for (ObjectIdx elem:info.getTypeObjects(variable->getType())) {
		copy.set(variable->getVariableId(), elem);
		if (!interpret_rec(assignment, copy, i + 1)) return false;
	}
	return true;
}









} } } // namespaces
