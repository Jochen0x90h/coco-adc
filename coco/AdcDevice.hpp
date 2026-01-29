#pragma once

#include <coco/BufferDevice.hpp>


namespace coco {

/// @brief ADC device
///
class AdcDevice : public BufferDevice {
public:
	AdcDevice(State state) : BufferDevice(state) {}

	/// @brief Check if an overflow happened and clear the overflow state
	/// An overflow happens when the ADC does a conversion but no buffer was active to store the value
	/// @return true if overrun happened
	virtual bool overrun() = 0;
};

} // namespace coco
