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

#include <string.h>
#include <stdio.h>
#include "pixyinterpreter.hpp"
#include "debuglog.h"

PixyInterpreter::PixyInterpreter() {
	is_closing_ = false;
	is_running_ = false;
	link_ = NULL;
	receiver_ = NULL;
}

PixyInterpreter::~PixyInterpreter() {
	log("pixydebug: PixyInterpreter::~PixyInterpreter()\n");
}

int PixyInterpreter::init(USBLink *link) {
	boost::lock_guard<boost::mutex> guard(chirp_access_mutex_);

	int return_value;

	log("pixydebug: PixyInterpreter::init()\n");

	link_ = link;

	receiver_ = new ChirpReceiver(link_, this);
	get_frame_proc_ = receiver_->getProc("cam_getFrame", (ProcPtr) &PixyInterpreter::frame_callback);

	// Create the interpreter thread //

	is_running_ = true;
	waiting_for_frame_ = false;
	thread_ = boost::thread(&PixyInterpreter::interpreter_thread, this);

	return 0;
}

void PixyInterpreter::close() {
	log("pixydebug: PixyInterpreter::close()\n");

	// Is the interpreter thread alive? //
	if (thread_.joinable()) {
		// Thread is running, tell the interpreter thread to die. //
		is_closing_ = true;
		thread_.join();
	}

	boost::lock_guard<boost::mutex> guard(chirp_access_mutex_);

	if (receiver_) {
		delete receiver_;
		receiver_ = NULL;
	}

	if (link_) {
		link_->close();
		delete link_;
		link_ = NULL;
	}

	log("pixydebug: PixyInterpreter::close() returned\n");
}

int PixyInterpreter::get_blocks(int max_blocks, Block * blocks) {
	boost::lock_guard<boost::mutex> guard(blocks_access_mutex_);

	uint16_t number_of_blocks_to_copy;
	uint16_t index;

	// Check parameters //

	if (max_blocks < 0 || blocks == 0) {
		return PIXY_ERROR_INVALID_PARAMETER;
	}

	number_of_blocks_to_copy = (max_blocks >= blocks_.size() ? blocks_.size() : max_blocks);

	// Copy blocks //

	for (index = 0; index != number_of_blocks_to_copy; ++index) {
		memcpy(&blocks[index], &blocks_[index], sizeof(Block));
	}

	blocks_are_new_ = false;

	return number_of_blocks_to_copy;
}

int PixyInterpreter::update_frame() {
	boost::lock_guard<boost::mutex> guard(chirp_access_mutex_);

	int return_value;

	if (!is_running_) {
		return -201;
	}

	if (waiting_for_frame_) {
		return -202;
	}

	return_value = receiver_->call(ASYNC, get_frame_proc_, UINT8(0x21), UINT16(0), UINT16(0), UINT16(PIXY_FRAME_WIDTH), UINT16(PIXY_FRAME_HEIGHT), END_OUT_ARGS);
	if (!return_value) {
		waiting_for_frame_ = true;
	}
	return return_value;
}

void PixyInterpreter::get_frame(uint8_t *frame) {
	boost::lock_guard<boost::mutex> guard(frame_access_mutex_);

	//TODO:
	memcpy(frame, bayer_frame_, PIXY_FRAME_WIDTH * PIXY_FRAME_HEIGHT);
}

void PixyInterpreter::reset_frame_wait() {
	waiting_for_frame_ = false;
}

void PixyInterpreter::frame_callback(int *response, uint32_t *fourCC, uint8_t *renderFlags, uint16_t *width, uint16_t *height, uint32_t *numPixels, uint8_t **frame, Chirp *chirp) {
	const void *args[] = { fourCC, renderFlags, width, height, numPixels, frame };
	static_cast<ChirpReceiver *>(chirp)->handleXdata(args);
}

int PixyInterpreter::send_command(const char * name, ...) {
	va_list arguments;
	int     return_value;

	va_start(arguments, name);
	return_value = send_command(name, arguments);
	va_end(arguments);

	return return_value;
}

int PixyInterpreter::send_command(const char * name, va_list args) {
	boost::lock_guard<boost::mutex> guard(chirp_access_mutex_);

	ChirpProc procedure_id;
	int       return_value;
	va_list   arguments;
	std::map<std::string, ChirpProc>::iterator search;

	va_copy(arguments, args);

	if (!is_running_) {
		va_end(arguments);
		return -201;
	}

	search = proc_cache_.find(name);
	if (search == proc_cache_.end()) {
		// Request chirp procedure id for 'name'. //
		procedure_id = receiver_->getProc(name);

		// Was there an error requesting procedure id? //
		if (procedure_id < 0) {
			// Request error //
			va_end(arguments);

			return PIXY_ERROR_INVALID_COMMAND;
		}

		proc_cache_[name] = procedure_id;
	} else {
		procedure_id = search->second;
	}

	// Execute chirp synchronous remote procedure call //
	return_value = receiver_->call(SYNC, procedure_id, arguments);
	va_end(arguments);

	return return_value;
}

void PixyInterpreter::interpreter_thread() {
	// Read from Pixy USB connection using the Chirp //
	// protocol until we're told to stop.            //
	while (!is_closing_) {
		{
			boost::lock_guard<boost::mutex> guard(chirp_access_mutex_);
			receiver_->service(false);
		}
		boost::this_thread::yield();
	}

	is_running_ = false;
	log("pixydebug: PixyInterpreter::interpreter_thread() returned\n");
}


