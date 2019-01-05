#include <boost/thread/shared_mutex.hpp>
#include <map>
#include <stdio.h>
#include "pixy.h"
#include "pixyinterpreter.hpp"
#include "debuglog.h"
#include "libusb.h"

#define LIBUSB_CONTEXT NULL

boost::shared_mutex pixy_map_mutex;
std::map<uint32_t, PixyInterpreter *> interpreters;

/**

  \mainpage libpixyusb-0.4 API Reference

  \section introduction Introduction

  libpixyusb is an open source library that allows you to communicate with
  Pixy over the USB protocol.

  This documentation is aimed at application developers wishing to send
  commands to Pixy or read sensor data from Pixy.

  \section library_features Library features

  - Read blocks with or without color codes
  - RGB LED control (color/intensity)
  - Auto white balance control
  - Auto exposure compensation control
  - Brightness control
  - Servo position control/query
  - Custom commands

  \section dependencies Dependencies

  Required to build:

  - <a href=http://www.cmake.org>cmake</a>

  Required for runtime:

  - <a href=http://www.libusb.org>libusb</a>
  - <a href=http://www.boost.org>libboost</a>

  \section getting_started Getting Started

  The libpixyusb API reference documentation can be found here:

  libpixyusb API Reference

  Some tutorials that use libpixyusb can be found here:

  <a href=http://cmucam.org/projects/cmucam5/wiki/Hooking_up_Pixy_to_a_Raspberry_Pi>Hooking up Pixy to a Raspberry Pi</a>

  <a href=http://cmucam.org/projects/cmucam5/wiki/Hooking_up_Pixy_to_a_Beaglebone_Black>Hooking up Pixy to a BeagleBone Black</a>

  \section getting_help Getting Help

  Tutorials, walkthroughs, and more are available on the Pixy wiki page:

  <a href=http://www.cmucam.org/projects/cmucam5/wiki>Pixy Developer Wiki Page</a>

  Our friendly developers and users might be able to answer your question on the forums:

  <a href=http://www.cmucam.org/projects/cmucam5/boards/9>Pixy Software Discussion Forum</a>

  <a href=http://www.cmucam.org/projects/cmucam5/boards/8>Pixy Hardware Discussion Forum</a>

*/

// Pixy C API //

