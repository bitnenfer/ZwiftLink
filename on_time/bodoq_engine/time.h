#pragma once

namespace time
{
	double ms();
	inline float secToMs(float secs) { return secs * 1000.0f; }
}
