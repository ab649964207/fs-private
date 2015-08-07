

#pragma once

#include <fs0_types.hxx>
#include <problem_info.hxx>
#include <constraints/direct/compiled.hxx>
#include <languages/fstrips/language.hxx>
#include <gecode/int.hh>

using namespace Gecode;

namespace fs = fs0::language::fstrips;

namespace fs0 { namespace gecode {

class SimpleCSP; class GecodeCSPVariableTranslator; enum class CSPVariableType;

//! Some helper methods related to the construction of Gecode CSPs.
class Helper {
public:
	//! Creates a CSP variable constraining its domain according to the given type
	static Gecode::IntVar createVariable(Gecode::Space& csp, TypeIdx typeId);
	
	//! Creates a CSP variable constraining its domain based on the planning problem variable type
	static Gecode::IntVar createPlanningVariable(Gecode::Space& csp, VariableIdx var);
	
	//! Creates a CSP variable constraining its domain based on the given domain type
	static Gecode::IntVar createTemporaryVariable(Gecode::Space& csp, TypeIdx typeId);
	
	static Gecode::IntVar createTemporaryIntVariable(Gecode::Space& csp, int min, int max);
	
	//! Constrains the given CSP variable to have values in the given domain
	static void constrainCSPVariable(SimpleCSP& csp, unsigned csp_variable_id, const DomainPtr& domain);
	
	//! Extensionalizes a given term (with scope up to two) by building a tupleset of the values of the relevant state variables 
	//! for which the term is properly defined and their resulting interpretations
	static Gecode::TupleSet extensionalize(const fs::Term::cptr term, const VariableIdxVector& scope);

	//! Builds a gecode tupleset from the values contained in a state variable domain
	static Gecode::TupleSet buildTupleset(const fs0::Domain& domain);
	
	//! A simple helper to post a certain Gecode branching strategy to the CSP
	static void postBranchingStrategy(SimpleCSP& csp);
};

} } // namespaces