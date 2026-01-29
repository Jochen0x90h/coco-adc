#pragma once

#include <coco/DacDummy.hpp>
#include <coco/platform/Loop_SysTick.hpp>
#include <coco/platform/Adc_ADC.hpp>
#include <coco/platform/Dac_DAC.hpp>
#include <coco/board/config.hpp>


using namespace coco;

using Sample = uint8_t;
constexpr auto FORMAT = adc::Config::RES_12 | adc::Config::ALIGN_LEFT;

// DAC1 pins for generator (generates input to the ADC)
const gpio::Config dac1Pins[] = {
    gpio::PA4, // channel 1 (CN8 3)
    gpio::PA5 // channel 2 (CN5 6) Note: green LED is connected to this pin, therefore debug::setGreen() etc. does not work
};

// ADC pins
const gpio::Config adcPins[] = {
    gpio::PA0, // ADC12_IN1 (CN8 1)
    gpio::PA1 // ADC12_IN2 (CN8 2)
};

// ADC inputs
const adc::Input adcInputs[] = {adc::Input::CH0 | adc::Input::CYCLES_7_5, adc::Input::CH1 | adc::Input::CYCLES_7_5};


/// @brief Drivers for AdcTest
/// Connect DAC1 channel 1 to ADC channel 1 (CN8 3 -> CN8 1)
/// Connect DAC1 channel 2 to ADC channel 2 (CN5 6 -> CN8 2)
/// Connect DAC3 channel 1/2 to oscilloscope (CN9 2, CN10 24) Note: This is not the default function, change SB17 and SB18
struct Drivers {
    Loop_SysTick loop{AHB_CLOCK};

    // ADC
    using Adc = Adc_ADC;
    Adc adc{adcPins,
        adc::ADC1_INFO,
        adc::ClockConfig::RCC_DIV_1, // 45.7MHz, see systemInit()
        FORMAT,
        adcInputs};

    // DAC for generating analog values that are measured by the ADC
    using Dac = Dac_DAC;
    Dac generatorDac{dac1Pins,
        dac::DAC1_INFO,
        dac::DualConfig::BUFFERED_EXTERNAL}; // DAC1 directly goes to pins

    // no second DAC for monitoring
    DacDummy testDac;
};

Drivers drivers;
