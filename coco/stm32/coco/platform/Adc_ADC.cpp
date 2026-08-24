#include "Adc_ADC.hpp"
#include <coco/debug.hpp>
#include <coco/bits.hpp>


namespace coco {

Adc_ADC::Adc_ADC(const adc::Info &adcInfo, Array<const gpio::Config> analogPins,
    adc::ClockConfig clockConfig, adc::Config config, Array<const adc::Input> inputs)
    : inputs_(inputs)
{
    // configure pins as analog
    for (auto pin : analogPins) {
        gpio::enableAnalog(pin);
    }

    // configure ADC
    adc_ = adcInfo.enableClock(clockConfig)
        .calibrate();

    for (int i = 0; i < 8; ++i)
        __NOP();
    __NOP();
    __NOP();
    adc_.enable(config, adc::Trigger::SOFTWARE);
}

Adc_ADC::~Adc_ADC() {
}

int Adc_ADC::get(int channel) {
    if (channel < 0 || channel >= inputs_.size())
        return 0;

    // select input, start conversion and get data
    return adc_.setInput(inputs_[channel])
        .start()
        .wait()
        .data();
}

} // namespace coco