void PixyInterpreter::interpret_data(const void * chirp_data[]) {
	uint8_t  chirp_message;
	uint32_t chirp_type;
	if (chirp_data[0]) {

		chirp_message = Chirp::getType(chirp_data[0]);

		switch (chirp_message) {

		case CRP_TYPE_HINT:

			chirp_type = *static_cast<const uint32_t *>(chirp_data[0]);

			switch (chirp_type) {

			case FOURCC('B', 'A', '8', '1'):
				interpret_BA81(chirp_data + 1);
				break;
			case FOURCC('C', 'C', 'Q', '1'):
				break;
			case FOURCC('C', 'C', 'B', '1'):
				interpret_CCB1(chirp_data + 1);
				break;
			case FOURCC('C', 'C', 'B', '2'):
				interpret_CCB2(chirp_data + 1);
				break;
			case FOURCC('C', 'M', 'V', '1'):
				break;
			case FOURCC('C', 'M', 'V', '2'):
				break;
			default:
				//printf("libpixy: Chirp hint [%u] not recognized.\n", chirp_type);
				break;
			}

			break;

		case CRP_HSTRING:

			break;

		default:

			//fprintf(stderr, "libpixy: Unknown message received from Pixy: [%u]\n", chirp_message);
			break;
		}
	}
}

void PixyInterpreter::interpret_BA81(const void * BA81_data[]) {
	boost::lock_guard<boost::mutex> guard(frame_access_mutex_);

	waiting_for_frame_ = false;
	//TODO:
	memcpy(bayer_frame_, BA81_data[4], PIXY_FRAME_WIDTH * PIXY_FRAME_HEIGHT);
}

void PixyInterpreter::interpret_CCB1(const void * CCB1_data[]) {
	boost::lock_guard<boost::mutex> guard(blocks_access_mutex_);

	uint32_t       number_of_blobs;
	const BlobA *  blobs;
	uint32_t       index;

	// Add blocks with normal signatures //

	number_of_blobs = *static_cast<const uint32_t *>(CCB1_data[3]);
	blobs = static_cast<const BlobA *>(CCB1_data[4]);

	number_of_blobs /= sizeof(BlobA) / sizeof(uint16_t);

	add_normal_blocks(blobs, number_of_blobs);
	blocks_are_new_ = true;
}


void PixyInterpreter::interpret_CCB2(const void * CCB2_data[]) {
	boost::lock_guard<boost::mutex> guard(blocks_access_mutex_);

	uint32_t       number_of_blobs;
	const BlobA *  A_blobs;
	const BlobB *  B_blobs;
	uint32_t       index;

	// The blocks container will only contain the newest //
	// blocks                                            //
	blocks_.clear();

	// Add blocks with color code signatures //

	number_of_blobs = *static_cast<const uint32_t *>(CCB2_data[5]);
	B_blobs = static_cast<const BlobB *>(CCB2_data[6]);

	number_of_blobs /= sizeof(BlobB) / sizeof(uint16_t);
	add_color_code_blocks(B_blobs, number_of_blobs);

	// Add blocks with normal signatures //

	number_of_blobs = *static_cast<const uint32_t *>(CCB2_data[3]);
	A_blobs = static_cast<const BlobA *>(CCB2_data[4]);

	number_of_blobs /= sizeof(BlobA) / sizeof(uint16_t);

	add_normal_blocks(A_blobs, number_of_blobs);
	blocks_are_new_ = true;
}

void PixyInterpreter::add_normal_blocks(const BlobA * blocks, uint32_t count) {
	uint32_t index;
	Block    block;

	for (index = 0; index != count; ++index) {

		// Decode CCB1 'Normal' Signature Type //

		block.type = PIXY_BLOCKTYPE_NORMAL;
		block.signature = blocks[index].m_model;
		block.width = blocks[index].m_right - blocks[index].m_left;
		block.height = blocks[index].m_bottom - blocks[index].m_top;
		block.x = blocks[index].m_left + block.width / 2;
		block.y = blocks[index].m_top + block.height / 2;

		// Angle is not a valid parameter for 'Normal'  //
		// signature types. Setting to zero by default. //
		block.angle = 0;

		// Store new block in block buffer //

		if (blocks_.size() == PIXY_BLOCK_CAPACITY) {
			// Blocks buffer is full - replace oldest received block with newest block //
			blocks_.erase(blocks_.begin());
			blocks_.push_back(block);
		}
		else {
			// Add new block to blocks buffer //
			blocks_.push_back(block);
		}
	}
}

void PixyInterpreter::add_color_code_blocks(const BlobB * blocks, uint32_t count) {
	uint32_t index;
	Block    block;

	for (index = 0; index != count; ++index) {

		// Decode 'Color Code' Signature Type //

		block.type = PIXY_BLOCKTYPE_COLOR_CODE;
		block.signature = blocks[index].m_model;
		block.width = blocks[index].m_right - blocks[index].m_left;
		block.height = blocks[index].m_bottom - blocks[index].m_top;
		block.x = blocks[index].m_left + block.width / 2;
		block.y = blocks[index].m_top + block.height / 2;
		block.angle = blocks[index].m_angle;

		// Store new block in block buffer //

		if (blocks_.size() == PIXY_BLOCK_CAPACITY) {
			// Blocks buffer is full - replace oldest received block with newest block //
			blocks_.erase(blocks_.begin());
			blocks_.push_back(block);
		}
		else {
			// Add new block to blocks buffer //
			blocks_.push_back(block);
		}
	}
}

int PixyInterpreter::blocks_are_new() {
	//usleep(100); // sleep a bit so client doesn't need to
	if (blocks_are_new_) {
		// Fresh blocks!! :D //
		return 1;
	}
	else {
		// Stale blocks... :\ //
		return 0;
	}
}

