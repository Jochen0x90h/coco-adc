#pragma once

#include <coco/platform/Loop_TIM2.hpp>
#include <coco/platform/AdcDevice_ADC_DMA.hpp>
#include <coco/platform/Dac_DAC.hpp>
#include <coco/platform/timer.hpp>
#include <coco/board/config.hpp>


using namespace coco;

using Sample = uint8_t;
constexpr auto FORMAT = adc::Config::RES_8;
constexpr int SAMPLE_COUNT = 16;

// DAC1 pins for generator (channel 1) and test (channel 2)
const gpio::Config dacPins[] = {
    gpio::PA4, // channel 1 (CN8 3)
    gpio::PA5 // channel 2 (CN5 6) Note: green LED is connected to this pin, therefore debug::setGreen() etc. does not work
};

// ADC pins
const gpio::Config adcPins[] = {
    gpio::PA0, // ADC12_IN1 (CN8 1)
};

// ADC input sequence
const adc::Input adcInput[] = {adc::Input::CH1 | adc::Input::CYCLES_6_5};



/// @brief Drivers for AdcDeviceTest
/// Connect DAC channel 0 to ADC (CN8 3 -> CN8 1)
/// Connect DAC channel 1 to oscilloscope (CN5 6)
struct Drivers {
    Loop_TIM2 loop{APB1_TIMER_CLOCK};

    // ADC
    using Adc = AdcDevice_ADC_DMA;
    Adc adc{loop,
        adcPins,
        adc::ADC1_INFO,
        dma::DMA1_CH1_INFO,
        //Adc::ClockConfig::AHB_DIV4, // 42MHz, see board/config.hpp
        adc::ClockConfig::RCC_DIV_1, // 48MHz, see systemInit()
        FORMAT,
        adcInput,
        adc::Trigger::ADC12_TIM8_TRGO}; // triggered by TIM8
    Adc::Buffer1<SAMPLE_COUNT * sizeof(Sample)> buffer1{adc};
    Adc::Buffer2<SAMPLE_COUNT * sizeof(Sample)> buffer2{buffer1};

    // DAC for generating analog values that are measured by the ADC
    using Dac = Dac_DAC;
    Dac dac{dacPins,
        dac::DAC1_INFO,
        AHB_CLOCK,
        dac::DualConfig::BUFFERED_EXTERNAL}; // DAC1 directly goes to pins

    Drivers() {
        // configure TIM8 as trigger for the ADC that runs at 1kHz
        timer::TIM8_INFO.enableClock()
            .setUpdateFrequency(APB2_TIMER_CLOCK, 100kHz)
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
