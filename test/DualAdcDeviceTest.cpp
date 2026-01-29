#include <coco/debug.hpp>
#include <DualAdcDeviceTest.hpp>


using namespace coco;

/*
    ADC device test
    A sawtooth waveform is output on DAC channel 0.
    The ADC samples the sawtooth waveform (connect DAC output to ADC input as described in AdcDeviceTest.hpp for each board).
    The green LED shows if the sampled value is >= 128.
    The sampled values are output on DAC channel 1 so that they can be checked with an oscilloscope.
*/


Coroutine run(Loop &loop, Dac &generatorDac, Buffer &buffer1, Buffer &buffer2, Dac &testDac) {
    uint8_t level = 0;
    while (true) {
        // set level to ADC input via generator DAC
        generatorDac.set(0, level << 8);
        generatorDac.set(1, int8_t(level + 128) << 8);
        ++level;
        //debug::toggleGreen();

        // sample buffer 1
        co_await buffer1.untilReadyOrDisabled();
        auto val1 = buffer1.pointer<Sample>()[SAMPLE_COUNT - 1];
        debug::setGreen((val1.x & 0x80) != 0);

        // output value on test DAC so that it can be measured
        testDac.set(0, val1.x << 8);
        testDac.set(1, val1.y << 8);

        // start buffer 1 again
        buffer1.startRead(SAMPLE_COUNT * sizeof(Sample));


        // set level to ADC input via generator DAC
        generatorDac.set(0, level << 8);
        generatorDac.set(1, int8_t(level + 128) << 8);
        ++level;
        debug::toggleRed();

        // wait for buffer 2
        co_await buffer2.untilReadyOrDisabled();
        auto val2 = buffer2.pointer<Sample>()[SAMPLE_COUNT - 1];
        debug::setGreen((val2.x & 0x80) != 0);

        // output value on test DAC so that it can be measured
        testDac.set(0, val2.x << 8);
        testDac.set(1, val2.y << 8);

        // start buffer 2 again
        buffer2.startRead(SAMPLE_COUNT * sizeof(Sample));
    }
}


int main() {
    debug::out << "DualAdcDeviceTest\n";

    // run test
    run(drivers.loop, drivers.generatorDac, drivers.buffer1, drivers.buffer2, drivers.testDac);

    drivers.loop.run();
    return 0;
}
