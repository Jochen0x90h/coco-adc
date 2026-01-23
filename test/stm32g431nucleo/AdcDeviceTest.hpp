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

// ADC pins
const gpio::Config adcPins[] = {
	gpio::Config::PA0, // ADC12_IN1 (CN8 1)
	gpio::Config::PA1 // ADC12_IN2 (CN8 2)
};

// ADC input sequence
using Input = adc::Input;
const Input sequence[] = {Input::CH1 | Input::CYCLES_6_5};

// DAC1 pins
const gpio::Config dacPins[] = {
	gpio::Config::PA4, // channel 1 (CN8 3)
	gpio::Config::PA5 // channel 2 (CN5 6)
};


///
/// Drivers for AdcDeviceTest
/// Connect DAC channel 1 to ADC (CN8 3 -> CN8 1)
///
struct Drivers {
	Loop_TIM2 loop{APB1_TIMER_CLOCK};

	// ADC
	using Adc = AdcDevice_ADC_DMA;
	Adc adc{loop,
		adcPins,
		adc::ADC1_INFO,
		dma::DMA1_CH1_INFO,
		//Adc::ClockConfig::AHB_DIV4, // 42MHz, see board/config.hpp
		adc::ClockConfig::RCC_DIV1, // 48MHz, see systemInit()
		FORMAT,
		sequence,
		adc::Trigger::ADC12_TIM8_TRGO}; // triggered by TIM8
	Adc::Buffer1<2 * SAMPLE_COUNT * sizeof(Sample)> buffer1{adc};
	Adc::Buffer2 buffer2{buffer1};

	// DAC for generating analog values that are measured by the ADC
	using Dac = Dac_DAC;
	Dac dac{dacPins,
		dac::DAC1_INFO,
		AHB_CLOCK,
		dac::Config::DUAL | dac::Config::BUFFERED_EXTERNAL}; // DAC1 directly goes to pins

	Drivers() {
		// configure TIM8 as trigger for the ADC that runs at 1kHz
		timer::TIM8_INFO.configure(APB2_TIMER_CLOCK, 1kHz).setMaster(timer::MasterMode::UPDATE).start();
	}
};

Drivers drivers;

extern "C" {
void DMA1_Channel1_IRQHandler() {
	drivers.adc.DMA_IRQHandler();
}
}
