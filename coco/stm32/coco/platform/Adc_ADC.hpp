#pragma once

#include <coco/Adc.hpp>
#include <coco/platform/Loop_Queue.hpp>
#include <coco/platform/adc.hpp>
#include <coco/platform/gpio.hpp>


namespace coco {

/**
 * Implementation of simple Analog/digital converter interface.
 *
 * Reference manual:
 *   f0:
 *     https://www.st.com/resource/en/reference_manual/rm0091-stm32f0x1stm32f0x2stm32f0x8-advanced-armbased-32bit-mcus-stmicroelectronics.pdf
 *       ADC: Section 13
 *   f3:
 *     https://www.st.com/resource/en/reference_manual/rm0364-stm32f334xx-advanced-armbased-32bit-mcus-stmicroelectronics.pdf
 *       ADC: Section 13
 *   g4:
 *     https://www.st.com/resource/en/reference_manual/rm0440-stm32g4-series-advanced-armbased-32bit-mcus-stmicroelectronics.pdf
 *       ADC: Section 21
 * Resources:
 *   ADCx
 */
class Adc_ADC : public Adc {
public:
    ///
    /// Constructor of the ADC wrapper.
    /// @param analogPins analog pins
    /// @param adcInfo info of ADC to use
    /// @param clockConfig clock configuration of ADC
    /// @param config configuration of ADC
    /// @param inputs input to use for the channels
    ///
    Adc_ADC(Array<const gpio::Config> analogPins, const adc::Info &adcInfo,
        adc::ClockConfig clockConfig, adc::Config config, Array<const adc::Input> inputs);

    ~Adc_ADC() override;

    int get(int channel) override;

protected:

    // ADC
    adc::Instance adc_;
    Array<const adc::Input> inputs_;
};

} // namespace coco
