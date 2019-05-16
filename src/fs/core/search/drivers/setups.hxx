
#pragma once

#include <fs/core/models/ground_state_model.hxx>
#include <fs/core/models/simple_state_model.hxx>

#include <lapkt/tools/events.hxx>
#include <fs/core/search/events.hxx>

namespace fs0 { class Problem; }

namespace fs0 { namespace drivers {

//! A catalog of common setups for grounding actions for both search and heuristic computations.
class GroundingSetup {
public:
	//! A simple model with all grounded actions
	static GroundStateModel fully_ground_model(Problem& problem);
	
	static SimpleStateModel fully_ground_simple_model(Problem& problem);
};

class EventUtils {
public:
	using HandlerPtr = std::unique_ptr<lapkt::events::EventHandler>;
	
	template <typename NodeT, typename StatsT>
	static void setup_stats_observer(StatsT& stats, std::vector<HandlerPtr>& handlers) {
		using StatsObserverT = StatsObserver<NodeT, StatsT>;
		handlers.push_back(std::unique_ptr<StatsObserverT>(new StatsObserverT(stats)));
	}
};

} } // namespaces
