#pragma once

#include "Adc.hpp"


namespace coco {

/// @brief Dummy ADC interface implementation that returns a pre-defined value
///
class AdcDummy : public Adc {
public:
	AdcDummy(int value = 0) : value(value) {}
	~AdcDummy() override;

	int get(int channel) override;

	int value;
};

} // namespace coco
