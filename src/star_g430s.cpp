/*
userspace_tablet_driver_daemon
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
#include "star_g430s.h"

star_g430s::star_g430s() {
    productIds.push_back(0x0913);
}

std::string star_g430s::getProductName(int productId) {
    if (productId == 0x0913) {
        return "XP-Pen Star G430S";
    }

    return star::getProductName(productId);
}

bool star_g430s::attachDevice(libusb_device_handle *handle, int interfaceId) {
    unsigned char* buf = new unsigned char[12];

    // We need to get a few more bits of information
    if (libusb_get_string_descriptor(handle, 0x64, 0x0409, buf, 12) != 12) {
        std::cout << "Could not get descriptor" << std::endl;
        return false;
    }

    // Hard coding these values in because the probe doesn't seem to work.
    int maxWidth = 32767; //(buf[3] << 8) + buf[2];
    int maxHeight = 32767; // (buf[5] << 8) + buf[4];
    int maxPressure = (buf[9] << 8) + buf[8];
    int resolution = 5080; // (buf[11] << 8) + buf[10];

    unsigned short vendorId = 0x28bd;
    unsigned short productId = 0xf913;
    unsigned short versionId = 0x0001;

    if (interfaceId == 2) {
        struct uinput_pen_args penArgs{
                .maxWidth = maxWidth,
                .maxHeight = maxHeight,
                .maxPressure = maxPressure,
                .resolution = resolution,
                .maxTiltX = 60,
                .maxTiltY = 60,
                .vendorId = vendorId,
                .productId = productId,
                .versionId = versionId,
                {"XP-Pen Star G430S"},
        };

        uinputPens[handle] = create_pen(penArgs);
    }

    return true;
}

