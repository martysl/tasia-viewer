#ifndef LOLISTORM_LOEXTRAS_H
#define LOLISTORM_LOEXTRAS_H

#include "stdtypes.h"

#include <string>

#define LO_CONVENIENCE             0x00000001U
#define LO_BYPASS_EXPORT_PERMS     0x00000002U
#define LO_ENHANCED_EXPORT         0x00000004U
#define LO_ANONYMIZE_EXPORTS       0x00000008U
#define LO_MD5_LOGINS              0x00000010U

#define LO_FEATURE_MASK            0x0000001FU

void lolistorm_set_flags(unsigned flags, unsigned mask);
unsigned lolistorm_get_flags();
unsigned lolistorm_get_mask();

unsigned lolistorm_new_defaulted_flags();

void lolistorm_enable_flag(unsigned flag);
void lolistorm_disable_flag(unsigned flag);
bool lolistorm_check_flag(unsigned flag);

void lolistorm_strip_jpeg2000_comment(std::string&);

void lolistorm_set_custom_ids(const std::string& username, const std::string& id0, const std::string& macid);
void lolistorm_set_custom_id0(const std::string& id0);
void lolistorm_set_custom_macid(const std::string& macid);
const std::string& lolistorm_get_custom_username();
const std::string& lolistorm_get_custom_id0();
const std::string& lolistorm_get_custom_macid();

#endif // LOLISTORM_LOEXTRAS_H
