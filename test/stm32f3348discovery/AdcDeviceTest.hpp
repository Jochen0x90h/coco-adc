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
    gpio::PA4, // channel 1 (PA4)
    gpio::PA5 // channel 2 (PA5)
};

// ADC pins
const gpio::Config adcPins[] = {
    gpio::PA0, // ADC12_IN1 (PA0)
    gpio::PA1 // ADC12_IN2 (PA1)
};

// ADC input
const adc::Input adcInput[] = {adc::Input::CH1 | adc::Input::CYCLES_7_5};



/// @brief Drivers for AdcDeviceTest
/// Connect DAC channel 1 to ADC (PA4 -> PA0)
/// Connect DAC channel 1 to oscilloscope (PA5)
struct Drivers {
    Loop_TIM2 loop{APB1_TIMER_CLOCK};

    // ADC
    using Adc = AdcDevice_ADC_DMA;
    Adc adc{loop,
        adc::ADC1_INFO,
        adcPins,
        dma::DMA1_CH1_INFO,
        //adc::ClockConfig::CLOCK_AHB_DIV4, // 10MHz
        adc::ClockConfig::RCC_DIV_1, // 40MHz, see systemInit()
        FORMAT,
        adcInput,
        adc::Trigger::ADC12_TIM1_TRGO}; // triggered by TIM1
    Adc::Buffer1<SAMPLE_COUNT * sizeof(Sample)> buffer1{adc};
    Adc::Buffer2<SAMPLE_COUNT * sizeof(Sample)> buffer2{buffer1};

    // DAC for generating analog values that are measured by the ADC
    using Dac = Dac_DAC;
    Dac dac{
        dac::DAC1_INFO,
        dacPins,
        dac::DualConfig::CH2_OUTPUT_ENABLE};

    Drivers() {
        // configure TIM1 as trigger for the ADC that runs at 1kHz
        timer::TIM1_INFO.enableClock()
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
