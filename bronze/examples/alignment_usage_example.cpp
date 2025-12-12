/**
 * @file alignment_usage_example.cpp
 * @brief Example showing how to use the alignment utilities
 *
 * This example demonstrates proper usage of the memory alignment utilities
 * for DMA operations in the HM11 driver.
 */

#include "hm11/platform/alignment.hpp"
#include <cstdio>
#include <cstring>

// Example function that simulates receiving data via DMA
void dma_receive_simulation(void* buffer, size_t size) {
    // In a real application, this would set up DMA hardware
    // to receive data into the buffer
    
    // For simulation, we'll just write some data
    std::memset(buffer, 0xAA, size);
    
    // Simulate DMA completion
    
    // After DMA completes, we need to invalidate the cache
    // so CPU reads see the data written by DMA
    hm11::platform::cache::invalidate_dma_buffer(buffer, size);
}

// Example function that simulates transmitting data via DMA
void dma_transmit_simulation(const void* buffer, size_t size) {
    // Before starting DMA, we need to clean the cache
    // to ensure DMA sees the data written by CPU
    hm11::platform::cache::clean_dma_buffer(const_cast<void*>(buffer), size);
    
    // In a real application, this would set up DMA hardware
    // to transmit data from the buffer
    
    // For simulation, we'll just read the data
    const uint8_t* data = static_cast<const uint8_t*>(buffer);
    for (size_t i = 0; i < size; ++i) {
        (void)data[i]; // Just to prevent unused variable warning
    }
}

int main() {
    printf("Memory Alignment Utilities Example\n");
    
    // Example 1: Fixed-size DMA-aligned buffer
    printf("\nExample 1: Fixed-size DMA-aligned buffer\n");
    hm11::platform::DmaAlignedBuffer<uint8_t, 128> rxBuffer;
    
    // Clear the buffer
    rxBuffer.clear();
    
    // Verify alignment
    printf("Buffer address: %p\n", rxBuffer.data());
    printf("Is aligned: %s\n", hm11::platform::is_dma_aligned(rxBuffer.data()) ? "Yes" : "No");
    
    // Simulate DMA receive
    dma_receive_simulation(rxBuffer.data(), rxBuffer.byte_size());
    
    // Process received data
    printf("First byte after DMA: 0x%02X\n", rxBuffer[0]);
    
    // Example 2: Dynamic DMA-aligned buffer
    printf("\nExample 2: Dynamic DMA-aligned buffer\n");
    hm11::platform::DynamicDmaBuffer<uint8_t> txBuffer(256);
    
    // Fill with test data
    for (size_t i = 0; i < txBuffer.size(); ++i) {
        txBuffer[i] = static_cast<uint8_t>(i & 0xFF);
    }
    
    // Verify alignment
    printf("Buffer address: %p\n", txBuffer.data());
    printf("Is aligned: %s\n", hm11::platform::is_dma_aligned(txBuffer.data()) ? "Yes" : "No");
    
    // Simulate DMA transmit
    dma_transmit_simulation(txBuffer.data(), txBuffer.byte_size());
    
    printf("\nExample complete\n");
    return 0;
}