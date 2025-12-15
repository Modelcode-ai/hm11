// SPDX-FileCopyrightText: 2025
// SPDX-License-Identifier: MIT

#ifndef HM11_TESTING_ENABLED
#error "MockUARTPort should only be used in test builds. Define HM11_TESTING_ENABLED to use this header."
#endif

namespace hm11 {

/**
 * @brief Mock UARTPort for testing (test-only code).
 *
 * Transmit does nothing (always reports Ok). The next call to receive will
 * return data previously supplied via `set_next_receive`.
 */
 class MockUARTPort : public UARTPort {
    public:
      MockUARTPort() = default;
      ~MockUARTPort() override = default;

      void transmit(std::string_view data [[maybe_unused]], Status& status) override {
          last_transmitted = data;
          if (transmit_error) {
              status = Status::Error;
          } else {
              status = Status::Ok;
          }
      }

      void receive(std::span<uint8_t> buffer, Status& status, unsigned int timeout [[maybe_unused]] = 1000) override {
          // Clear the buffer first to ensure no leftover data
          std::fill(buffer.begin(), buffer.end(), 0);

          // If we have receive fragments, return them one at a time
          if (!receive_fragments.empty()) {
              const std::string FRAGMENT = receive_fragments.front();  // Copy first
              receive_fragments.erase(receive_fragments.begin());      // Then erase

              if (FRAGMENT.size() > buffer.size()) {
                  status = Status::InvalidLength;
                  return;
              }

              for (std::size_t i = 0; i < FRAGMENT.size(); ++i) {
                  buffer[i] = static_cast<uint8_t>(FRAGMENT[i]);
              }
              // buffer is already zero-filled, so data is null-terminated
              status = Status::Ok;
              return;
          }

          // If next_receive has data, return it (only once)
          if (!next_receive.empty()) {
              if (next_receive.size() > buffer.size()) {
                  status = Status::InvalidLength;
                  return;
              }

              for (std::size_t i = 0; i < next_receive.size(); ++i) {
                  buffer[i] = static_cast<uint8_t>(next_receive[i]);
              }
              next_receive.clear();  // Clear after returning once
              // buffer is already zero-filled, so data is null-terminated
              status = Status::Ok;
              return;
          }

          // No more data available - return timeout
          status = Status::Timeout;
      }

      void set_receive_handler(ReceiveHandler h [[maybe_unused]]) override {
          // No-op for mock – users set data via set_next_receive.
      }

      std::string get_transmitted_data() const { return last_transmitted; }

      /**
       * @brief Define the data that the next receive operation will return.
       */
      void set_next_receive(const std::string_view DATA) { next_receive = std::string(DATA); }

      /**
       * @brief Add a fragment to be returned by subsequent receive calls.
       * Each call to receive() will return one fragment in order.
       */
      void add_receive_fragment(const std::string_view FRAGMENT) {
          receive_fragments.emplace_back(FRAGMENT);
      }

      /**
       * @brief Simulate a transmit error.
       */
      void set_transmit_error(bool error) { transmit_error = error; }

    private:
      std::string next_receive;
      std::string last_transmitted;
      std::vector<std::string> receive_fragments;
      bool transmit_error = false;
  };

} // namespace hm11
