#pragma once

#include <coco/align.hpp>
#include <coco/BufferDevice.hpp>
#include <coco/AdcDevice.hpp>
#include <coco/platform/Loop_Queue.hpp>
#include <coco/platform/adc.hpp>
#include <coco/platform/dma.hpp>
#include <coco/platform/gpio.hpp>
#include <coco/platform/nvic.hpp>


namespace coco {

/// @brief Analog/digital converter with DMA transfer.
/// Uses circular DMA transfers, therefore only two buffers are supported.
/// The first must be an instance of Buffer1 and the second an instance of Buffer2. When calling start() on a buffer,
/// the transfer always starts with the first buffer.
///
/// Reference manual:
///   f0: https://www.st.com/resource/en/reference_manual/rm0091-stm32f0x1stm32f0x2stm32f0x8-advanced-armbased-32bit-mcus-stmicroelectronics.pdf
///   f3: https://www.st.com/resource/en/reference_manual/rm0364-stm32f334xx-advanced-armbased-32bit-mcus-stmicroelectronics.pdf
///   g4: https://www.st.com/resource/en/reference_manual/rm0440-stm32g4-series-advanced-armbased-32bit-mcus-stmicroelectronics.pdf
/// Resources:
///   ADCx
///   DMAx
class AdcDevice_ADC_DMA : public AdcDevice {
public:
    /// @brief Constructor of the ADC device.
    /// @param loop event loop
    /// @param analogPins analog pins
    /// @param adcInfo info of dual ADC to use
    /// @param dmaInfo info of DMA channel to use
    /// @param clockConfig clock configuration of ADC
    /// @param config configuration of ADC channels
    /// @param sequence input sequence, may have restrictions based on hardware
    /// @param trigger trigger index, see reference manual
    AdcDevice_ADC_DMA(Loop_Queue &loop, Array<const gpio::Config> analogPins,
        const adc::Info &adcInfo, const dma::Info<dma::Feature::CIRCULAR> &dmaInfo,
        adc::ClockConfig clockConfig, adc::Config config,
        Array<const adc::Input> sequence, adc::Trigger trigger);

#ifdef HAVE_ADC_DUAL_MODE
    /// @brief Constructor of the ADC device in dual mode.
    /// @param loop event loop
    /// @param analogPins analog pins
    /// @param adcInfo info of dual ADC to use
    /// @param dmaInfo info of DMA channel to use
    /// @param clockConfig clock configuration of both ADC channels
    /// @param config configuration of ADC channels
    /// @param sequence1 input sequence for first channel, may have restrictions based on hardware
    /// @param sequence2 input sequence for second channel, may have restrictions based on hardware
    /// @param trigger trigger index, see reference manual
    AdcDevice_ADC_DMA(Loop_Queue &loop, Array<const gpio::Config> analogPins,
        const adc::DualInfo &adcInfo, const dma::Info<dma::Feature::CIRCULAR> &dmaInfo,
        adc::ClockConfig clockConfig, adc::Config config,
        Array<const adc::Input> sequence1, Array<const adc::Input> sequence2, adc::Trigger trigger);
#endif

    ~AdcDevice_ADC_DMA() override;

    // internal buffer base class, derives from IntrusiveListNode for the list of active transfers and Loop_Queue::Handler to be notified from the event loop
    class BufferBase : public coco::Buffer, public IntrusiveListNode, public Loop_Queue::Handler {
        friend class AdcDevice_ADC_DMA;
        //friend class Buffer2;
    public:
        /// @brief Constructor
        /// @param headerCapacity capacity of the header
        /// @param data data of the buffer
        /// @param capacity capacity of the buffer
        /// @param channel channel to attach to
        BufferBase(uint8_t *data, int capacity, AdcDevice_ADC_DMA &device, int id);
        ~BufferBase() override;

        /// @brief Transfer the buffer contents from the ADC. The whole buffer gets transferred regardless of the current size.
        ///
        bool start(Op op) override;
        bool cancel() override;

    protected:
        void handle() override;

        AdcDevice_ADC_DMA &device_;
        int id_;
    };

    template <int C>
    class Buffer2;

    /// @brief Buffer for transferring data from the ADC.
    /// Internally uses circular mode, therefore only one instance of Buffer1 and Buffer2 are supported.
    /// @tparam C capacity of buffer, must be aligned to the sample size
    template <int C>
    class Buffer1 : public BufferBase {
        friend class Buffer2<C>;
    public:
        Buffer1(AdcDevice_ADC_DMA &device) : BufferBase(data_, C, device, 1) {}

    protected:
        // data for Buffer1 and Buffer2
        alignas(4) uint8_t data_[C * 2];
    };

    template <int C>
    class Buffer2 : public BufferBase {
    public:
        Buffer2(Buffer1<C> &buffer) : BufferBase(buffer.data_ + C, C, buffer.device_, 2) {}
    };


    // BufferDevice methods
    int getBufferCount();
    BufferBase &getBuffer(int index);

    // AdcDevice methods
	bool overrun() override;


    /// @brief Call from interrupt handler for the DMA channel (e.g. DMA1_Channel1_IRQHandler())
    ///
    void DMA_IRQHandler();

protected:
    void start();

    Loop_Queue &loop_;

    // ADC
    adc::Instance adc_;

    // DMA
    using DmaChannel = dma::Channel<dma::Mode::PERIPHERAL_TO_MEMORY | dma::Mode::CIRCULAR | dma::Mode::SOURCE_WIDTH_32 | dma::Mode::DESTINATION_DYNAMIC>;
    DmaChannel dmaChannel_;
    int dmaIrq_;

    // list of buffers
    BufferBase *buffers_[2];
    int bufferCount_ = 0;

    // queue of buffer id's of up to two active transfers
    std::atomic<int> queue_ = 0;
};

} // namespace coco
