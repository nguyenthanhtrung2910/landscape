#include "InputManager.h"
#include "InputInterface.h"

#include <UnigineGame.h>

#include "components/Drone/Drone.h"
#include "components/utils.h"

using namespace Unigine;
using namespace Math;

REGISTER_COMPONENT(InputManager)

void InputManager::init()
{
	for (int i = 0; i < inputs_param.size(); i++)
	{
		if (!inputs_param[i].nullCheck())
		{
			InputInterface *input = getComponent<InputInterface>(inputs_param[i].get());
			if (input)
			{
				inputs.push_back(input);

				const auto& ibinds = input->binds;
				for (auto& bind : ibinds)
				{
					if(i == 0)
						binds[bind.key].first = bind.data;
					else
						binds[bind.key].second = bind.data;
				}
			}
			else
				Log::error("InputManager error: node %d not have a InputInterface component assigned.\n", i);
		}
		else
			Log::error("InputManager error: input node %d is null\n", i);
	}

	drone = getComponentInParent<Drone>(node);
	CHECK_COMPONENT_VALUE(drone, "can not find 'Drone' component in parent of ");
	
	active_input = inputs.size() != 0 ? inputs.last() : nullptr;
}

void InputManager::update()
{

}

void InputManager::shutdown()
{
	inputs.clear();
}


bool InputManager::isStartLanding() const
{
	for (const auto &i : inputs)
		if (i->isStartLanding())
			return true;
	return false;
}

bool InputManager::isStartFlight() const
{
	for (const auto &i : inputs)
		if (i->isStartFlight())
			return true;
	return false;
}

