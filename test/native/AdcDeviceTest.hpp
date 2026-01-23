#pragma once

#include <coco/DacDummy.hpp>
#include <coco/platform/Loop_native.hpp>
#include <coco/platform/BufferDevice_cout.hpp>


using namespace coco;

using Sample = uint8_t;
constexpr int SAMPLE_COUNT = 16;

/**
 * Drivers for AdcDeviceTest
 */
struct Drivers {
	Loop_native loop;

	// dummy ADC
	using Adc = BufferDevice_cout;
	Adc adc{loop, "ADC", 50ms};
	Adc::Buffer buffer1{SAMPLE_COUNT, adc};
	Adc::Buffer buffer2{SAMPLE_COUNT, adc};

	// dummy DAC
	using Dac = DacDummy;
	Dac dac;
};

Drivers drivers;
