// SPDX-LICENSE-IDENTIFIER: Apache-2.0
#include "translator.h"

#include <cstdint>

#include "mmkv/client/information.h"

#include "mmkv/protocol/command.h"
#include "mmkv/protocol/mmbp_request.h"
#include "mmkv/protocol/mmbp_response.h"
#include "mmkv/util/conv.h"
#include "mmkv/util/print_util.h"
#include "mmkv/util/tokenizer.h"

using namespace mmkv::protocol;
using namespace mmkv::util;
using namespace kanon;
using namespace std;

#define SYNTAX_ERROR_ROUTINE                                                   \
  do {                                                                         \
    if (token_iter == tokenizer.end()) {                                       \
      return E_SYNTAX_ERROR;                                                   \
    }                                                                          \
  } while (0)

#define SYNTAX_ERROR_ROUTINE_END                                               \
  do {                                                                         \
    if (token_iter != tokenizer.end()) {                                       \
      return E_SYNTAX_ERROR;                                                   \
    }                                                                          \
  } while (0)

#define TO_MMKV_STRING(token) mmkv::algo::String((token).data(), (token).size())

#define SET_KEY                                                                \
  do {                                                                         \
    SYNTAX_ERROR_ROUTINE;                                                      \
    request->SetKey();                                                         \
    request->key = TO_MMKV_STRING(*token_iter);                                \
    ++token_iter;                                                              \
  } while (0)

#define SET_VALUE                                                              \
  do {                                                                         \
    SYNTAX_ERROR_ROUTINE;                                                      \
    request->SetValue();                                                       \
    request->value = TO_MMKV_STRING(*token_iter);                              \
    ++token_iter;                                                              \
  } while (0)

#define SET_VALUES                                                             \
  do {                                                                         \
    SYNTAX_ERROR_ROUTINE;                                                      \
    StrValues values;                                                          \
    for (;;) {                                                                 \
      values.push_back(TO_MMKV_STRING(*token_iter));                           \
      ++token_iter;                                                            \
      if (token_iter == tokenizer.end()) break;                                \
    }                                                                          \
    request->SetValues();                                                      \
    request->values = std::move(values);                                       \
    ++token_iter;                                                              \
  } while (0)

#define SET_INTEGER(var, msg)                                                  \
  do {                                                                         \
    SYNTAX_ERROR_ROUTINE;                                                      \
    char buf[64];                                                              \
    auto token = *token_iter;                                                  \
    ::memcpy(buf, token.data(), token.size());                                 \
    buf[token.size()] = 0;                                                     \
    char *end = NULL;                                                          \
    (var) = ::strtoull(buf, &end, 10);                                         \
    if ((var) == 0 && buf == end) {                                            \
      ErrorPrintf(msg);                                                        \
      return E_SYNTAX_ERROR;                                                   \
    }                                                                          \
    ++token_iter;                                                              \
  } while (0)

#define SET_SIGN_INTEGER(var, msg)                                             \
  do {                                                                         \
    SYNTAX_ERROR_ROUTINE;                                                      \
    char buf[64];                                                              \
    auto token = *token_iter;                                                  \
    ::memcpy(buf, token.data(), token.size());                                 \
    buf[token.size()] = 0;                                                     \
    char *end = NULL;                                                          \
    (var) = ::strtol(buf, &end, 10);                                           \
    if ((var) == 0 && buf == end) {                                            \
      ErrorPrintf(msg);                                                        \
      return E_SYNTAX_ERROR;                                                   \
    }                                                                          \
    ++token_iter;                                                              \
  } while (0)

#define SET_DOUBLE(var, msg)                                                   \
  do {                                                                         \
    char buf[128];                                                             \
    auto token = *token_iter;                                                  \
    ::memcpy(buf, (token).data(), (token).size());                             \
    buf[(token).size()] = 0;                                                   \
    char *end;                                                                 \
    (var) = ::strtod(buf, &end);                                               \
    if ((var) == 0 && end == buf) {                                            \
      ErrorPrintf(msg);                                                        \
      return E_SYNTAX_ERROR;                                                   \
    }                                                                          \
    ++token_iter;                                                              \
  } while (0)

