/**
 * @file advertising_data.cpp
 * @brief Implementation of advertising data management functions for HM11Driver
 *
 * This file contains the implementation of functions related to managing
 * advertising data in the HM11 BLE module. These functions allow setting
 * various information in the advertising packets, such as flags, temperature,
 * humidity, and custom advertisement data.
 */

#include "../include/hm11/hm11_driver.hpp"

namespace hm11 {

template <UartPort Port, ReceiveHandler ReceiveHandlerImpl, LastReadPositionHandler ReadPositionHandlerImpl>
void HM11Driver<Port, ReceiveHandlerImpl, ReadPositionHandlerImpl>::set_advertising_flag(
    const util::AdvertisingFlag& flag, UartStatus& status) {
    // Convert the flag to a string using the to_string method
    std::string flag_str = flag.to_string();
    
    // Send the command and check the response
    transmit_and_check("AT+FLAG" + flag_str, std::string(OK_SET) + flag_str, status);
}

template <UartPort Port, ReceiveHandler ReceiveHandlerImpl, LastReadPositionHandler ReadPositionHandlerImpl>
void HM11Driver<Port, ReceiveHandlerImpl, ReadPositionHandlerImpl>::set_humi_information_byte_in_advertising(
    const util::HumiInformation& info, UartStatus& status) {
    // Convert the info to a string using the to_string method
    std::string info_str = info.to_string();
    
    // Send the command and check the response
    transmit_and_check("AT+HUMI" + info_str, std::string(OK_SET) + info_str, status);
}

template <UartPort Port, ReceiveHandler ReceiveHandlerImpl, LastReadPositionHandler ReadPositionHandlerImpl>
void HM11Driver<Port, ReceiveHandlerImpl, ReadPositionHandlerImpl>::set_temperature_information_byte_in_advertising(
    const util::AdvertisingTemperatureType& temperature, UartStatus& status) {
    // Convert the temperature to a string using the to_string method
    std::string temp_str = temperature.to_string();
    
    // Send the command and check the response
    // Note that this command expects "0x" prefix in the response
    transmit_and_check("AT+TEMP0x" + temp_str, std::string(OK_SET) + "0x" + temp_str, status);
}

template <UartPort Port, ReceiveHandler ReceiveHandlerImpl, LastReadPositionHandler ReadPositionHandlerImpl>
void HM11Driver<Port, ReceiveHandlerImpl, ReadPositionHandlerImpl>::set_module_advertisement_data(
    const util::AdvertisementData& data, UartStatus& status) {
    // Convert the data to a string using the to_string method
    std::string data_str = data.to_string();
    
    // Send the command and check the response
    transmit_and_check("AT+PACK" + data_str, std::string(OK_SET) + data_str, status);
}

} // namespace hm11