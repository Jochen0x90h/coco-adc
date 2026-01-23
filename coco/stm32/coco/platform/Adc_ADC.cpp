#include "Adc_ADC.hpp"
#include <coco/debug.hpp>
#include <coco/bits.hpp>


namespace coco {

Adc_ADC::Adc_ADC(Array<const gpio::Config> analogPins, const adc::Info &adcInfo,
    adc::ClockConfig clockConfig, adc::Config config, Array<const adc::Input> inputs)
    : inputs(inputs)
{
    // configure pins as analog
    for (auto pin : analogPins) {
        gpio::configureAnalog(pin);
    }

    // configure ADC
    this->adc = adcInfo.configure(clockConfig,
        config,
        adc::Trigger::SOFTWARE);
}

Adc_ADC::~Adc_ADC() {
}

int Adc_ADC::get(int channel) {
    if (channel < 0 || channel >= this->inputs.size())
        return 0;

    // select input and start conversion
    return this->adc.setInput(this->inputs[channel])
        .start()
        .wait()
        .data();
}

} // namespace coco
