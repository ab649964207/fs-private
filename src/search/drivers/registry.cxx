
#include <problem.hxx>
#include <search/drivers/registry.hxx>
#include <search/drivers/gbfs_constrained.hxx>
#include <search/drivers/iterated_width.hxx>
#include <search/drivers/breadth_first_search.hxx>
#include <search/drivers/bfws/bfws.hxx>
#include "bfws/enhanced_bfws.hxx"
// #include <search/drivers/asp_engine.hxx>
#include <search/drivers/unreached_atom_driver.hxx>
#include <search/drivers/native_driver.hxx>
#include <search/drivers/smart_effect_driver.hxx>
#include <search/drivers/smart_lifted_driver.hxx>
#include "fully_lifted_driver.hxx"
// #include <heuristics/relaxed_plan/direct_crpg.hxx>
// #include <heuristics/relaxed_plan/gecode_crpg.hxx>
#include <actions/ground_action_iterator.hxx>
#include <actions/grounding.hxx>
#include <problem_info.hxx>

// using namespace fs0::gecode;

namespace fs0 { namespace drivers {



EngineRegistry& EngineRegistry::instance() {
	static EngineRegistry theInstance;
	return theInstance;
}

EngineRegistry::EngineRegistry() {
	// We register the pre-configured search drivers on the instantiation of the singleton
// 	add("standard",  new GBFSConstrainedHeuristicsCreator());
	
	add("native",  new NativeDriver<GroundStateModel>());
// 	add("lnative",  new NativeDriver<LiftedStateModel>()); // The native driver is not ready for this
	
// 	add("lite",  new NativeDriver());
	add("lunreached",  new UnreachedAtomDriver<LiftedStateModel>());
	add("lifted",  new FullyLiftedDriver());
	
	add("iw",  new IteratedWidthDriver<GroundStateModel>());
	add("liw",  new IteratedWidthDriver<LiftedStateModel>());
	
	add("bfws",  new BFWSDriver<GroundStateModel>());
	add("lbfws",  new BFWSDriver<LiftedStateModel>());
	
	add("ebfws",  new EnhancedBFWSDriver());
	
	add("bfs",  new BreadthFirstSearchDriver());
	
	add("smart",  new SmartEffectDriver());
	add("lsmart",  new SmartLiftedDriver());
	
// 	add("asp_engine",  new ASPEngine());
}

EngineRegistry::~EngineRegistry() {
	for (const auto elem:_creators) delete elem.second;
}

void EngineRegistry::add(const std::string& engine_name, Driver* creator) {
auto res = _creators.insert(std::make_pair(engine_name, creator));
	if (!res.second) throw new std::runtime_error("Duplicate registration of engine creator for symbol " + engine_name);
}


Driver* EngineRegistry::get(const std::string& engine_name) {
	auto it = _creators.find(engine_name);
	if (it == _creators.end()) throw std::runtime_error("No engine creator has been registered for given engine name '" + engine_name + "'");
	return it->second;
}


} } // namespaces
