/*
userspace-tablet-driver-daemon
Copyright (C) 2021 - Aren Villanueva <https://github.com/kurikaesu/>

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <iostream>
#include <cstring>
#include <unistd.h>
#include <linux/uinput.h>
#include "artist_16_pro.h"

artist_16_pro::artist_16_pro() {
    productIds.push_back(0x090a);

    for (int currentAssignedButton = BTN_0; currentAssignedButton < BTN_8; ++currentAssignedButton) {
        padButtonAliases.push_back(currentAssignedButton);
    }
}

std::string artist_16_pro::getProductName(int productId) {
    if (productId == 0x090a) {
        return "XP-Pen Artist 16 Pro";
    }

    return "Unknown XP-Pen Device";
}

void artist_16_pro::setConfig(nlohmann::json config) {
    if (!config.contains("mapping") || config["mapping"] == nullptr) {
        config["mapping"] = nlohmann::json({});

        auto addToButtonMap = [&config](int key, int eventType, std::vector<int> codes) {
            std::string evstring = std::to_string(eventType);
            config["mapping"]["buttons"][std::to_string(key)][evstring] = codes;
        };

        // We are going to emulate the default mapping of the device
        addToButtonMap(BTN_0, EV_KEY, {KEY_B});
        addToButtonMap(BTN_1, EV_KEY, {KEY_E});
        addToButtonMap(BTN_2, EV_KEY, {KEY_LEFTALT});
        addToButtonMap(BTN_3, EV_KEY, {KEY_SPACE});
        addToButtonMap(BTN_4, EV_KEY, {KEY_LEFTCTRL, KEY_S});
        addToButtonMap(BTN_5, EV_KEY, {KEY_LEFTCTRL, KEY_Z});
        addToButtonMap(BTN_6, EV_KEY, {KEY_LEFTCTRL, KEY_LEFTALT, KEY_Z});
        addToButtonMap(BTN_7, EV_KEY, {KEY_LEFTCTRL, KEY_LEFTSHIFT, KEY_Z});
    }
    jsonConfig = config;

    submitMapping(jsonConfig);
}

int artist_16_pro::sendInitKeyOnInterface() {
    return 0x02;
}

bool artist_16_pro::attachToInterfaceId(int interfaceId) {
    return interfaceId == 2;
}

bool artist_16_pro::attachDevice(libusb_device_handle *handle, int interfaceId) {
    unsigned char* buf = new unsigned char[12];

    // We need to get a few more bits of information
    if (libusb_get_string_descriptor(handle, 0x64, 0x0409, buf, 12) != 12) {
        std::cout << "Could not get descriptor" << std::endl;
        return false;
    }

    int maxWidth = (buf[3] << 8) + buf[2];
    int maxHeight = (buf[5] << 8) + buf[4];
    maxPressure = (buf[9] << 8) + buf[8];
    int resolution = (buf[11] << 8) + buf[10];

    unsigned short vendorId = 0x28bd;
    unsigned short productId = 0xf90a;
    unsigned short versionId = 0x0001;

    struct uinput_pen_args penArgs {
        .maxWidth = maxWidth,
        .maxHeight = maxHeight,
        .maxPressure = maxPressure,
        .resolution = resolution,
        .maxTiltX = 60,
        .maxTiltY = 60,
        .vendorId = vendorId,
        .productId = productId,
        .versionId = versionId,
        {"XP-Pen Artist 16 Pro"},
    };

    struct uinput_pad_args padArgs {
        .padButtonAliases = padButtonAliases,
        .hasWheel = false,
        .hasHWheel = false,
        .wheelMax = 0,
        .hWheelMax = 0,
        .vendorId = vendorId,
        .productId = productId,
        .versionId = versionId,
        {"XP-Pen Artist 16 Pro Pad"},
    };

    uinputPens[handle] = create_pen(penArgs);
    uinputPads[handle] = create_pad(padArgs);

    return true;
}

bool artist_16_pro::handleTransferData(libusb_device_handle* handle, unsigned char *data, size_t dataLen) {
    switch (data[0]) {
        // Unified interface
        case 0x02:
            handleDigitizerEvent(handle, data, dataLen);
            handleFrameEvent(handle, data, dataLen);

            break;

        default:
            break;
    }

    return true;
}

void artist_16_pro::handleFrameEvent(libusb_device_handle *handle, unsigned char *data, size_t dataLen) {
    if (data[1] >= 0xf0) {
        // Extract the button being pressed (If there is one)
        long button = (data[4] << 16) + (data[3] << 8) + data[2];
        // Grab the first bit set in the button long which tells us the button number
        long position = ffsl(button);

        if (button != 0) {
            handlePadButtonPressed(handle, position);
        } else {
            handlePadButtonUnpressed(handle);
        }

        uinput_send(uinputPads[handle], EV_SYN, SYN_REPORT, 1);
    }
}
