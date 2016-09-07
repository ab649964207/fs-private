
#include <constraints/gecode/supports.hxx>
#include <constraints/gecode/csp_translator.hxx>
#include <constraints/gecode/gecode_csp.hxx>
#include <utils/tuple_index.hxx>
#include <problem.hxx>

namespace fs0 { namespace gecode {

std::vector<TupleIdx>
Supports::extract_support(const GecodeCSP* solution, const CSPTranslator& translator, const std::vector<std::pair<unsigned, std::vector<unsigned>>>& tuple_indexes, const std::vector<TupleIdx>& necessary_tuples) {
	const auto& tuple_index = Problem::getInstance().get_tuple_index();
	std::vector<TupleIdx> support;
	
	// We extract the actual support from the solution
	// First process the direct state variables
	for (const auto& element:translator.getAllInputVariables()) {
		VariableIdx variable = element.first;
		ObjectIdx value = translator.resolveVariableFromIndex(element.second, *solution).val();
		
		support.push_back(tuple_index.to_index(variable, value));
	}
	
	// Now the rest of fluent elements, i.e. the nested fluent terms
	for (const auto& tuple_info:tuple_indexes) {
		unsigned symbol = tuple_info.first;
		
		ValueTuple tuple;
		for (unsigned subterm_idx:tuple_info.second) {
			tuple.push_back(translator.resolveValueFromIndex(subterm_idx, *solution));
		}
		
		support.push_back(tuple_index.to_index(symbol, tuple));
	}
	
	// Now the support of atoms such as 'clear(b)' that might appear in formulas in non-negated form.
	support.insert(support.end(), necessary_tuples.begin(), necessary_tuples.end());
	
	/*
	// Now, insert support tuples related to nested terms that have been mapped into element constraints
	for (auto fluent:effect_nested_fluents) {
		const NestedFluentData& nested_data = getNestedFluentTranslator(fluent).getNestedFluentData();
		VariableIdx variable = nested_data.resolveStateVariable(*solution);

		ObjectIdx value = _translator.resolveValue(fluent, *solution);
		
		
		translator.resolveValueFromIndex(subterm_idx, *solution);
		
		support.push_back(tuple_index.to_index(variable, value));
		}
	}
	*/
	
	return support;
}



} } // namespaces
