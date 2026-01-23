#include <coco/debug.hpp>
#include <AdcDeviceTest.hpp>


using namespace coco;

/*
	ADC device test
	The ADC samples the value of DAC channel 1 which gets incremented.
	The green LED shows if the sampled value is >= 128.
	The result is output on DAC channel 2 so it can be checked with an oscilloscope.
*/


Coroutine run(Loop &loop, Dac &dac, Buffer &buffer1, Buffer &buffer2) {
	uint8_t level = 0;
	while (true) {
		// set level to ADC input via first channel of DAC
		dac.set(0, level << 8);
		++level;
		//debug::toggleGreen();

		// sample buffer 1
		co_await buffer1.untilReadyOrDisabled();
		int val1 = buffer1.pointer<Sample>()[SAMPLE_COUNT - 1];
		debug::setGreen((val1 & 0x80) != 0);

		// output value on second channel of DAC so that it can be measured
		dac.set(1, val1 << 8);

		// start buffer 1 again
		buffer1.startRead(SAMPLE_COUNT * sizeof(Sample));


		// set level to ADC input via first channel of DAC
		dac.set(0, level << 8);
		++level;
		debug::toggleRed();

		// wait for buffer 2
		co_await buffer2.untilReadyOrDisabled();
		int val2 = buffer2.pointer<Sample>()[SAMPLE_COUNT - 1];
		debug::setGreen((val2 & 0x80) != 0);

		// output value on second channel of DAC so that it can be measured
		dac.set(1, val2 << 8);

		// start buffer 2 again
		buffer2.startRead(SAMPLE_COUNT * sizeof(Sample));
	}
}


int main() {
	// run test
	run(drivers.loop, drivers.dac, drivers.buffer1, drivers.buffer2);

	drivers.loop.run();
	return 0;
}
