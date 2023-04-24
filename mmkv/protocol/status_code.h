// SPDX-LICENSE-IDENTIFIER: Apache-2.0
#ifndef _MMKV_PROTOCOL_ERROR_CODE_H_
#define _MMKV_PROTOCOL_ERROR_CODE_H_

#include <stdint.h>

#ifdef S_OK
#  undef S_OK
#endif

namespace mmkv {
namespace protocol {

enum StatusCode : uint8_t {
  S_OK = 0,            /** OK */
  S_EXISTS,            /** Key exists, can't add */
  S_NONEXISTS,         /** Key doesn't exists, can't search and update */
  S_INVALID_MESSAGE,   /** Invalid format of message of mmbp protocol */
  S_INVALID_RANGE,     /** Invalid range, can't range query */
  S_VMEMBER_NONEXISTS, /** The member doesn't exists in the vset */
  S_EXISITS_DIFF_TYPE, /** Key exists but with different type(i.e. not expected)
                        */
  S_FIELD_NONEXISTS,   /** The field doesn't exists in the map */
  S_SET_MEMBER_NONEXISTS, /** The member doesn't exists in the set */
  S_INVALID_REQUEST,      /** Not satisfy the command required fields */
  S_SET_NO_MEMBER,        /** No member in the set */
  S_DEST_EXISTS,          /** Destination set already exists */
  S_EXPIRE_DISABLE,       /** Expiration is disable */
};

/**
 * \brief Get the message of status code
 * e.g.
 * If this is a error, will return error message: "ERROR: ..."
 */
char const *GetStatusMessage(StatusCode code) noexcept;

/**
 * \brief Translate the status code to string trivially
 */
char const *StatusCode2Str(StatusCode code) noexcept;

} // namespace protocol
} // namespace mmkv

#endif // _MMKV_PROTOCOL_ERROR_CODE_H_
