#include "AdcDevice_ADC_DMA.hpp"
#include <coco/debug.hpp>
#include <coco/bits.hpp>


namespace coco {

AdcDevice_ADC_DMA::AdcDevice_ADC_DMA(Loop_Queue &loop, const adc::Info &adcInfo, Array<const gpio::Config> analogPins,
    const dma::Info<dma::Feature::CIRCULAR> &dmaInfo, adc::ClockConfig clockConfig, adc::Config config,
    Array<const adc::Input> sequence, adc::Trigger trigger)
    : AdcDevice(State::READY)
    , loop_(loop)
{
    // configure pins as analog
    for (auto pin : analogPins) {
        gpio::enableAnalog(pin);
    }

    // configure ADC
    adc_ = adcInfo.enableClock(clockConfig)
        .calibrate()
        .enable(config, trigger, adc::DmaMode::CIRCULAR)
        .setSequence(sequence);


    // initialize DMA channel
    int destinationShift = adc::getResolution(config);
    dmaChannel_ = dmaInfo.enableClock<DmaChannel::MODE>()
        .configure(destinationShift, dma::Destination::INCREMENT)
        .setSourceAddress(&adcInfo.adc->DR);
    dmaIrq_ = dmaInfo.irq;

	// map DMA to ADC
    adcInfo.map(dmaInfo);
}

#ifdef HAVE_ADC_DUAL_MODE
AdcDevice_ADC_DMA::AdcDevice_ADC_DMA(Loop_Queue &loop, const adc::DualInfo &adcInfo,
    Array<const gpio::Config> analogPins,
    const dma::Info<dma::Feature::CIRCULAR> &dmaInfo, adc::ClockConfig clockConfig, adc::Config config,
    Array<const adc::Input> sequence1, Array<const adc::Input> sequence2, adc::Trigger trigger)
    : AdcDevice(State::READY)
    , loop_(loop)
{
    // configure pins as analog
    for (auto pin : analogPins) {
        gpio::enableAnalog(pin);
    }

    // configure ADC
    adc_ = adcInfo.enableClock(clockConfig)
        .calibrate()
        .enable(config, trigger, adc::DmaMode::CIRCULAR)
        .setSequence1(sequence1)
        .setSequence2(sequence2)
        [0]; // master


    // initialize DMA channel
    int destinationShift = 1 + adc::getResolution(config); // 16 or 32 bits
    dmaChannel_ = dmaInfo.enableClock<DmaChannel::MODE>()
        .configure(destinationShift, dma::Destination::INCREMENT)
        .setSourceAddress(&adcInfo.common->CDR);
    dmaIrq_ = dmaInfo.irq;
    nvic::setPriority(dmaIrq_, nvic::Priority::MEDIUM); // interrupt gets enabled in first call to start()

	// map DMA to ADC
    adcInfo.map(dmaInfo);
}
#endif

AdcDevice_ADC_DMA::~AdcDevice_ADC_DMA() {
}

int AdcDevice_ADC_DMA::getBufferCount() {
    return bufferCount_;
}

AdcDevice_ADC_DMA::BufferBase &AdcDevice_ADC_DMA::getBuffer(int index) {
    return *buffers_[index];
}

bool AdcDevice_ADC_DMA::overrun() {
#if defined(ADC_SR_OVR) || defined(ADC_ISR_OVR)
    bool ovr = (adc_.status() & adc::Status::OVERRUN) != 0;
    adc_.clear(adc::Status::OVERRUN);
    return ovr;
#else
    return false;
#endif
}

void AdcDevice_ADC_DMA::start() {
    // always start with first buffer
    auto &buffer = *buffers_[0];

    volatile uint8_t *data = buffer.data_;
    int size = buffer.capacity_ * 2;

    // configure DMA
    dmaChannel_.setDestinationAddress(data)
        .setDestinationSize(size)
        .enable(dma::Config::HALF_TRANSFER_INTERRUPT
            | dma::Config::TRANSFER_COMPLETE_INTERRUPT);

    // start ADC
    adc_.start();

    // -> DMAx_IRQHandler()
}

void AdcDevice_ADC_DMA::DMA_IRQHandler() {
    auto flags = dmaChannel_.status() & (dma::Status::HALF_TRANSFER | dma::Status::TRANSFER_COMPLETE);

    if (flags != 0) {
        int queue = queue_;

        // stop DMA and ADC if no more pending transfers
        if ((queue & 0xf0) == 0) {
            adc_.stop();
            dmaChannel_.disable();
        }

        // clear interrupt flag
        dmaChannel_.clear(flags);

        // end of transfer
        int bufferIndex = (flags & dma::Status::HALF_TRANSFER) != 0 ? 0 : 1;
        BufferBase *buffer = buffers_[bufferIndex];
        if (buffer->id_ == (queue & 0x0f)) {
            // pop queue
            queue_ = queue >> 4;

            // hand over to event loop (which calls BufferBase::handle())
            loop_.push(*buffer);
        }
    }
}


// AdcDevice_ADC_DMA::BufferBase

AdcDevice_ADC_DMA::BufferBase::BufferBase(uint8_t *data, int capacity, AdcDevice_ADC_DMA &device, int id)
    : coco::Buffer(data, capacity, BufferBase::State::READY), device_(device), id_(id)
{
    assert(device.bufferCount_ < 2);
    device.buffers_[device.bufferCount_++] = this;
}

AdcDevice_ADC_DMA::BufferBase::~BufferBase() {
}

bool AdcDevice_ADC_DMA::BufferBase::start() {
    if (state_ != State::READY || (op_ & Op::READ) == 0 || size_ == 0) {
        // starting a buffer when the state is BUSY is a bug
        assert(state_ != State::BUSY);
        return false;
    }

    auto &device = device_;

    nvic::disable(device.dmaIrq_);

    int queue = device.queue_;
    int i = (queue & 0x0f) == 0 ? 0 : 4;
    device.queue_ = queue | (id_ << i);

    // start if DMA is stopped
    if (!device.dmaChannel_.enabled())
        device.start();

    nvic::enable(device.dmaIrq_);

    // set state
    setBusy();

    return true;
}

bool AdcDevice_ADC_DMA::BufferBase::cancel() {
    if (state_ != State::BUSY)
        return false;

    // not supported, always complete normally
    return true;
}

void AdcDevice_ADC_DMA::BufferBase::onCompletion() {
    // always transfers full capacity
    setSuccess(capacity_);
    setReady();
}

} // namespace coco
