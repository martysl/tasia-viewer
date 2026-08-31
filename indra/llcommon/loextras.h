#ifndef LOLISTORM_LOEXTRAS_H
#define LOLISTORM_LOEXTRAS_H

#include "stdtypes.h"

#include <string>

// Tasia: removed Lostorm feature flags (LO_CONVENIENCE, LO_BYPASS_EXPORT_PERMS, LO_ENHANCED_EXPORT, LO_ANONYMIZE_EXPORTS, LO_MD5_LOGINS)
// These were Lostorm viewer leftovers. Core spoofing/ID functions remain.
// Defined as 0 for compile compatibility with existing code
#define LO_CONVENIENCE             0x00000000U
#define LO_BYPASS_EXPORT_PERMS     0x00000000U
#define LO_ENHANCED_EXPORT         0x00000000U
#define LO_ANONYMIZE_EXPORTS       0x00000000U
#define LO_MD5_LOGINS              0x00000000U

#define LO_FEATURE_MASK            0x00000000U

// Stubs for removed flag API (always return false/empty)
void lolistorm_set_flags(unsigned flags, unsigned mask);
unsigned lolistorm_get_flags();
unsigned lolistorm_get_mask();

unsigned lolistorm_new_defaulted_flags();

void lolistorm_block_flag(unsigned flag);
void lolistorm_unblock_flag(unsigned flag);
void lolistorm_enable_flag(unsigned flag);
void lolistorm_disable_flag(unsigned flag);
bool lolistorm_check_flag(unsigned flag);
bool lolistorm_check_block(unsigned flag);

void lolistorm_strip_jpeg2000_comment(std::string&);

// Tasia extras password gate.
void lolistorm_set_password(const std::string& password_hash);
bool lolistorm_extras_unlocked();
bool lolistorm_unlock_extras(const std::string& password_hash);
void lolistorm_lock_extras();

// Tasia: hard-block dangerous extras when connected to Second Life (Agni/Aditi).
void lolistorm_set_secondlife(bool is_secondlife);

void lolistorm_set_custom_ids(const std::string& username, const std::string& id0, const std::string& macid);
void lolistorm_set_custom_id0(const std::string& id0);
void lolistorm_set_custom_macid(const std::string& macid);
const std::string& lolistorm_get_custom_username();
const std::string& lolistorm_get_custom_id0();
const std::string& lolistorm_get_custom_macid();

#endif // LOLISTORM_LOEXTRAS_H
