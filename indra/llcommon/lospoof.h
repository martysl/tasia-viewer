#ifndef LOLISTORM_LOSPOOF_H
#define LOLISTORM_LOSPOOF_H

#include "stdtypes.h"

#include <string>

class LLSD;

// Identifying information in Second Life:
//
// NodeID / mac_address (viewer stats):
//	MAC address of the first-found network adapter
//	All-zeroes if no network adapters were detected
//	Generated in LLUUID::getNodeID() -- not stored
//
// MachineID / serial_number (viewer stats):
//	Windows "Serial Number" on Windows
//	Equal to NodeID other platforms
//	Generated and stored in LLMachineID::init()
//
// id0 (login) / mSerialNumber:
//	md5 of C: partition serial number (Windows)
//	md5 of highest-numbered partition serial seen in /dev/disk/by-uuid/ (Linux)
//	md5 of Mac serial number (IOPlatformSerialNumber) (macOS)
//	Generated in LLAppViewerWin32::generateSerialNumber()
//	Stored in LLAppViewer::init()
//
// "macid" / mac (login) / llHashedUniqueID:
//	if nodeid is not all-zeroes (i.e. not found):
//		md5 of nodeid
//	otherwise if machineid is not all-zeroes:
//		md5 of machineid
//	otherwise:
//		all-zeroes
//	Generated in llHashedUniqueID()

const std::string& lolistorm_get_seed();
const std::string& lolistorm_get_username();

void lolistorm_reroll_seed();

void lolistorm_set_seed(const std::string&);
void lolistorm_set_username(const std::string& username);

void lolistorm_set_real_serial(std::string serial);
void lolistorm_set_real_nodeid(unsigned char nodeid[6]);
void lolistorm_set_real_machineid(unsigned char machineid[6]);

const std::string& lolistorm_get_real_serial();
const std::string& lolistorm_get_real_macid_str();
const std::string& lolistorm_get_real_nodeid_str();
const std::string& lolistorm_get_real_machineid_str();

const std::string& lolistorm_get_id0();
const std::string& lolistorm_get_macid();

// SL wants to send out your unhashed identifiers too!
void lolistorm_get_faux_nodeid(unsigned char out[6]);
const std::string& lolistorm_get_faux_nodeid_str();
void lolistorm_get_faux_machineid(unsigned char out[6]);
const std::string& lolistorm_get_faux_machineid_str();

// Replacement functions implemented in their appropriate source files
// The original functions are modified to return the spoofed IDs instead
S32 LLUUID_getNodeID_real(unsigned char* node_id);
S32 LLMachineID_getUniqueID_real(unsigned char* unique_id, size_t len);

void lolistorm_fake_support_info(LLSD& info);

#endif // LOLISTORM_LOSPOOF_H
