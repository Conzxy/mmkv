#include "log_command.h"

namespace disk = mmkv::disk;

using namespace disk;
using namespace disk::detail;
using namespace mmkv::protocol;

CommandType detail::g_command_type[COMMAND_NUM];

int InitCommandType() noexcept {
  for (size_t i = 0; i < COMMAND_NUM; ++i) {
    g_command_type[i] = CT_READ;
  }

#define REGISTER_WRITE_CMD(_cmd) \
  g_command_type[_cmd] = CT_WRITE

  REGISTER_WRITE_CMD(STR_ADD);
  REGISTER_WRITE_CMD(STR_DEL);
  REGISTER_WRITE_CMD(STR_SET);
  REGISTER_WRITE_CMD(STRAPPEND);
  REGISTER_WRITE_CMD(STRPOPBACK);
  REGISTER_WRITE_CMD(LADD);
  REGISTER_WRITE_CMD(LAPPEND);
  REGISTER_WRITE_CMD(LPREPEND);
  REGISTER_WRITE_CMD(LPOPFRONT);
  REGISTER_WRITE_CMD(LPOPBACK);
  REGISTER_WRITE_CMD(LDEL);
  REGISTER_WRITE_CMD(VADD);
  REGISTER_WRITE_CMD(VDELM);
  REGISTER_WRITE_CMD(VDELMRANGE);
  REGISTER_WRITE_CMD(VDELMRANGEBYWEIGHT);
  REGISTER_WRITE_CMD(MADD);
  REGISTER_WRITE_CMD(MDEL);
  REGISTER_WRITE_CMD(SADD);
  REGISTER_WRITE_CMD(SDELM);
  REGISTER_WRITE_CMD(DEL);
  REGISTER_WRITE_CMD(RENAME);

  return 0;
}

static int _dummy = InitCommandType();