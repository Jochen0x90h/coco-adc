#pragma once

#include <coco/DacDummy.hpp>
#include <coco/platform/Loop_TIM2.hpp>
#include <coco/platform/AdcDevice_ADC_DMA.hpp>
#include <coco/platform/timer.hpp>
#include <coco/board/config.hpp>


using namespace coco;

using Sample = uint8_t;
constexpr auto FORMAT = adc::Config::RES_8;
constexpr int SAMPLE_COUNT = 16;

// analog pins
const gpio::Config adcPins[] = {
	gpio::Config::PA0, // ADC12_IN1 (MEAS_UENC)
	gpio::Config::PA1 // ADC12_IN2 (MEAS_IENC)
};

// input sequence
using Input = adc::Input;
const Input sequence[] = {Input::CH1 | Input::CYCLES_6_5};


/**
 * Drivers for AdcDeviceTest
 */
struct Drivers {
	Loop_TIM2 loop{APB1_TIMER_CLOCK};

	// ADC
	using Adc = AdcDevice_ADC_DMA;
	Adc adc{loop,
		adcPins,
		adc::ADC1_INFO,
		dma::DMA1_CH1_INFO,
		//adc::ClockConfig::AHB_DIV4, // 40MHz
		adc::ClockConfig::RCC_DIV1, // 40MHz
		FORMAT,
		sequence,
		adc::Trigger::ADC12_TIM8_TRGO};// triggered by TIM8
	Adc::Buffer1<2 * SAMPLE_COUNT * sizeof(Sample)> buffer1{adc};
	Adc::Buffer2 buffer2{buffer1};

	// dummy DAC
	using Dac = DacDummy;
	Dac dac;

	Drivers() {
		// configure TIM8 as trigger for the ADC that runs at 1kHz
		timer::TIM8_INFO.configureTrigger(APB2_TIMER_CLOCK, 1kHz).start();
	}
};

Drivers drivers;

extern "C" {
void DMA1_Channel1_IRQHandler() {
	drivers.adc.DMA_IRQHandler();
}
}
