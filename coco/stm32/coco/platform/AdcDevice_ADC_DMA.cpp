#include "AdcDevice_ADC_DMA.hpp"
#include <coco/debug.hpp>
#include <coco/bits.hpp>


namespace coco {

AdcDevice_ADC_DMA::AdcDevice_ADC_DMA(Loop_Queue &loop, Array<const gpio::Config> analogPins,
    const adc::Info &adcInfo, const dma::Info &dmaInfo, adc::ClockConfig clockConfig,
    adc::Config config, Array<const adc::Input> sequence, adc::Trigger trigger)
    : BufferDevice(State::READY)
    , loop(loop)
{
	// enable clocks (note two cycles wait time until peripherals can be accessed, see STM32G4 reference manual section 7.2.17)
    dmaInfo.rcc.enableClock();

    // configure pins as analog
    for (auto pin : analogPins) {
        gpio::configureAnalog(pin);
    }

    // configure ADC
    this->adc = adcInfo.configure(clockConfig, config, trigger, adc::DmaMode::CIRCULAR)
        .setSequence(sequence);

    // initialize DMA channel
    this->dmaStatus = dmaInfo.status();
    this->dmaChannel = dmaInfo.channel();
    this->dmaChannel.setPeripheralAddress(&adcInfo.adc->DR);
    this->dmaIrq = dmaInfo.irq;
    const int resolution = extract(config, adc::Config::RES_MASK);
    this->dmaShift = 1 - (resolution >> 1); // 8 or 16 bytes

	// map DMA to ADC
    adcInfo.map(dmaInfo);
}

#ifdef HAVE_ADC_DUAL_MODE
AdcDevice_ADC_DMA::AdcDevice_ADC_DMA(Loop_Queue &loop, Array<const gpio::Config> analogPins,
    const adc::Info2 &adcInfo, const dma::Info &dmaInfo, adc::ClockConfig clockConfig,
    adc::Config config, Array<const adc::Input> sequence1, Array<const adc::Input> sequence2, adc::Trigger trigger)
    : BufferDevice(State::READY)
    , loop(loop)
{
    // enable clocks (note two cycles wait time until peripherals can be accessed, see STM32G4 reference manual section 7.2.17)
    dmaInfo.rcc.enableClock();

    // configure pins as analog
    for (auto pin : analogPins) {
        gpio::configureAnalog(pin);
    }

    // configure ADC
    this->adc = adcInfo.configure(clockConfig, config, trigger, adc::DmaMode::CIRCULAR)
        .setSequence1(sequence1)
        .setSequence2(sequence2)
        .master();

    // initialize DMA channel
    this->dmaStatus = dmaInfo.status();
    this->dmaChannel = dmaInfo.channel();
    this->dmaChannel.setPeripheralAddress(&adcInfo.common->CDR);
    this->dmaIrq = dmaInfo.irq;
    const int resolution = extract(config, adc::Config::RES_MASK);
    this->dmaShift = 2 - (resolution >> 1); // 16 or 32 bytes

	// map DMA to ADC
    adcInfo.map(dmaInfo);
}
#endif

AdcDevice_ADC_DMA::~AdcDevice_ADC_DMA() {
}

int AdcDevice_ADC_DMA::getBufferCount() {
    return this->bufferCount;
}

AdcDevice_ADC_DMA::BufferBase &AdcDevice_ADC_DMA::getBuffer(int index) {
    return *this->buffers[index];
}

void AdcDevice_ADC_DMA::DMA_IRQHandler() {
    auto flags = this->dmaStatus.get();

    // check if read DMA has completed
    if ((flags & dma::Status::Flags::HALF_TRANSFER) != 0) {
        // clear interrupt flag
        this->dmaStatus.clear(dma::Status::Flags::HALF_TRANSFER);

        // stop DMA and ADC if buffer for second half is not ready
        if (!this->buffers[1]->active) {
            //adc::stop(this->adc1);
            this->adc.stop();
            this->dmaChannel.disable();
        }

        // end of transfer
        BufferBase *buffer = this->buffers[0];
        if (buffer->active) {
            buffer->active = false;
            this->loop.push(*buffer);
        }
    }
    if ((flags & dma::Status::Flags::TRANSFER_COMPLETE) != 0) {
        // clear interrupt flag
        this->dmaStatus.clear(dma::Status::Flags::TRANSFER_COMPLETE);

        // stop DMA and ADC if buffer for first half is not ready
        if (!this->buffers[0]->active) {
            //adc::stop(this->adc1);
            this->adc.stop();
            this->dmaChannel.disable();
        }

        // end of transfer
        BufferBase *buffer = this->buffers[1];
        if (buffer->active) {
            buffer->active = false;
            this->loop.push(*buffer);
        }
    }
}


// BufferBase

AdcDevice_ADC_DMA::BufferBase::BufferBase(uint8_t *data, int capacity, AdcDevice_ADC_DMA &device)
    : coco::Buffer(data, capacity, BufferBase::State::READY), device(device)
{
    assert(device.bufferCount < 2);
    device.buffers[device.bufferCount++] = this;
}

AdcDevice_ADC_DMA::BufferBase::~BufferBase() {
}

bool AdcDevice_ADC_DMA::BufferBase::start(Op op) {
    if (this->st.state != State::READY) {
        assert(this->p.state != State::BUSY);
        return false;
    }

    // check if READ flag is set
    assert((op & Op::READ) != 0);

    auto &device = this->device;

    nvic::disable(device.dmaIrq);
    this->active = true;

    // start if DMA is stopped
    if (!device.dmaChannel.enabled())
        start();

    nvic::enable(device.dmaIrq);

    // set state
    setBusy();

    return true;
}

bool AdcDevice_ADC_DMA::BufferBase::cancel() {
    if (this->st.state != State::BUSY)
        return false;

    // always complete normally
    return true;
}

void AdcDevice_ADC_DMA::BufferBase::start() {
    auto &device = this->device;

    // always start first buffer
    auto &buffer = *device.buffers[0];

    int dmaShift = device.dmaShift;
    auto data = buffer.p.data;
    int count = (buffer.p.capacity >> dmaShift) * 2;

    // configure DMA
    device.dmaChannel.setMemoryAddress(data);
    device.dmaChannel.setCount(count);
	device.dmaChannel.enable(dma::Channel::Config::RX
		| dma::Channel::Config::PERIPHERAL_SIZE_32
		| dma::Channel::Config::HALF_TRANSFER_INTERRUPT
		| dma::Channel::Config::TRANSFER_COMPLETE_INTERRUPT
		| dma::Channel::Config::CIRCULAR,
		dmaShift);

    // start ADC
    device.adc.start();

    // -> DMAx_IRQHandler()
}

void AdcDevice_ADC_DMA::BufferBase::handle() {
    auto &buffer = *device.buffers[0];
    int dmaShift = device.dmaShift;
    int transferred = (buffer.p.capacity >> dmaShift) << dmaShift;

    setReady(transferred);
}

} // namespace coco
