#pragma once

#include <coco/platform/Loop_SysTick.hpp>
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
const adc::Input adcInput[] = {adc::Input::CH0 | adc::Input::CYCLES_7_5};



/// @brief Drivers for AdcDeviceTest
/// Make sure SB28 is closed to provice VREF+ (is default)
/// Connect DAC channel 0 to ADC (CN8 3 -> CN8 1)
/// Connect DAC channel 1 to oscilloscope (CN5 6)
struct Drivers {
    Loop_SysTick loop{AHB_CLOCK};

    // ADC
    using Adc = AdcDevice_ADC_DMA;
    Adc adc{loop,
        adc::ADC1_INFO,
        adcPins,
        dma::DMA1_CH1_INFO,
        //Adc::ClockConfig::AHB_DIV_4, // 42MHz, see board/config.hpp
        adc::ClockConfig::RCC_DIV_1, // 48MHz, see systemInit()
        FORMAT,
        adcInput,
        adc::Trigger::ADC1_TIM3_TRGO}; // triggered by TIM8
    Adc::Buffer1<SAMPLE_COUNT * sizeof(Sample)> buffer1{adc};
    Adc::Buffer2<SAMPLE_COUNT * sizeof(Sample)> buffer2{buffer1};

    // DAC for generating analog values that are measured by the ADC
    using Dac = Dac_DAC;
    Dac dac{
        dac::DAC1_INFO,
        dacPins,
        dac::DualConfig::BUFFERED_EXTERNAL}; // DAC1 directly goes to pins

    Drivers() {
        // configure TIM3 as trigger for the ADC that runs at 1kHz
        timer::TIM3_INFO.enableClock()
            .setUpdateFrequency(APB_TIMER_CLOCK, 100kHz)
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
