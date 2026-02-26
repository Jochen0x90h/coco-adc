#pragma once

#include <coco/Vector2.hpp>
#include <coco/platform/Loop_TIM2.hpp>
#include <coco/platform/AdcDevice_ADC_DMA.hpp>
#include <coco/platform/Dac_DAC.hpp>
#include <coco/platform/opamp.hpp>
#include <coco/platform/timer.hpp>
#include <coco/board/config.hpp>


using namespace coco;

using Sample = Vector2<uint8_t>;
constexpr auto FORMAT = adc::Config::RES_8;
constexpr int SAMPLE_COUNT = 16;

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
const adc::Input adcInput1[] = {adc::Input::CH1 | adc::Input::CYCLES_6_5};
const adc::Input adcInput2[] = {adc::Input::CH2 | adc::Input::CYCLES_6_5};

// DAC3 pins for test
const gpio::Config dac3Pins[] = {
    gpio::PB11, // channel 1, OPAMP6 (CN10 18)
    gpio::PB1 // channel 2, OPAMP3 (CN10 24)
};



/// @brief Drivers for AdcDeviceTest
/// Make sure the VREF jumper is at default position (1-2)
/// Connect DAC1 channel 1 to ADC channel 1 (CN8 3 -> CN8 1)
/// Connect DAC1 channel 2 to ADC channel 2 (CN5 6 -> CN8 2)
/// Connect DAC3 channel 1/2 to oscilloscope (CN10 18, CN10 24)
struct Drivers {
    Loop_TIM2 loop{APB1_TIMER_CLOCK};

    // ADC
    using Adc = AdcDevice_ADC_DMA;
    Adc adc{loop,
        adcPins,
        adc::ADC12_INFO,
        dma::DMA1_CH1_INFO,
        //Adc::ClockConfig::AHB_DIV_4,  // 40MHz, see board/config.hpp
        adc::ClockConfig::RCC_DIV_1, // 45.7MHz, see systemInit()
        FORMAT,
        adcInput1, adcInput2,
        adc::Trigger::ADC12_TIM8_TRGO}; // triggered by TIM8
    Adc::Buffer1<SAMPLE_COUNT * sizeof(Sample)> buffer1{adc};
    Adc::Buffer2<SAMPLE_COUNT * sizeof(Sample)> buffer2{buffer1};

    // DAC for generating analog values that are measured by the ADC
    using Dac = Dac_DAC;
    Dac generatorDac{dac1Pins,
        dac::DAC1_INFO,
        AHB_CLOCK,
        dac::DualConfig::BUFFERED_EXTERNAL}; // DAC1 directly goes to pins
    Dac testDac{dac3Pins,
        dac::DAC3_INFO,
        AHB_CLOCK,
        dac::DualConfig::INTERNAL}; // DAC3 is internally connected to op-amps

    Drivers() {
        // enable opamps for DAC3
        opamp::enable(OPAMP6, opamp::Config::OUT_PIN | opamp::Config::OPAMP6_INP_DAC3_CH1 | opamp::Config::INM_FOLLOWER);
        opamp::enable(OPAMP3, opamp::Config::OUT_PIN | opamp::Config::OPAMP3_INP_DAC3_CH2 | opamp::Config::INM_FOLLOWER);

        // configure TIM8 as trigger for the ADC that runs at 10kHz
        timer::TIM8_INFO.enableClock()
            .setUpdateFrequency(APB2_TIMER_CLOCK, 10kHz)
            .setMasterMode(timer::MasterMode::UPDATE)
            .start();
    }
};

Drivers drivers;

extern "C" {
void DMA1_Channel1_IRQHandler() {
    drivers.adc.DMA_IRQHandler();
}
}
