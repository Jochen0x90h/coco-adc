#pragma once

#include <coco/platform/Loop_TIM2.hpp>
#include <coco/platform/Adc_ADC.hpp>
#include <coco/platform/Dac_DAC.hpp>
#include <coco/platform/opamp.hpp>
#include <coco/board/config.hpp>


using namespace coco;

using Sample = uint8_t;
constexpr auto FORMAT = adc::Config::RES_8 | adc::Config::ALIGN_LEFT;

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
const adc::Input adcInputs[] = {adc::Input::CH1 | adc::Input::CYCLES_6_5, adc::Input::CH2 | adc::Input::CYCLES_6_5};

// DAC3 pins for test
const gpio::Config dac3Pins[] = {
    //gpio::PA2, // channel 1 via OPAMP1 (CN9 2) Note: Is VCP_TX by default, change SB17 and SB18
    gpio::PB1 // channel 2 via OPAMP3 (CN10 24)
};


/// @brief Drivers for AdcTest
/// Make sure the VREF jumper is at default position (1-2)
/// Connect DAC1 channel 1 to ADC channel 1 (CN8 3 -> CN8 1)
/// Connect DAC1 channel 2 to ADC channel 2 (CN5 6 -> CN8 2)
/// Connect DAC3 channel 1/2 to oscilloscope (CN9 2, CN10 24) Note: PA2 (CN9 2) is VCP_TX by default, change SB17 and SB18
struct Drivers {
    Loop_TIM2 loop{APB1_TIMER_CLOCK};

    // ADC
    using Adc = Adc_ADC;
    Adc adc{
        adc::ADC1_INFO,
        adcPins,
        adc::ClockConfig::RCC_DIV_1, // 45.7MHz, see systemInit()
        FORMAT,
        adcInputs};

    // DAC for generating analog values that are measured by the ADC
    using Dac = Dac_DAC;
    Dac generatorDac{
        dac::DAC1_INFO,
        dac1Pins,
        AHB_CLOCK,
        dac::DualConfig::BUFFERED_EXTERNAL}; // DAC1 directly goes to pins

    // second DAC for monitoring
    Dac testDac{
        dac::DAC3_INFO,
        dac3Pins,
        AHB_CLOCK,
        1, dac::Config::INTERNAL}; // DAC3 is internally connected to op-amps
        //dac::DualConfig::INTERNAL}; // DAC3 is internally connected to op-amps

    Drivers() {
        // enable opamps for DAC3
        //opamp::enable(OPAMP1, opamp::Config::OUT_PIN | opamp::Config::OPAMP1_INP_DAC3_CH1 | opamp::Config::INM_FOLLOWER);
        opamp::enable(OPAMP3, opamp::Config::OUT_PIN | opamp::Config::OPAMP3_INP_DAC3_CH2 | opamp::Config::INM_FOLLOWER);
    }
};

Drivers drivers;
