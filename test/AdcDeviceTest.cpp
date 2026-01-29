#include <coco/debug.hpp>
#include <AdcDeviceTest.hpp>


using namespace coco;

/*
    ADC device test
    A sawtooth waveform with about 24Hz is output on DAC channel 0.
	The ADC samples the sawtooth waveform (connect DAC output to ADC input as described in AdcDeviceTest.hpp for each board).
    The green LED shows if the sampled value is >= 128.
    The sampled values are output on DAC channel 1 so that they can be measured with an oscilloscope.
*/

constexpr int GENERATE_INDEX = 0;
constexpr int OSCILLOSCOPE_INDEX = 1;


Coroutine run(Loop &loop, Dac &dac, Buffer &buffer1, Buffer &buffer2) {
    uint8_t level = 0;
    while (true) {
        // set level to ADC input via first channel of DAC
        dac.set(GENERATE_INDEX, level << 8);
        ++level;
        //debug::toggleGreen();

        // sample buffer 1
        co_await buffer1.untilReadyOrDisabled();
        auto val1 = buffer1.pointer<Sample>()[SAMPLE_COUNT - 1];
        debug::setGreen((val1 & 0x80) != 0);

        // output value on second channel of DAC so that it can be measured using an oscilloscope
        dac.set(OSCILLOSCOPE_INDEX, val1 << 8);

        // start buffer 1 again
        buffer1.startRead(SAMPLE_COUNT * sizeof(Sample));


        // set level to ADC input via channel 0 of DAC
        dac.set(GENERATE_INDEX, level << 8);
        ++level;
        debug::toggleRed();

        // wait for buffer 2
        co_await buffer2.untilReadyOrDisabled();
        auto val2 = buffer2.pointer<Sample>()[SAMPLE_COUNT - 1];
        debug::setGreen((val2 & 0x80) != 0);

        // output value on channel 1 of DAC so that it can be measured
        dac.set(OSCILLOSCOPE_INDEX, val2 << 8);

        // start buffer 2 again
        buffer2.startRead(SAMPLE_COUNT * sizeof(Sample));
    }
}


int main() {
    debug::out << "AdcDeviceTest\n";

    // run test
    run(drivers.loop, drivers.dac, drivers.buffer1, drivers.buffer2);

    drivers.loop.run();
    return 0;
}
