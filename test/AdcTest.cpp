#include <coco/debug.hpp>
#include <AdcTest.hpp>


using namespace coco;

/*
	ADC test
	A sawtooth waveform is output on generator DAC channels 0 and 1.
	The ADC samples the sawtooth waveform on channels 0 and 1 (connect DAC outputs to ADC inputs as described in board header).
	The sampled values are output on test DAC channels 0 and 1 so that they can be checked with an oscilloscope.
*/


Coroutine run(Loop &loop, Dac &generatorDac, Adc &adc, Dac &testDac) {
	uint8_t level = 0;
	while (true) {
		// set level to ADC inputs using ADC
		generatorDac.set(0, level << 8);
		generatorDac.set(1, int8_t(level + 128) << 8);
		++level;

		// sample buffer
		int val0 = adc.get(0);
		int val1 = adc.get(1);

		// output value on second channel of DAC so that it can be measured with an oscilloscope
		testDac.set(0, val0);
		testDac.set(1, val1);

		co_await loop.sleep(10ms);
	}
}


int main() {
	// run test
	run(drivers.loop, drivers.generatorDac, drivers.adc, drivers.testDac);

	drivers.loop.run();
	return 0;
}
