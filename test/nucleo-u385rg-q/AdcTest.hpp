#pragma once

#include <coco/DacDummy.hpp>
#include <coco/platform/Loop_TIM2.hpp>
#include <coco/platform/Adc_ADC.hpp>
#include <coco/platform/Dac_DAC.hpp>
#include <coco/platform/opamp.hpp>
#include <coco/board/config.hpp>


using namespace coco;

using Sample = uint8_t;
constexpr auto FORMAT = adc::Config::RES_8 ;//| adc::Config::ALIGN_LEFT;

// DAC1 pins for generator (generates input to the ADC)
const gpio::Config dac1Pins[] = {
    gpio::PA4, // channel 1 (CN8 3)
    gpio::PA5 // channel 2 (CN5 5) Note: green LED is connected to this pin, therefore debug::setGreen() etc. does not work
};

// ADC pins
const gpio::Config adcPins[] = {
    gpio::PC0, // ADC1_IN1 (CN8 6)
    gpio::PC1 // ADC1_IN2 (CN8 5)
};

// ADC inputs
const adc::Input adcInputs[] = {adc::Input::CH1 | adc::Input::CYCLES_6_5, adc::Input::CH2 | adc::Input::CYCLES_6_5};


/// @brief Drivers for AdcTest
/// Make sure the VREF jumper is at default position (1-2)
/// Connect DAC1 channel 1 to ADC channel 1 (CN8 3 -> CN8 6)
/// Connect DAC1 channel 2 to ADC channel 2 (CN5 5 -> CN8 5)
/// No test DAC, therefore check the debug console
struct Drivers {
    Loop_TIM2 loop{APB1_TIMER_CLOCK};

    // ADC
    using Adc = Adc_ADC;
    Adc adc{
        adc::ADC1_INFO,
        adcPins,
        adc::ClockConfig::RCC_DIV_1, // 48MHz, see systemInit()
        FORMAT,
        adcInputs};

    // DAC for generating analog values that are measured by the ADC
    using Dac = Dac_DAC;
    Dac generatorDac{
        dac::DAC1_INFO,
        dac1Pins,
        AHB_CLOCK,
        dac::DualConfig::BUFFERED_EXTERNAL}; // DAC1 directly goes to pins
    DacDummy testDac; // no test DAC, therefore dummy

    Drivers() {
    }
};

Drivers drivers;