extern "C"
{
	static struct {
		int           error;
		const char *  text;
	} PIXY_ERROR_TABLE[] = {
	  { 0,                          "Success" },
	  { PIXY_ERROR_USB_IO,          "USB Error: I/O" },
	  { PIXY_ERROR_USB_BUSY,        "USB Error: Busy" },
	  { PIXY_ERROR_USB_NO_DEVICE,   "USB Error: No device" },
	  { PIXY_ERROR_USB_NOT_FOUND,   "USB Error: Target not found" },
	  { PIXY_ERROR_CHIRP,           "Chirp Protocol Error" },
	  { PIXY_ERROR_INVALID_COMMAND, "Pixy Error: Invalid command" },
	  { 0,                          0 }
	};

	int pixy_enumerate(int max_pixy_count, uint32_t *uids) {
		log("pixydebug: pixy_enumerate()\n");

		pixy_close();

		usleep(100000);

		boost::lock_guard<boost::shared_mutex> exclusive_lock(pixy_map_mutex);

		int return_value, device_count, i, pixy_count;
		libusb_device **device_list;
		libusb_device *device;
		libusb_device_descriptor descriptor;
		libusb_device_handle *handle;
		USBLink *link;
		PixyInterpreter *interpreter;
		uint32_t device_uid;

		return_value = libusb_init(LIBUSB_CONTEXT);
		log("pixydebug:  libusb_init() = %d\n", return_value);
		if (return_value) {
			goto pixy_enumerate_return;
		}

		//libusb_set_debug(LIBUSB_CONTEXT, LIBUSB_LOG_LEVEL_INFO);

		return_value = libusb_get_device_list(LIBUSB_CONTEXT, &device_list);
		log("pixydebug:  libusb_get_device_list() = %d\n", return_value);
		if (return_value < 0) {
			goto pixy_enumerate_return;
		}
		device_count = return_value;

		for (i = 0, pixy_count = 0; i < device_count && pixy_count < max_pixy_count; i++) {
			device = device_list[i];
			libusb_get_device_descriptor(device, &descriptor);
			log("pixydebug:  libusb_get_device_descriptor(): idVendor = 0x%04X, idProduct = 0x%04X\n", descriptor.idVendor, descriptor.idProduct);
			if (descriptor.idVendor != PIXY_VID || descriptor.idProduct != PIXY_PID) {
				log("pixydebug:  skipping device\n");
				continue;
			}

			return_value = libusb_open(device, &handle);
			log("pixydebug:  libusb_open() = %d\n", return_value);
			if (return_value) {
				continue;
			}

			return_value = libusb_set_configuration(handle, -1);
			log("pixydebug:  libusb_set_configuration() = %d\n", return_value);
			if (return_value) {
				goto pixy_enumerate_close_handle;
			}
			return_value = libusb_set_configuration(handle, 1);
			log("pixydebug:  libusb_set_configuration() = %d\n", return_value);
			if (return_value) {
				goto pixy_enumerate_close_handle;
			}

			libusb_detach_kernel_driver(handle, 0);
			libusb_detach_kernel_driver(handle, 1);

			return_value = libusb_claim_interface(handle, 0);
			log("pixydebug:  libusb_claim_interface() = %d\n", return_value);
			if (return_value) {
				goto pixy_enumerate_close_handle;
			}
			return_value = libusb_claim_interface(handle, 1);
			log("pixydebug:  libusb_claim_interface() = %d\n", return_value);
			if (return_value) {
				goto pixy_enumerate_release_interface_0;
			}

			return_value = libusb_set_interface_alt_setting(handle, 1, 0);
			log("pixydebug:  libusb_set_interface_alt_setting() = %d\n", return_value);
			if (return_value) {
				goto pixy_enumerate_release_interface_1;
			}

			// Is this really necessary?
			return_value = libusb_reset_device(handle);
			log("pixydebug:  libusb_reset_device() = %d\n", return_value);
			if (return_value) {
				goto pixy_enumerate_release_interface_1;
			}

			link = new USBLink(handle);
			interpreter = new PixyInterpreter();
			return_value = interpreter->init(link);
			log("pixydebug:  PixyInterpreter::init() = %d\n", return_value);
			if (return_value) {
				goto pixy_enumerate_close_interpreter;
			}

			return_value = interpreter->send_command("getUID", END_OUT_ARGS, &device_uid, END_IN_ARGS);
			log("pixydebug:  PixyInterpreter::send_command() = %d: device_uid = 0x%08X\n", return_value, device_uid);
			if (return_value) {
				goto pixy_enumerate_close_interpreter;
			}

			interpreters[device_uid] = interpreter;
			uids[pixy_count++] = device_uid;
			continue;

		pixy_enumerate_release_interface_1:
			libusb_release_interface(handle, 1);

		pixy_enumerate_release_interface_0:
			libusb_release_interface(handle, 0);

		pixy_enumerate_close_handle:
			libusb_close(handle);
			continue;

		pixy_enumerate_close_interpreter:
			interpreter->close();
			delete interpreter;
			continue;
		}

		libusb_free_device_list(device_list, 1);
		return_value = pixy_count;

	pixy_enumerate_return:
		log("pixydebug: pixy_enumerate() = %d\n", return_value);
		return return_value;
	}

	void pixy_close() {
		boost::lock_guard<boost::shared_mutex> exclusive_lock(pixy_map_mutex);

		std::map<uint32_t, PixyInterpreter *>::iterator it;
		PixyInterpreter *interpreter;

		log("pixydebug: pixy_close()\n");

		for (it = interpreters.begin(); it != interpreters.end(); ++it) {
			log("pixydebug:  closing 0x%08X\n", it->first);
			interpreter = it->second;
			interpreter->close();
			delete interpreter;
		}

		interpreters.clear();

		log("pixydebug: pixy_close() returned\n");
	}

	int pixy_get_blocks(uint32_t uid, uint16_t max_blocks, struct Block * blocks) {
		boost::shared_lock_guard<boost::shared_mutex> shared_lock(pixy_map_mutex);

		std::map<uint32_t, PixyInterpreter *>::iterator search;
		PixyInterpreter *interpreter;

		search = interpreters.find(uid);
		if (search == interpreters.end()) {
			return -1;
		}
		interpreter = search->second;

		return interpreter->get_blocks(max_blocks, blocks);
	}

	int pixy_blocks_are_new(uint32_t uid) {
		boost::shared_lock_guard<boost::shared_mutex> shared_lock(pixy_map_mutex);

		std::map<uint32_t, PixyInterpreter *>::iterator search;
		PixyInterpreter *interpreter;

		search = interpreters.find(uid);
		if (search == interpreters.end()) {
			return -1;
		}
		interpreter = search->second;

		return interpreter->blocks_are_new();
	}

	int pixy_cam_update_frame(uint32_t uid) {
		boost::shared_lock_guard<boost::shared_mutex> shared_lock(pixy_map_mutex);

		std::map<uint32_t, PixyInterpreter *>::iterator search;
		PixyInterpreter *interpreter;

		search = interpreters.find(uid);
		if (search == interpreters.end()) {
			return -1;
		}
		interpreter = search->second;

		return interpreter->update_frame();
	}

	int pixy_cam_get_frame(uint32_t uid, uint8_t *frame) {
		boost::shared_lock_guard<boost::shared_mutex> shared_lock(pixy_map_mutex);

		std::map<uint32_t, PixyInterpreter *>::iterator search;
		PixyInterpreter *interpreter;

		search = interpreters.find(uid);
		if (search == interpreters.end()) {
			return -1;
		}
		interpreter = search->second;

		interpreter->get_frame(frame);
		return 0;
	}

	int pixy_cam_reset_frame_wait(uint32_t uid) {
		boost::shared_lock_guard<boost::shared_mutex> shared_lock(pixy_map_mutex);

		std::map<uint32_t, PixyInterpreter *>::iterator search;
		PixyInterpreter *interpreter;

		search = interpreters.find(uid);
		if (search == interpreters.end()) {
			return -1;
		}
		interpreter = search->second;

		interpreter->reset_frame_wait();
		return 0;
	}

	int pixy_command(uint32_t uid, const char *name, ...) {
		boost::shared_lock_guard<boost::shared_mutex> shared_lock(pixy_map_mutex);

		va_list arguments;
		int     return_value;
		std::map<uint32_t, PixyInterpreter *>::iterator search;
		PixyInterpreter *interpreter;

		search = interpreters.find(uid);
		if (search == interpreters.end()) {
			return -1;
		}
		interpreter = search->second;

		va_start(arguments, name);
		return_value = interpreter->send_command(name, arguments);
		va_end(arguments);

		return return_value;
	}

	void pixy_error(int error_code) {
		int index;

		// Convert pixy error code to string and display to stdout //

		index = 0;

		while (PIXY_ERROR_TABLE[index].text != 0) {

			if (PIXY_ERROR_TABLE[index].error == error_code) {
				printf("%s\n", PIXY_ERROR_TABLE[index].text);
				return;
			}

			index += 1;
		}

		printf("Undefined error: [%d]\n", error_code);
	}

	int pixy_led_set_RGB(uint32_t uid, uint8_t red, uint8_t green, uint8_t blue) {
		int      chirp_response;
		int      return_value;
		uint32_t RGB;

		// Pack the RGB value //
		RGB = blue + (green << 8) + (red << 16);

		return_value = pixy_command(uid, "led_set", INT32(RGB), END_OUT_ARGS, &chirp_response, END_IN_ARGS);

		if (return_value < 0) {
			// Error //
			return return_value;
		}
		else {
			// Success //
			return chirp_response;
		}
	}

	int pixy_led_set_max_current(uint32_t uid, uint32_t current) {
		int chirp_response;
		int return_value;

		return_value = pixy_command(uid, "led_setMaxCurrent", INT32(current), END_OUT_ARGS, &chirp_response, END_IN_ARGS);

		if (return_value < 0) {
			// Error //
			return return_value;
		}
		else {
			// Success //
			return chirp_response;
		}
	}

	int pixy_led_get_max_current(uint32_t uid) {
		int      return_value;
		uint32_t chirp_response;

		return_value = pixy_command(uid, "led_getMaxCurrent", END_OUT_ARGS, &chirp_response, END_IN_ARGS);

		if (return_value < 0) {
			// Error //
			return return_value;
		}
		else {
			// Success //
			return chirp_response;
		}
	}

	int pixy_cam_set_auto_white_balance(uint32_t uid, uint8_t enable) {
		int      return_value;
		uint32_t chirp_response;

		return_value = pixy_command(uid, "cam_setAWB", UINT8(enable), END_OUT_ARGS, &chirp_response, END_IN_ARGS);

		if (return_value < 0) {
			// Error //
			return return_value;
		}
		else {
			// Success //
			return chirp_response;
		}
	}

	int pixy_cam_get_auto_white_balance(uint32_t uid) {
		int      return_value;
		uint32_t chirp_response;

		return_value = pixy_command(uid, "cam_getAWB", END_OUT_ARGS, &chirp_response, END_IN_ARGS);

		if (return_value < 0) {
			// Error //
			return return_value;
		}
		else {
			// Success //
			return chirp_response;
		}
	}

	uint32_t pixy_cam_get_white_balance_value(uint32_t uid) {
		int      return_value;
		uint32_t chirp_response;

		return_value = pixy_command(uid, "cam_getWBV", END_OUT_ARGS, &chirp_response, END_IN_ARGS);

		if (return_value < 0) {
			// Error //
			return return_value;
		}
		else {
			// Success //
			return chirp_response;
		}
	}

	int pixy_cam_set_white_balance_value(uint32_t uid, uint8_t red, uint8_t green, uint8_t blue) {
		int      return_value;
		uint32_t chirp_response;
		uint32_t white_balance;

		white_balance = green + (red << 8) + (blue << 16);

		return_value = pixy_command(uid, "cam_setWBV", UINT32(white_balance), END_OUT_ARGS, &chirp_response, END_IN_ARGS);

		if (return_value < 0) {
			// Error //
			return return_value;
		}
		else {
			// Success //
			return chirp_response;
		}
	}

	int pixy_cam_set_auto_exposure_compensation(uint32_t uid, uint8_t enable) {
		int      return_value;
		uint32_t chirp_response;

		return_value = pixy_command(uid, "cam_setAEC", UINT8(enable), END_OUT_ARGS, &chirp_response, END_IN_ARGS);

		if (return_value < 0) {
			// Error //
			return return_value;
		}
		else {
			// Success //
			return chirp_response;
		}
	}

	int pixy_cam_get_auto_exposure_compensation(uint32_t uid) {
		int      return_value;
		uint32_t chirp_response;

		return_value = pixy_command(uid, "cam_getAEC", END_OUT_ARGS, &chirp_response, END_IN_ARGS);

		if (return_value < 0) {
			// Error //
			return return_value;
		}
		else {
			// Success //
			return chirp_response;
		}
	}

	int pixy_cam_set_exposure_compensation(uint32_t uid, uint8_t gain, uint16_t compensation) {
		int      return_value;
		uint32_t chirp_response;
		uint32_t exposure;

		exposure = gain + (compensation << 8);

		return_value = pixy_command(uid, "cam_setECV", UINT32(exposure), END_OUT_ARGS, &chirp_response, END_IN_ARGS);

		if (return_value < 0) {
			// Error //
			return return_value;
		}
		else {
			// Success //
			return chirp_response;
		}
	}

	int pixy_cam_get_exposure_compensation(uint32_t uid, uint8_t * gain, uint16_t * compensation) {
		uint32_t exposure;
		int      return_value;

		return_value = pixy_command(uid, "cam_getECV", END_OUT_ARGS, &exposure, END_IN_ARGS);

		if (return_value < 0) {
			// Chirp error //
			return return_value;
		}

		if (gain == 0 || compensation == 0) {
			// Error: Null pointer //
			return PIXY_ERROR_INVALID_PARAMETER;
		}

		//printf("exp:%08x\n", exposure);

		*gain = exposure & 0xFF;
		*compensation = 0xFFFF & (exposure >> 8);

		return 0;
	}

	int pixy_cam_set_brightness(uint32_t uid, uint8_t brightness) {
		int chirp_response;
		int return_value;

		return_value = pixy_command(uid, "cam_setBrightness", UINT8(brightness), END_OUT_ARGS, &chirp_response, END_IN_ARGS);

		if (return_value < 0) {
			// Error //
			return return_value;
		}
		else {
			// Success //
			return chirp_response;
		}
	}

	int pixy_cam_get_brightness(uint32_t uid) {
		int chirp_response;
		int return_value;

		return_value = pixy_command(uid, "cam_getBrightness", END_OUT_ARGS, &chirp_response, END_IN_ARGS);

		if (return_value < 0) {
			// Error //
			return return_value;
		}
		else {
			// Success //
			return chirp_response;
		}
	}

	int pixy_rcs_get_position(uint32_t uid, uint8_t channel) {
		int chirp_response;
		int return_value;

		return_value = pixy_command(uid, "rcs_getPos", UINT8(channel), END_OUT_ARGS, &chirp_response, END_IN_ARGS);

		if (return_value < 0) {
			// Error //
			return return_value;
		}
		else {
			// Success //
			return chirp_response;
		}
	}

	int pixy_rcs_set_position(uint32_t uid, uint8_t channel, uint16_t position) {
		int chirp_response;
		int return_value;

		return_value = pixy_command(uid, "rcs_setPos", UINT8(channel), INT16(position), END_OUT_ARGS, &chirp_response, END_IN_ARGS);

		if (return_value < 0) {
			// Error //
			return return_value;
		}
		else {
			// Success //
			return chirp_response;
		}
	}

	int pixy_rcs_set_frequency(uint32_t uid, uint16_t frequency) {
		int chirp_response;
		int return_value;

		return_value = pixy_command(uid, "rcs_setFreq", UINT16(frequency), END_OUT_ARGS, &chirp_response, END_IN_ARGS);

		if (return_value < 0) {
			// Error //
			return return_value;
		}
		else {
			// Success //
			return chirp_response;
		}
	}

	int pixy_get_firmware_version(uint32_t uid, uint16_t * major, uint16_t * minor, uint16_t * build) {
		uint16_t * pixy_version;
		uint32_t   version_length;
		uint32_t   response;
		uint16_t   version[3];
		int        return_value;
		int        chirp_response;

		if (major == 0 || minor == 0 || build == 0) {
			// Error: Null pointer //
			return PIXY_ERROR_INVALID_PARAMETER;
		}

		return_value = pixy_command(uid, "version", END_OUT_ARGS, &response, &version_length, &pixy_version, END_IN_ARGS);

		if (return_value < 0) {
			// Error //
			return return_value;
		}

		memcpy((void *)version, pixy_version, 3 * sizeof(uint16_t));

		*major = version[0];
		*minor = version[1];
		*build = version[2];

		return 0;
	}
}

