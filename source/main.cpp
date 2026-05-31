// #include <boost/asio.hpp>
#include <iostream>

#include <UnigineEngine.h>

#include "AppSystemLogic.h"
#include "AppWorldLogic.h"
// #include "eventsubscribers/helix.h"
// #include "eventsubscribers/udpreceiver.h"

// namespace asio = boost::asio;

// asio::awaitable<void> engineLoop(
// 	AppSystemLogic *systemLogic,
// 	AppWorldLogic *worldLogic,
// 	Unigine::EnginePtr engine,
// 	UDPReceiver *udpReceiver)
// {
// 	auto executor = co_await asio::this_coro::executor;

// 	asio::steady_timer timer(executor);
// 	while (true)
// 	{
// 		if (engine->isQuit())
// 		{
// 			udpReceiver->stop();
// 			co_return;
// 		}
// 		engine->iterate();
// 		// expire immediately
// 		timer.expires_after(std::chrono::milliseconds(0));
// 		co_await timer.async_wait(asio::use_awaitable);
// 	}
// }

#ifdef _WIN32
int wmain(int argc, wchar_t *argv[])
#else
int main(int argc, char *argv[])
#endif
{
	// asio::io_context io;
	// engine
	AppSystemLogic systemLogic;
	AppWorldLogic worldLogic;
	Unigine::EnginePtr engine(argc, argv);
	engine->addSystemLogic(&systemLogic);
	engine->addWorldLogic(&worldLogic);
	engine->main();
	// event subscriber
	// UDPReceiver udpReceiver(&io);
	// udpReceiver.start(&worldLogic);
	// start engine loop via asio co-routine
	// asio::co_spawn(io, engineLoop(&systemLogic, &worldLogic, engine, &udpReceiver), asio::detached);
	// io.run();
	return 0;
}