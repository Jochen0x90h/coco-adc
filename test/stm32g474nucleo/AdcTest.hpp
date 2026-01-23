#pragma once

#include <coco/platform/Loop_TIM2.hpp>
#include <coco/platform/Adc_ADC.hpp>
#include <coco/platform/Dac_DAC.hpp>
#include <coco/platform/opamp.hpp>
#include <coco/board/config.hpp>


using namespace coco;

using Sample = uint8_t;
constexpr auto FORMAT = adc::Config::RES_8 | adc::Config::ALIGN_LEFT;

// ADC pins
const gpio::Config adcPins[] = {
    gpio::Config::PA0, // ADC12_IN1 (CN8 1)
    gpio::Config::PA1 // ADC12_IN2 (CN8 2)
};

// ADC inputs
const adc::Input adcInputs[] = {adc::Input::CH1 | adc::Input::CYCLES_6_5, adc::Input::CH2 | adc::Input::CYCLES_6_5};

// DAC3 pins
const gpio::Config dac3Pins[] = {
    gpio::Config::PB11, // channel 1 (CN10 18)
    gpio::Config::PB1 // channel 2 (CN10 24)
};

// DAC1 pins
const gpio::Config dac1Pins[] = {
	gpio::Config::PA4, // channel 1 (CN8 3)
	gpio::Config::PA5 // channel 2 (CN5 6) Note: green LED is connected to this pin, therefore debug::setGreen() etc. does not work
};


///
/// Drivers for AdcTest
/// Connect DAC3 channel 1 to ADC channel 1 (CN10 18 -> CN8 1)
/// Connect DAC3 channel 2 to ADC channel 2 (CN10 24 -> CN8 2)
/// Connect DAC1 channel 1/2 to oscilloscope (CN8 3, CN5 6)
///
struct Drivers {
    Loop_TIM2 loop{APB1_TIMER_CLOCK};

    // ADC
    using Adc = Adc_ADC;
    Adc adc{adcPins,
        adc::ADC1_INFO,
        adc::ClockConfig::RCC_DIV1, // 45.7MHz, see systemInit()
        FORMAT,
        adcInputs};

    // DAC for generating analog values that are measured by the ADC
    using Dac = Dac_DAC;
    Dac generatorDac{dac3Pins,
        dac::DAC3_INFO,
        AHB_CLOCK,
        dac::Config::DUAL | dac::Config::INTERNAL}; // DAC3 is internally connected to op-amps
    Dac testDac{dac1Pins,
        dac::DAC1_INFO,
        AHB_CLOCK,
        dac::Config::DUAL | dac::Config::BUFFERED_EXTERNAL}; // DAC1 directly goes to pins

    Drivers() {
        // enable opamps for DAC3
        opamp::configure(OPAMP3, opamp::Config::OUT_PIN | opamp::Config::OPAMP3_INP_DAC3_CH2 | opamp::Config::INM_FOLLOWER);
        opamp::configure(OPAMP6, opamp::Config::OUT_PIN | opamp::Config::OPAMP6_INP_DAC3_CH1 | opamp::Config::INM_FOLLOWER);
    }
};

Drivers drivers;
