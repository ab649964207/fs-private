
#pragma once

#include <fs/core/fs_types.hxx>


namespace fs0 {

class State; class GroundAction;

class Projections {
public:
	//! Projects a non-relaxed state into a subset of relevant values, which returns.
	static std::vector<object_id> project(const State& s, const VariableIdxVector& scope);

	//! Zip a scope and a values to an equivalent partial assignment
	static PartialAssignment zip(const VariableIdxVector& scope, const std::vector<object_id>& values);
	
	/**
	 * Returns the projection of the domains contained in a domain map into a subset of variables.
	 * It is assumed that all the variables in scope are contained in the DomainMap `domains`.
	 */
	static DomainVector project(const DomainMap& domains, const VariableIdxVector& scope);
	
	
	//! Deep-copies a domain map
	static DomainMap clone(const DomainMap& domains);
	
	//! Helper to print sets of domains
	static void printDomain(const Domain& domain);
	static void printDomains(const DomainMap& domains);
	static void printDomains(const DomainVector& domains);
};


} // namespaces
