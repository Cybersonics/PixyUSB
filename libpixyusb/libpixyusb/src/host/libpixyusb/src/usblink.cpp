//
// begin license header
//
// This file is part of Pixy CMUcam5 or "Pixy" for short
//
// All Pixy source code is provided under the terms of the
// GNU General Public License v2 (http://www.gnu.org/licenses/gpl-2.0.html).
// Those wishing to use Pixy source code, software and/or
// technologies under different licensing terms should contact us at
// cmucam@cs.cmu.edu. Such licensing terms are available for
// all portions of the Pixy codebase presented here.
//
// end license header
//

#include <unistd.h>
#include <stdio.h>
#include "usblink.h"
#include "pixy.h"
#include "utils/timer.hpp"
#include "debuglog.h"
#include "chirpreceiver.hpp"

USBLink::USBLink(libusb_device_handle *handle)
{
	m_handle = handle;
	m_blockSize = 64;
	m_flags = LINK_FLAG_ERROR_CORRECTED;
}

USBLink::~USBLink()
{
	log("pixydebug: USBLink::~USBLink()\n");
}

void USBLink::close() {
	log("pixydebug: USBLink::close()\n");
	int return_value;

	return_value = libusb_release_interface(m_handle, 0);
	log("pixydebug:  libusb_release_interface() = %d\n", return_value);
	return_value = libusb_release_interface(m_handle, 1);
	log("pixydebug:  libusb_release_interface() = %d\n", return_value);
	libusb_close(m_handle);
	m_handle = NULL;

	log("pixydebug: USBLink::close() returned\n");
}

int USBLink::send(const uint8_t *data, uint32_t len, uint16_t timeoutMs)
{
	int res, transferred;

	//log("pixydebug: USBLink::send()\n");

	if (timeoutMs == 0) // 0 equals infinity
		timeoutMs = 10;

	if ((res = libusb_bulk_transfer(m_handle, 0x02, (unsigned char *)data, len, &transferred, timeoutMs)) < 0)
	{
		//log("pixydebug: USBLink::send():     libusb_bulk_transfer(len = %d, transferred = %d, timeoutMs = %d) = %d\n", len, transferred, timeoutMs, res);
		//log("pixydebug: USBLink::send() returned %d\n", res);
		return res;
	}

	//log("pixydebug: USBLink::send() returned %d\n", transferred);
	return transferred;
}

int USBLink::receive(uint8_t *data, uint32_t len, uint16_t timeoutMs)
{
	int res, transferred;

	//log("pixydebug: USBLink::receive()\n");

	if (timeoutMs == 0) // 0 equals infinity
		timeoutMs = 10;

	if ((res = libusb_bulk_transfer(m_handle, 0x82, (unsigned char *)data, len, &transferred, timeoutMs)) < 0)
	{
		//log("pixydebug: USBLink::receive():  libusb_bulk_transfer(len = %d, transferred = %d, timeoutMs = %d) = %d\n", len, transferred, timeoutMs, res);
		return res;
	}

	//log("pixydebug:  libusb_bulk_transfer(%d bytes) = %d\n", len, res);
	//log("pixydebug: USBLink::receive() returned %d (bytes transferred)\n", transferred);
	return transferred;
}

void USBLink::setTimer()
{
	timer_.reset();
}

uint32_t USBLink::getTimer()
{
	return timer_.elapsed();
}