Translator::ErrorCode Translator::Parse(MmbpRequest *request, Command cmd,
                                        StringView statement)
{
  Tokenizer tokenizer(statement);
  request->command = cmd;
  auto token_iter = tokenizer.begin();

  switch (GetCommandFormat(cmd)) {
    case F_NONE: {
      if (token_iter != tokenizer.end()) {
        return E_SYNTAX_ERROR;
      }
    } break;
    case F_VALUE: {
      SET_KEY;
      SET_VALUE;
      SYNTAX_ERROR_ROUTINE_END;
    } break;
    case F_ONLY_KEY: {
      SET_KEY;
      SYNTAX_ERROR_ROUTINE_END;
    } break;
    case F_VALUES: {
      SET_KEY;
      SET_VALUES;
    } break;

    case F_SET_OP: {
      SET_KEY;
      SET_VALUE;
      SYNTAX_ERROR_ROUTINE_END;
    } break;

    case F_SET_OP_TO: {
      SET_VALUES;
      if (request->values.size() > 3) {
        return E_SYNTAX_ERROR;
      }
    } break;

    case F_FIELD_VALUE: {
      SET_KEY;

      auto &values = request->values;
      values.reserve(2);
      SYNTAX_ERROR_ROUTINE;
      values.push_back(TO_MMKV_STRING(*token_iter));
      SYNTAX_ERROR_ROUTINE;
      values.push_back(TO_MMKV_STRING(*token_iter));
      SYNTAX_ERROR_ROUTINE_END;
      request->SetValues();
    } break;

    case F_RANGE: {
      SET_KEY;
      SET_SIGN_INTEGER(request->range.left,
                       "ERROR: left bound of range is invalid");
      SET_SIGN_INTEGER(request->range.right,
                       "ERROR: right bound of range is invalid");
      request->SetRange();
      SYNTAX_ERROR_ROUTINE_END;
    } break;
    case F_COUNT: {
      SET_KEY;
      // FIXME uint64_t
      uint32_t count;
      SET_INTEGER(count, "ERROR: count is invalid");
      request->SetCount();
      request->count = count;
      SYNTAX_ERROR_ROUTINE_END;
    } break;
    case F_VSET_MEMBERS: {
      SET_KEY;
      auto &vms = request->vmembers;

      while (++token_iter != tokenizer.end()) {
        double weight;
        SET_DOUBLE(weight, "ERROR: invalid weight");
        SYNTAX_ERROR_ROUTINE;
        auto member = TO_MMKV_STRING(*token_iter);
        vms.push_back({weight, std::move(member)});
      }
      request->SetVmembers();
    } break;
    case F_DRANGE: {
      SET_KEY;

      double drange[2];
      SYNTAX_ERROR_ROUTINE;
      SET_DOUBLE(drange[0], "ERROR: left weight of range is invalid");
      SYNTAX_ERROR_ROUTINE;
      SET_DOUBLE(drange[1], "ERROR: right weight of range is invalid");
      SYNTAX_ERROR_ROUTINE_END;
      request->SetWeightRange(drange[0], drange[1]);
    } break;
    case F_MAP_VALUES: {
      SET_KEY;
      auto &kvs = request->kvs;
      while (++token_iter != tokenizer.end()) {
        StrKeyValue kv;
        kv.key = TO_MMKV_STRING(*token_iter);
        SYNTAX_ERROR_ROUTINE;
        kv.value = TO_MMKV_STRING(*token_iter);
        kvs.push_back(std::move(kv));
      }
      request->SetKvs();
    } break;
    case F_EXPIRE: {
      SET_KEY;
      SET_INTEGER(request->expire_time, "ERROR: expiration time is invalid");
      request->SetExpireTime();
      SYNTAX_ERROR_ROUTINE_END;
    } break;
    case F_MUL_KEYS: {
      SET_VALUES;
    } break;
    case F_INVALID:
    default:
      assert(false && "This must be a valid command");
  }

  request->DebugPrint();
  return E_OK;
}
