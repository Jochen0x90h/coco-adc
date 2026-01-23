#pragma once

#include <coco/align.hpp>
#include <coco/BufferDevice.hpp>
#include <coco/platform/Loop_Queue.hpp>
#include <coco/platform/adc.hpp>
#include <coco/platform/dma.hpp>
#include <coco/platform/gpio.hpp>
#include <coco/platform/nvic.hpp>


namespace coco {

/**
 * Dual Analog/digital converter with DMA transfer.
 *
 * Reference manual:
 *   f0:
 *     https://www.st.com/resource/en/reference_manual/rm0091-stm32f0x1stm32f0x2stm32f0x8-advanced-armbased-32bit-mcus-stmicroelectronics.pdf
 *       ADC: Section 13
 *   f3:
 *     https://www.st.com/resource/en/reference_manual/rm0364-stm32f334xx-advanced-armbased-32bit-mcus-stmicroelectronics.pdf
 *       ADC: Section 13
 *   g4:
 *     https://www.st.com/resource/en/reference_manual/rm0440-stm32g4-series-advanced-armbased-32bit-mcus-stmicroelectronics.pdf
 *       ADC: Section 21
 * Resources:
 *   ADCx
 *   DMAx
 */
class AdcDevice_ADC_DMA : public BufferDevice {
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
    ///
    AdcDevice_ADC_DMA(Loop_Queue &loop, Array<const gpio::Config> analogPins, const adc::Info &adcInfo, const dma::Info &dmaInfo,
        adc::ClockConfig clockConfig, adc::Config config, Array<const adc::Input> sequence, adc::Trigger trigger);

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
    ///
    AdcDevice_ADC_DMA(Loop_Queue &loop, Array<const gpio::Config> analogPins, const adc::Info2 &adcInfo, const dma::Info &dmaInfo,
        adc::ClockConfig clockConfig, adc::Config config,
        Array<const adc::Input> sequence1, Array<const adc::Input> sequence2, adc::Trigger trigger);
#endif

    ~AdcDevice_ADC_DMA() override;

    class Buffer2;

    // internal buffer base class, derives from IntrusiveListNode for the list of active transfers and Loop_Queue::Handler to be notified from the event loop
    class BufferBase : public coco::Buffer, public IntrusiveListNode, public Loop_Queue::Handler {
        friend class AdcDevice_ADC_DMA;
        friend class Buffer2;
    public:
        /**
         * Constructor
         * @param headerCapacity capacity of the header
         * @param data data of the buffer
         * @param capacity capacity of the buffer
         * @param channel channel to attach to
         */
        BufferBase(uint8_t *data, int capacity, AdcDevice_ADC_DMA &device);
        ~BufferBase() override;

        /**
         * Transfer the buffer contents from the ADC. The whole buffer gets transferred regardless of the current size.
         */
        bool start(Op op) override;
        bool cancel() override;

    protected:
        void start();
        void handle() override;

        AdcDevice_ADC_DMA &device;
        std::atomic<bool> active = false;
    };

    /**
     * Buffer for transferring data from the ADC.
     * Internally uses circular mode, therefore only one instance of Buffer1 and Buffer2 are allowed.
     * @tparam C capacity of buffer
     */
    template <int C>
    class Buffer1 : public BufferBase {
        friend class Buffer2;
    public:
        Buffer1(AdcDevice_ADC_DMA &device) : BufferBase(data, C / 2, device) {}

    protected:
        alignas(4) uint8_t data[C];
    };

    class Buffer2 : public BufferBase {
    public:
        template <int C>
        Buffer2(Buffer1<C> &buffer) : BufferBase(buffer.data + C / 2, C / 2, buffer.device) {}
    };


    // BufferDevice methods
    int getBufferCount();
    BufferBase &getBuffer(int index);

    /**
     * Call from interrupt handler for the DMA channel (e.g. DMA1_Channel1_IRQHandler())
     */
    void DMA_IRQHandler();

protected:
    Loop_Queue &loop;

    // ADC
    adc::Registers adc;

    // DMA
    dma::Status dmaStatus;
    dma::Channel dmaChannel;
    int dmaIrq;
    int dmaShift;

    // list of buffers
    BufferBase *buffers[2];
    int bufferCount = 0;
};

} // namespace coco
