#include "Timer.hpp"

// Save the start time for comparing at the end ('Results' function)
GameTimer::GameTimer() : start(timer.now()) {}

void GameTimer::Results() 
{
	// Get time taken since start
	const auto endTime = timer.now() - start;
	
	// Not trying to make it more complicated, so just print out both the ms and us just in case.
	const std::int64_t millisecs = std::chrono::duration_cast<std::chrono::milliseconds>(endTime).count();
	const std::int64_t microsecs = std::chrono::duration_cast<std::chrono::microseconds>(endTime).count();
	std::cout << "Timer ended, time taken: " << millisecs << "ms or " << microsecs << "us.\n";
}
