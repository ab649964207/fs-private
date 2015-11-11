
#pragma once

#include <fs0_types.hxx>

namespace fs0 { namespace language { namespace fstrips {

//! A (possibly partial) binding of quantified variables / action parameters to actual values
class Binding {
protected:
	std::vector<ObjectIdx> _values;
	std::vector<bool> _set;
	
public:
	//! Construct an empty binding
	Binding() {}
	
	Binding(std::size_t size) 
		: _values(size), _set(size)
	{}
	
	//! Returns true iff the current binding contains a binding for the given variable
	bool binds(unsigned variable) const { return _set.at(variable); }
	
	ObjectIdx value(unsigned variable) const {
		assert(binds(variable));
		return _values.at(variable);
	}
	
	void set(unsigned variable, ObjectIdx value) {
		_values[variable] = value;
		_set[variable] = true;
	}
	
	void unset(unsigned variable) { _set[variable] = false; }
	
	std::size_t size() const { return _values.size(); }

	//! Prints a representation of the object to the given stream.
	friend std::ostream& operator<<(std::ostream &os, const Binding& o) { return o.print(os); }
	
	std::ostream& print(std::ostream& os) const {
		os << "{";
		for (unsigned i = 0; i < _values.size(); ++i) {
			if (!binds(i)) continue;
			os << i << ": " << value(i) << ", ";
		}
		os << "}";
		return os;
	}
};

} } } // namespaces
