/**
 * @file dma.cpp
 * @brief Implementation of STM32 DMA configuration and management
 *
 * This file implements the DMA controller and stream configuration for STM32 microcontrollers,
 * providing direct access to DMA hardware following Design Decision 30.
 * It implements proper memory alignment and cache management as per Design Decision 20.
 */

#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <hm11/platform/stm32/dma.hpp>
#include <hm11/util/memory_order.hpp>
#include <unordered_map>
#include <utility>

namespace hm11::platform::stm32 {

// Registry for DMA stream instances to allow IRQ handlers to find the C++ objects
namespace {
// Indexed by DMA index and stream index
std::array<std::array<DMAStream*, 8>, 2> dma_stream_registry = {{{nullptr}}};

// Helper function to convert stream enum to index
constexpr std::size_t stream_to_index(device::DMAStream stream) {
    return static_cast<std::size_t>(stream);
}

// Helper function to convert DMA enum to index
constexpr std::size_t dma_to_index(device::DMAIndex dma) {
    return static_cast<std::size_t>(dma);
}

// DMA flag positions for each stream
constexpr std::array<std::uint32_t, 8> dma_flag_offsets = {
    0U,  // Stream 0: bits 0-5
    6U,  // Stream 1: bits 6-11
    16U, // Stream 2: bits 16-21
    22U, // Stream 3: bits 22-27
    0U,  // Stream 4: bits 0-5 in HISR/HIFCR
    6U,  // Stream 5: bits 6-11 in HISR/HIFCR
    16U, // Stream 6: bits 16-21 in HISR/HIFCR
    22U  // Stream 7: bits 22-27 in HISR/HIFCR
};

// DMA flag bit masks
constexpr std::uint32_t DMA_FLAG_FEIF = 0x01U;  // FIFO error flag
constexpr std::uint32_t DMA_FLAG_DMEIF = 0x04U; // Direct mode error flag
constexpr std::uint32_t DMA_FLAG_TEIF = 0x08U;  // Transfer error flag
constexpr std::uint32_t DMA_FLAG_HTIF = 0x10U;  // Half transfer flag
constexpr std::uint32_t DMA_FLAG_TCIF = 0x20U;  // Transfer complete flag
constexpr std::uint32_t DMA_FLAG_ALL = 0x3DU;   // All flags

// DMA channel selection
constexpr std::uint32_t DMA_SxCR_CHSEL_Pos = 25U;
constexpr std::uint32_t DMA_SxCR_CHSEL_Msk = 7U << DMA_SxCR_CHSEL_Pos;

// DMA configuration bit positions
constexpr std::uint32_t DMA_SxCR_EN_Pos = 0U;
constexpr std::uint32_t DMA_SxCR_DMEIE_Pos = 1U;
constexpr std::uint32_t DMA_SxCR_TEIE_Pos = 2U;
constexpr std::uint32_t DMA_SxCR_HTIE_Pos = 3U;
constexpr std::uint32_t DMA_SxCR_TCIE_Pos = 4U;
constexpr std::uint32_t DMA_SxCR_PFCTRL_Pos = 5U;
constexpr std::uint32_t DMA_SxCR_DIR_Pos = 6U;
constexpr std::uint32_t DMA_SxCR_CIRC_Pos = 8U;
constexpr std::uint32_t DMA_SxCR_PINC_Pos = 9U;
constexpr std::uint32_t DMA_SxCR_MINC_Pos = 10U;
constexpr std::uint32_t DMA_SxCR_PSIZE_Pos = 11U;
constexpr std::uint32_t DMA_SxCR_MSIZE_Pos = 13U;
constexpr std::uint32_t DMA_SxCR_PINCOS_Pos = 15U;
constexpr std::uint32_t DMA_SxCR_PL_Pos = 16U;
constexpr std::uint32_t DMA_SxCR_DBM_Pos = 18U;
constexpr std::uint32_t DMA_SxCR_CT_Pos = 19U;
constexpr std::uint32_t DMA_SxCR_PBURST_Pos = 21U;
constexpr std::uint32_t DMA_SxCR_MBURST_Pos = 23U;

// DMA FIFO configuration bit positions
constexpr std::uint32_t DMA_SxFCR_FTH_Pos = 0U;
constexpr std::uint32_t DMA_SxFCR_DMDIS_Pos = 2U;
constexpr std::uint32_t DMA_SxFCR_FS_Pos = 3U;
constexpr std::uint32_t DMA_SxFCR_FEIE_Pos = 7U;

// Helper functions to convert enums to register values
constexpr std::uint32_t direction_to_reg(DmaDirection dir) {
    switch (dir) {
        case DmaDirection::PeripheralToMemory:
            return 0U << DMA_SxCR_DIR_Pos;
        case DmaDirection::MemoryToPeripheral:
            return 1U << DMA_SxCR_DIR_Pos;
        case DmaDirection::MemoryToMemory:
            return 2U << DMA_SxCR_DIR_Pos;
        default:
            return 0U;
    }
}

constexpr std::uint32_t data_size_to_reg(DmaDataSize size) {
    switch (size) {
        case DmaDataSize::Byte:
            return 0U;
        case DmaDataSize::HalfWord:
            return 1U;
        case DmaDataSize::Word:
            return 2U;
        default:
            return 0U;
    }
}

constexpr std::uint32_t priority_to_reg(DmaPriority priority) {
    switch (priority) {
        case DmaPriority::Low:
            return 0U << DMA_SxCR_PL_Pos;
        case DmaPriority::Medium:
            return 1U << DMA_SxCR_PL_Pos;
        case DmaPriority::High:
            return 2U << DMA_SxCR_PL_Pos;
        case DmaPriority::VeryHigh:
            return 3U << DMA_SxCR_PL_Pos;
        default:
            return 0U;
    }
}

constexpr std::uint32_t burst_size_to_reg(DmaBurstSize size) {
    switch (size) {
        case DmaBurstSize::Single:
            return 0U;
        case DmaBurstSize::Burst4:
            return 1U;
        case DmaBurstSize::Burst8:
            return 2U;
        case DmaBurstSize::Burst16:
            return 3U;
        default:
            return 0U;
    }
}

constexpr std::uint32_t channel_to_reg(device::DMAChannel channel) {
    return static_cast<std::uint32_t>(channel) << DMA_SxCR_CHSEL_Pos;
}
} // namespace

// Registry functions
DMAStream* get_dma_stream_instance(device::DMAIndex dma_index, device::DMAStream stream) {
    return dma_stream_registry[dma_to_index(dma_index)][stream_to_index(stream)];
}

void register_dma_stream_instance(device::DMAIndex dma_index, device::DMAStream stream, DMAStream* dma_stream) {
    dma_stream_registry[dma_to_index(dma_index)][stream_to_index(stream)] = dma_stream;
}

void unregister_dma_stream_instance(device::DMAIndex dma_index, device::DMAStream stream) {
    dma_stream_registry[dma_to_index(dma_index)][stream_to_index(stream)] = nullptr;
}

void handle_dma_stream_irq(device::DMAIndex dma_index, device::DMAStream stream) {
    DMAStream* dma_stream = get_dma_stream_instance(dma_index, stream);
    if (dma_stream != nullptr) {
        dma_stream->handle_irq();
    }
}

// DMAStream implementation
DMAStream::DMAStream(device::DMAIndex dma_index, device::DMAStream stream, device::DMAChannel channel)
    : dma_peripheral_(device::get_dma_peripheral(dma_index)),
      stream_peripheral_(device::get_dma_stream_peripheral(dma_index, stream)), dma_index_(dma_index), stream_(stream),
      channel_(channel), status_(DmaStatus::OK), completion_semaphore_() {
    // Enable DMA clock
    device::enable_dma_clock(dma_index);

    // Register this instance for IRQ handling
    register_dma_stream_instance(dma_index, stream, this);
}

DMAStream::~DMAStream() {
    // Disable DMA stream
    stop_transfer();

    // Disable interrupts
    disable_interrupts();
    disable_dma_nvic();

    // Unregister this instance
    unregister_dma_stream_instance(dma_index_, stream_);
}

DmaStatus DMAStream::init(const DmaConfig& config) {
    // Check if DMA is busy
    if (is_busy()) {
        return DmaStatus::Busy;
    }

    // Disable the stream
    stream_peripheral_->CR &= ~(1U << DMA_SxCR_EN_Pos);

    // Wait until the stream is disabled
    while (stream_peripheral_->CR & (1U << DMA_SxCR_EN_Pos)) {
        // Wait for the stream to be disabled
    }

    // Clear all flags
    clear_all_flags();

    // Configure DMA stream
    std::uint32_t tmpreg = 0;

    // Set the channel
    tmpreg |= channel_to_reg(channel_);

    // Set the data transfer direction
    tmpreg |= direction_to_reg(config.direction);

    // Configure peripheral and memory incrementation
    if (config.peripheral_increment_mode) {
        tmpreg |= (1U << DMA_SxCR_PINC_Pos);
    }

    if (config.memory_increment_mode) {
        tmpreg |= (1U << DMA_SxCR_MINC_Pos);
    }

    // Configure data sizes
    tmpreg |= (data_size_to_reg(config.peripheral_data_size) << DMA_SxCR_PSIZE_Pos);
    tmpreg |= (data_size_to_reg(config.memory_data_size) << DMA_SxCR_MSIZE_Pos);

    // Configure circular mode
    if (config.circular_mode) {
        tmpreg |= (1U << DMA_SxCR_CIRC_Pos);
    }

    // Configure priority
    tmpreg |= priority_to_reg(config.priority);

    // Configure flow controller
    if (config.flow_controller == DmaFlowController::Peripheral) {
        tmpreg |= (1U << DMA_SxCR_PFCTRL_Pos);
    }

    // Configure burst modes
    if (config.peripheral_burst) {
        tmpreg |= (burst_size_to_reg(config.peripheral_burst_size) << DMA_SxCR_PBURST_Pos);
    }

    if (config.memory_burst) {
        tmpreg |= (burst_size_to_reg(config.memory_burst_size) << DMA_SxCR_MBURST_Pos);
    }

    // Write to DMA Stream CR register
    stream_peripheral_->CR = tmpreg;

    // Configure FIFO
    tmpreg = 0;
    if (config.fifo_mode) {
        tmpreg |= (1U << DMA_SxFCR_DMDIS_Pos);
        tmpreg |= (config.fifo_threshold & 0x3U) << DMA_SxFCR_FTH_Pos;
    }

    // Write to DMA Stream FCR register
    stream_peripheral_->FCR = tmpreg;

    return DmaStatus::OK;
}

DmaStatus DMAStream::start_transfer(std::uint32_t src_addr, std::uint32_t dest_addr, std::uint16_t data_length) {
    // Check if DMA is busy
    if (is_busy()) {
        return DmaStatus::Busy;
    }

    // Ensure the DMA stream is disabled
    stream_peripheral_->CR &= ~(1U << DMA_SxCR_EN_Pos);

    // Wait until the stream is disabled
    while (stream_peripheral_->CR & (1U << DMA_SxCR_EN_Pos)) {
        // Wait for the stream to be disabled
    }

    // Clear all flags
    clear_all_flags();

    // Set data length
    stream_peripheral_->NDTR = data_length;

    // Set addresses
    if ((stream_peripheral_->CR & (3U << DMA_SxCR_DIR_Pos)) == 0) {
        // Peripheral to memory
        stream_peripheral_->PAR = src_addr;
        stream_peripheral_->M0AR = dest_addr;
    } else if ((stream_peripheral_->CR & (3U << DMA_SxCR_DIR_Pos)) == (1U << DMA_SxCR_DIR_Pos)) {
        // Memory to peripheral
        stream_peripheral_->PAR = dest_addr;
        stream_peripheral_->M0AR = src_addr;
    } else {
        // Memory to memory
        stream_peripheral_->PAR = src_addr;
        stream_peripheral_->M0AR = dest_addr;
    }

    // Reset status
    status_.store(DmaStatus::OK, std::memory_order_relaxed);

    // Enable DMA stream
    stream_peripheral_->CR |= (1U << DMA_SxCR_EN_Pos);

    return DmaStatus::OK;
}

DmaStatus DMAStream::poll_for_completion(std::chrono::milliseconds timeout_ms) {
    using Clock = std::chrono::steady_clock;
    auto start = Clock::now();
    auto end = start + timeout_ms;

    while (Clock::now() < end) {
        if (is_transfer_complete_flag_set()) {
            clear_transfer_complete_flag();
            return DmaStatus::OK;
        }

        if (is_transfer_error_flag_set()) {
            clear_transfer_error_flag();
            return DmaStatus::TransferError;
        }

        if (is_direct_mode_error_flag_set()) {
            clear_direct_mode_error_flag();
            return DmaStatus::DirectModeError;
        }

        if (is_fifo_error_flag_set()) {
            clear_fifo_error_flag();
            return DmaStatus::FifoError;
        }
    }

    return DmaStatus::Timeout;
}

DmaStatus DMAStream::stop_transfer() {
    // Disable DMA stream
    stream_peripheral_->CR &= ~(1U << DMA_SxCR_EN_Pos);

    // Wait until the stream is disabled (timeout after a short period)
    std::uint32_t timeout = 1000;
    while ((stream_peripheral_->CR & (1U << DMA_SxCR_EN_Pos)) && timeout > 0) {
        timeout--;
    }

    // Clear all flags
    clear_all_flags();

    // Check if we timed out
    if ((stream_peripheral_->CR & (1U << DMA_SxCR_EN_Pos)) != 0) {
        return DmaStatus::Error;
    }

    return DmaStatus::OK;
}

std::uint16_t DMAStream::get_remaining_transfers() const {
    return static_cast<std::uint16_t>(stream_peripheral_->NDTR);
}

void DMAStream::enable_interrupts(
    bool transfer_complete,
    bool half_transfer,
    bool transfer_error,
    bool direct_mode_error,
    bool fifo_error) {
    std::uint32_t cr_reg = stream_peripheral_->CR;
    std::uint32_t fcr_reg = stream_peripheral_->FCR;

    // Clear all interrupt flags first
    cr_reg &= ~(
        (1U << DMA_SxCR_TCIE_Pos) | (1U << DMA_SxCR_HTIE_Pos) | (1U << DMA_SxCR_TEIE_Pos) | (1U << DMA_SxCR_DMEIE_Pos));
    fcr_reg &= ~(1U << DMA_SxFCR_FEIE_Pos);

    // Set requested interrupt flags
    if (transfer_complete) {
        cr_reg |= (1U << DMA_SxCR_TCIE_Pos);
    }

    if (half_transfer) {
        cr_reg |= (1U << DMA_SxCR_HTIE_Pos);
    }

    if (transfer_error) {
        cr_reg |= (1U << DMA_SxCR_TEIE_Pos);
    }

    if (direct_mode_error) {
        cr_reg |= (1U << DMA_SxCR_DMEIE_Pos);
    }

    if (fifo_error) {
        fcr_reg |= (1U << DMA_SxFCR_FEIE_Pos);
    }

    // Write back the registers
    stream_peripheral_->CR = cr_reg;
    stream_peripheral_->FCR = fcr_reg;
}

void DMAStream::disable_interrupts() {
    // Clear all interrupt flags
    stream_peripheral_->CR &= ~(
        (1U << DMA_SxCR_TCIE_Pos) | (1U << DMA_SxCR_HTIE_Pos) | (1U << DMA_SxCR_TEIE_Pos) | (1U << DMA_SxCR_DMEIE_Pos));
    stream_peripheral_->FCR &= ~(1U << DMA_SxFCR_FEIE_Pos);
}

void DMAStream::enable_dma_nvic(std::uint8_t priority) {
    // Get the IRQ number
    int16_t irq_number = device::to_cmsis_irqn(get_irq());

    // Set priority (0-15, where 0 is highest)
    NVIC_SetPriority(static_cast<IRQn_Type>(irq_number), priority);

    // Enable the IRQ in NVIC
    NVIC_EnableIRQ(static_cast<IRQn_Type>(irq_number));
}

void DMAStream::disable_dma_nvic() {
    // Get the IRQ number
    int16_t irq_number = device::to_cmsis_irqn(get_irq());

    // Disable the IRQ in NVIC
    NVIC_DisableIRQ(static_cast<IRQn_Type>(irq_number));
}

void DMAStream::handle_irq() {
    DmaStatus status = DmaStatus::OK;
    bool signal_completion = false;

    // Check for transfer complete
    if (is_transfer_complete_flag_set()) {
        clear_transfer_complete_flag();
        status = DmaStatus::OK;
        signal_completion = true;
    }
    // Check for transfer error
    else if (is_transfer_error_flag_set()) {
        clear_transfer_error_flag();
        status = DmaStatus::TransferError;
        signal_completion = true;
    }
    // Check for direct mode error
    else if (is_direct_mode_error_flag_set()) {
        clear_direct_mode_error_flag();
        status = DmaStatus::DirectModeError;
        signal_completion = true;
    }
    // Check for FIFO error
    else if (is_fifo_error_flag_set()) {
        clear_fifo_error_flag();
        status = DmaStatus::FifoError;
        signal_completion = true;
    }
    // We don't signal completion for half transfer
    else if (is_half_transfer_flag_set()) {
        clear_half_transfer_flag();
    }

    if (signal_completion) {
        // Store the status
        status_.store(status, memory_order::release);

        // Signal completion
        completion_semaphore_.give_from_isr();

        // Call the callback if provided
        if (completion_callback_ != nullptr) {
            completion_callback_(status);
        }
    }
}

void DMAStream::set_completion_callback(void (*callback)(DmaStatus)) {
    completion_callback_ = callback;
}

bool DMAStream::is_enabled() const {
    return (stream_peripheral_->CR & (1U << DMA_SxCR_EN_Pos)) != 0;
}

DmaStatus DMAStream::get_status() const {
    return status_.load(std::memory_order_acquire);
}

void DMAStream::wait_for_completion(std::chrono::milliseconds timeout_ms, DmaStatus& status) {
    if (completion_semaphore_.take_with_timeout(timeout_ms)) {
        status = status_.load(std::memory_order_acquire);
    } else {
        status = DmaStatus::Timeout;
    }
}

void DMAStream::clear_all_flags() {
    // Determine which register to use based on stream number
    if (stream_to_index(stream_) < 4) {
        // Streams 0-3 use LIFCR
        dma_peripheral_->LIFCR = DMA_FLAG_ALL << dma_flag_offsets[stream_to_index(stream_)];
    } else {
        // Streams 4-7 use HIFCR
        dma_peripheral_->HIFCR = DMA_FLAG_ALL << dma_flag_offsets[stream_to_index(stream_)];
    }
}

bool DMAStream::is_busy() const {
    return (stream_peripheral_->CR & (1U << DMA_SxCR_EN_Pos)) != 0;
}

std::uint32_t DMAStream::get_flag_position() const {
    return dma_flag_offsets[stream_to_index(stream_)];
}

bool DMAStream::is_transfer_complete_flag_set() const {
    std::uint32_t flag_pos = get_flag_position();
    if (stream_to_index(stream_) < 4) {
        return (dma_peripheral_->LISR & (DMA_FLAG_TCIF << flag_pos)) != 0;
    } else {
        return (dma_peripheral_->HISR & (DMA_FLAG_TCIF << flag_pos)) != 0;
    }
}

bool DMAStream::is_half_transfer_flag_set() const {
    std::uint32_t flag_pos = get_flag_position();
    if (stream_to_index(stream_) < 4) {
        return (dma_peripheral_->LISR & (DMA_FLAG_HTIF << flag_pos)) != 0;
    } else {
        return (dma_peripheral_->HISR & (DMA_FLAG_HTIF << flag_pos)) != 0;
    }
}

bool DMAStream::is_transfer_error_flag_set() const {
    std::uint32_t flag_pos = get_flag_position();
    if (stream_to_index(stream_) < 4) {
        return (dma_peripheral_->LISR & (DMA_FLAG_TEIF << flag_pos)) != 0;
    } else {
        return (dma_peripheral_->HISR & (DMA_FLAG_TEIF << flag_pos)) != 0;
    }
}

bool DMAStream::is_direct_mode_error_flag_set() const {
    std::uint32_t flag_pos = get_flag_position();
    if (stream_to_index(stream_) < 4) {
        return (dma_peripheral_->LISR & (DMA_FLAG_DMEIF << flag_pos)) != 0;
    } else {
        return (dma_peripheral_->HISR & (DMA_FLAG_DMEIF << flag_pos)) != 0;
    }
}

bool DMAStream::is_fifo_error_flag_set() const {
    std::uint32_t flag_pos = get_flag_position();
    if (stream_to_index(stream_) < 4) {
        return (dma_peripheral_->LISR & (DMA_FLAG_FEIF << flag_pos)) != 0;
    } else {
        return (dma_peripheral_->HISR & (DMA_FLAG_FEIF << flag_pos)) != 0;
    }
}

void DMAStream::clear_transfer_complete_flag() {
    std::uint32_t flag_pos = get_flag_position();
    if (stream_to_index(stream_) < 4) {
        dma_peripheral_->LIFCR = (DMA_FLAG_TCIF << flag_pos);
    } else {
        dma_peripheral_->HIFCR = (DMA_FLAG_TCIF << flag_pos);
    }
}

void DMAStream::clear_half_transfer_flag() {
    std::uint32_t flag_pos = get_flag_position();
    if (stream_to_index(stream_) < 4) {
        dma_peripheral_->LIFCR = (DMA_FLAG_HTIF << flag_pos);
    } else {
        dma_peripheral_->HIFCR = (DMA_FLAG_HTIF << flag_pos);
    }
}

void DMAStream::clear_transfer_error_flag() {
    std::uint32_t flag_pos = get_flag_position();
    if (stream_to_index(stream_) < 4) {
        dma_peripheral_->LIFCR = (DMA_FLAG_TEIF << flag_pos);
    } else {
        dma_peripheral_->HIFCR = (DMA_FLAG_TEIF << flag_pos);
    }
}

void DMAStream::clear_direct_mode_error_flag() {
    std::uint32_t flag_pos = get_flag_position();
    if (stream_to_index(stream_) < 4) {
        dma_peripheral_->LIFCR = (DMA_FLAG_DMEIF << flag_pos);
    } else {
        dma_peripheral_->HIFCR = (DMA_FLAG_DMEIF << flag_pos);
    }
}

void DMAStream::clear_fifo_error_flag() {
    std::uint32_t flag_pos = get_flag_position();
    if (stream_to_index(stream_) < 4) {
        dma_peripheral_->LIFCR = (DMA_FLAG_FEIF << flag_pos);
    } else {
        dma_peripheral_->HIFCR = (DMA_FLAG_FEIF << flag_pos);
    }
}

// DMA channel selection for peripherals
device::DMAChannel
get_dma_channel_for_peripheral(void* peripheral_address, device::DMAIndex dma_index, device::DMAStream stream) {
    // This mapping is specific to STM32F4xx series and follows the DMA request mapping
    // from the STM32F4xx reference manual.

    // We'll implement a simple lookup table for common peripherals
    // For a complete implementation, refer to the DMA request mapping table in the reference manual

    uintptr_t addr = reinterpret_cast<uintptr_t>(peripheral_address);

    // DMA1 channels
    if (dma_index == device::DMAIndex::DMA1) {
        // UART/USART peripheral mappings for DMA1
        if (addr == reinterpret_cast<uintptr_t>(USART1)) {
            if (stream == device::DMAStream::Stream1)
                return device::DMAChannel::Channel4; // RX
            if (stream == device::DMAStream::Stream4)
                return device::DMAChannel::Channel4; // TX
        } else if (addr == reinterpret_cast<uintptr_t>(USART2)) {
            if (stream == device::DMAStream::Stream5)
                return device::DMAChannel::Channel4; // RX
            if (stream == device::DMAStream::Stream6)
                return device::DMAChannel::Channel4; // TX
        } else if (addr == reinterpret_cast<uintptr_t>(USART3)) {
            if (stream == device::DMAStream::Stream1)
                return device::DMAChannel::Channel4; // RX
            if (stream == device::DMAStream::Stream3)
                return device::DMAChannel::Channel4; // TX
        } else if (addr == reinterpret_cast<uintptr_t>(UART4)) {
            if (stream == device::DMAStream::Stream2)
                return device::DMAChannel::Channel4; // RX
            if (stream == device::DMAStream::Stream4)
                return device::DMAChannel::Channel4; // TX
        } else if (addr == reinterpret_cast<uintptr_t>(UART5)) {
            if (stream == device::DMAStream::Stream0)
                return device::DMAChannel::Channel4; // RX
            if (stream == device::DMAStream::Stream7)
                return device::DMAChannel::Channel4; // TX
        } else if (addr == reinterpret_cast<uintptr_t>(USART6)) {
            if (stream == device::DMAStream::Stream1)
                return device::DMAChannel::Channel5; // RX
            if (stream == device::DMAStream::Stream6)
                return device::DMAChannel::Channel5; // TX
        } else if (addr == reinterpret_cast<uintptr_t>(UART7)) {
            if (stream == device::DMAStream::Stream3)
                return device::DMAChannel::Channel5; // RX
            if (stream == device::DMAStream::Stream1)
                return device::DMAChannel::Channel5; // TX
        } else if (addr == reinterpret_cast<uintptr_t>(UART8)) {
            if (stream == device::DMAStream::Stream6)
                return device::DMAChannel::Channel5; // RX
            if (stream == device::DMAStream::Stream0)
                return device::DMAChannel::Channel5; // TX
        }
    }

    // DMA2 channels
    else if (dma_index == device::DMAIndex::DMA2) {
        // UART/USART peripheral mappings for DMA2
        if (addr == reinterpret_cast<uintptr_t>(USART1)) {
            if (stream == device::DMAStream::Stream2)
                return device::DMAChannel::Channel4; // RX
            if (stream == device::DMAStream::Stream7)
                return device::DMAChannel::Channel4; // TX
        } else if (addr == reinterpret_cast<uintptr_t>(USART6)) {
            if (stream == device::DMAStream::Stream2)
                return device::DMAChannel::Channel5; // RX
            if (stream == device::DMAStream::Stream6)
                return device::DMAChannel::Channel5; // TX
            if (stream == device::DMAStream::Stream1)
                return device::DMAChannel::Channel5; // RX (alternate)
            if (stream == device::DMAStream::Stream7)
                return device::DMAChannel::Channel5; // TX (alternate)
        }
    }

    // Default channel if no specific mapping is found
    // In practice, this should be verified against the datasheet
    return device::DMAChannel::Channel0;
}

} // namespace hm11::platform::stm32