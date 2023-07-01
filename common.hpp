/*
*   Copyright [2023] [Skylark Wireless LLC]
*
*   Licensed under the Apache License, Version 2.0 (the "License");
*   you may not use this file except in compliance with the License.
*   You may obtain a copy of the License at
*
*       http://www.apache.org/licenses/LICENSE-2.0
*
*   Unless required by applicable law or agreed to in writing, software
*   distributed under the License is distributed on an "AS IS" BASIS,
*   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*   See the License for the specific language governing permissions and
*   limitations under the License.
*/



////////////////////////////////////////////////////////////////////////
// Common faros system definitions
// Changing these values might be dangerous!
// FPGA designs have similar values built into them...
////////////////////////////////////////////////////////////////////////

#pragma once
#include <cstdio>

/***********************************************************************
 * Data block and FEC constants
 **********************************************************************/

//! padding for coding: max history for sub-block interleaving (3 bits by 32 cols)
constexpr size_t SKLK_PHY_D_PAD = 32*3;

//! Maximum size bit count for forward error correction
constexpr size_t SKLK_PHY_FEC_BITS_MAX = 4096;

//! The span of a band in subcarriers
constexpr size_t SKLK_PHY_BAND_SIZE = 36;

//! The span of a LTE resource block in subcarriers
constexpr size_t SKLK_PHY_LTE_RB_SIZE = 12;

//! The number of LTE resource blocks in a subframe
constexpr size_t SKLK_PHY_LTE_RB_PER_SUBFRAME = (SKLK_PHY_BAND_SIZE / SKLK_PHY_LTE_RB_SIZE) * 2;

//! Number of control symbols on the first downlink subframe
constexpr size_t SKLK_PHY_DOWNLINK_CTRL_SYMS = 1;

//! Number of control symbols on the first uplink subframe
constexpr size_t SKLK_PHY_UPLINK_CTRL_SYMS = 0;

//! The frequency range per subcarrier
constexpr size_t SKLK_PHY_FREQ_PER_BIN = 15000;

/*!
 * Maximum number of bits in a turbo encoded data block 13 symbols at QAM256
 * Normal cylic prefix subframe, equalization pilot, no control symbols.
 */
constexpr size_t SKLK_PHY_BLOCK_MAX_BITS = SKLK_PHY_BAND_SIZE*13*8;

//! How many blocks can be in a band (one block per subframe)
constexpr size_t SKLK_PHY_MAX_BLOCKS_PER_BAND = 10;

/*!
 * The maximum number of frames to wait for a downlink response before
 * automatically NACKing.
 */
constexpr size_t SKLK_PHY_MESSAGE_TIME_WINDOW = 5;

/***********************************************************************
 * System maximums
 **********************************************************************/

//a single pilot tone is sent for each sub-band within a band
//this pilot is used as the estimate across the entire sub-band
//more sub-bands mean more accuracy, but more time taken to re-estimate
constexpr int NUM_PILOT_SUBBANDS = 4;

//! Maximum number of bands (in a 40 MHz system)
constexpr size_t SKLK_PHY_MAX_BANDS = 128;

//! Maximum number of users per MIMO spatial stream
constexpr size_t SKLK_PHY_MAX_MIMO_USERS = 64;

//! Maximum number of spatial stream users per UE
constexpr size_t SKLK_PHY_MAX_UE_USERS = 2;

//! Maximum number of spacial streams total
constexpr size_t SKLK_PHY_MAX_USERS = 256;

//! Maximum number of unique frame numbers
constexpr size_t SKLK_PHY_MAX_FRAMES = 256;

//! Maximum number of radio channels for mu2
constexpr size_t SKLK_PHY_MAX_RADIOS = 256;

constexpr size_t SKLK_PHY_MAX_ESTIMATIONS = 4;

//! Maximum number of downlink subframes per frame
constexpr size_t SKLK_PHY_MAX_DNL_SUBFRAMES = 8;

//! Maximum number of uplink subframes per frame
constexpr size_t SKLK_PHY_MAX_UPL_SUBFRAMES = 4;

//! Maximum number of uplink pilots per frame
constexpr size_t SKLK_PHY_MAX_UPL_PILOTS = 8;

/***********************************************************************
 * Other constants
 **********************************************************************/

//! The standard valid used to indicate a temperature sensor is invalid
constexpr int SKLK_PHY_INVALID_TEMPERATURE = -300;

#ifdef SKLK_PHY_DEBUG
static const bool SKLK_PHY_DEBUG_FLAG = true;
#else
static const bool SKLK_PHY_DEBUG_FLAG = false;
#endif
