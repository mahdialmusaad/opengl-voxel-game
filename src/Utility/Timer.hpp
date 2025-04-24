#pragma once
#ifndef _SOURCE_UTILITY_TIMER_HEADER_
#define _SOURCE_UTILITY_TIMER_HEADER_

#include <chrono>
#include <iostream>

// Very simple and lacking timer class for testing performance.

class GameTimer
{
private:
	std::chrono::high_resolution_clock timer;
	std::chrono::system_clock::time_point start;
public:
	GameTimer();
	void Results();
};

#endif // _SOURCE_UTILITY_TIMER_HEADER_
