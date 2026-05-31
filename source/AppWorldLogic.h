#ifndef __APP_WORLD_LOGIC_H__
#define __APP_WORLD_LOGIC_H__

#include <queue>
#include <boost/asio.hpp>

#include <UnigineLogic.h>
#include <UnigineStreams.h>

#include "eventsubscribers/eventsubscriber.h"

class AppWorldLogic : public Unigine::WorldLogic
{

public:
	AppWorldLogic();
	~AppWorldLogic() override;

	int init() override;
	int update() override;
	int postUpdate() override;
	int updatePhysics() override;
	int shutdown() override;

	void enqueueEvent(const FlightEvent &event);
	FlightEvent popEvent();
	bool hasEvents() const { return !m_eventQueue.empty(); }
	std::size_t getEventQueueSize() const { return m_eventQueue.size(); }

private:
	std::queue<FlightEvent> m_eventQueue;
};

#endif // __APP_WORLD_LOGIC_H__
