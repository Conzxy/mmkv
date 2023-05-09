// SPDX-LICENSE-IDENTIFIER: Apache-2.0
#include "mmkv/sharder/util.h"
#include "mmkv/storage/db.h"
#include "mmkv/protocol/mmbp_util.h"
#include "mmkv/protocol/mmbp_request.h"
#include "mmkv/algo/string.h"
#include "util.h"

using namespace mmkv::algo;
using namespace mmkv;
using namespace mmkv::db;
using namespace mmkv::protocol;
using namespace kanon;

void mmkv::SerializeMmbpDataToSharderRequest(
    MmkvDb                            *p_db,
    std::vector<const String *> const &keys,
    size_t                            *p_index,
    ShardMessage                      *p_msg
)
{
  db::DataType data_type;
  Buffer       buffer;
  for (; *p_index < keys.size(); ++*p_index) {
    MmbpRequest mmbp_req;

    auto &key = *keys[*p_index];
    p_db->Type(key, data_type);
    switch (data_type) {
      case DataType::D_STRING: {
        String *p_str;
        auto    code = p_db->GetStr(key, p_str);
        if (S_OK == code) {
          mmbp_req.command = Command::STR_ADD;
          mmbp_req.key     = key;
          mmbp_req.SetKey();
          mmbp_req.value = *p_str;
          mmbp_req.SetValue();

        } else {
          continue;
        }
      } break;

        // FIXME Complete
      default:
        assert(false);
    }

    mmbp_req.SerializeTo(buffer);
    p_msg->set_data_num(p_msg->data_num() + 1);
    p_msg->mutable_data()->append(buffer.GetReadBegin(), buffer.GetReadableSize());
    // if (p_msg->data().size() > (1 << 16)) {
    //   break;
    // }
    buffer.AdvanceAll();
  }
}
