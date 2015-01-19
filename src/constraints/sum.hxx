
#pragma once

#include <memory>
#include <vector>
#include <constraints/constraints.hxx>

namespace fs0 {


/**
 * A Sum constraint custom propagator. 
 */
class SumConstraint : public Constraint
{
public:
	
	SumConstraint(unsigned arity);
	
	virtual ~SumConstraint() {}
	
	bool isSatisfied(const ObjectIdxVector&  values) const;
	
	Output filter(const DomainVector& domains);
};


} // namespaces

