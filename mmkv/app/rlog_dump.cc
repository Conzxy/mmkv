#include <iostream>

#include "mmkv/disk/file.h"
#include "mmkv/disk/log_command.h"

#include "mmkv/option/takina.h"
#include "mmkv/disk/stat.h"

#include "mmkv/util/conv.h"
#include "mmkv/util/str_util.h"
#include "mmkv/protocol/mmbp_request.h"

using namespace mmkv::disk;
using namespace mmkv::protocol;
using namespace mmkv;

struct Option {
  bool clear;
  bool size;
  std::string file_location = "/tmp/.mmkv-request.log";
};

Option g_option;

static constexpr int BUFFER_SIZE = 1 << 16;

void PrintRequest(MmbpRequest const& req);

int main(int argc, char** argv) {
  takina::AddUsage("./rlogdump [OPTIONS]");
  takina::AddDescription("This is a parser of mmkv request log");
  takina::AddOption({"", "clear", "Clear the request log"}, &g_option.clear);
  takina::AddOption({"s", "size", "Size of log file"}, &g_option.size);
  takina::AddOption({"f", "file", "File location(default: /tmp/.mmkv-request.log)", "LOCATION"}, &g_option.file_location);

  std::string errmsg;
  if (!takina::Parse(argc, argv, &errmsg)) {
    ::printf("Failed to parse the options: %s\n", errmsg.c_str()); 
    return 0;
  }

  if (g_option.clear) {
    std::string cmd;
    util::StrCat(cmd, "cat %a > %a", g_option.file_location.c_str(), g_option.file_location.c_str());
    ::system(cmd.c_str());
    return 0;
  }

  File file;
  if (!file.Open(g_option.file_location, File::READ)) {
    fprintf(stderr, "Failed to open file: %s\n", g_option.file_location.c_str());
    return 0;
  }

  Buffer buffer;
  buffer.ReserveWriteSpace(BUFFER_SIZE);

  size_t request_num = 0;
  size_t n = 0;
  MmbpRequest request;
  while ( ( n = file.Read(buffer.GetWriteBegin(), buffer.GetWritableSize()) ) != (size_t)-1) {
    LOG_DEBUG << "Read " << n << " bytes";
    buffer.AdvanceWrite(n);
    LOG_DEBUG << "Readable size = " << buffer.GetReadableSize();
    while (buffer.GetReadableSize() >= sizeof(uint32_t)) {
      auto size = buffer.Read32();
      LOG_DEBUG << "size header = " << size;
      if (buffer.GetReadableSize() >= size - sizeof(size)) {
        request_num++;
        request.ParseFrom(buffer);
        PrintRequest(request);
        assert(GetCommandType((Command)request.command) == CT_WRITE);
      } else {
        break;
      }
    }

    buffer.ReserveWriteSpace(BUFFER_SIZE);
    if (n < BUFFER_SIZE) break;
  }

  std::cout << "\nTotal number of requests: " << request_num << std::endl;
}

inline void PrintRequest(MmbpRequest const& req) {
  std::cout << "Command: " << GetCommandString((Command)req.command) << "\n";
  if (req.HasKey()) {
    std::cout << "Key: " << req.key << "\n";
  }

  if (req.HasValue()) {
    std::cout << "Value: " << req.value << "\n";
  } else if (req.HasValues()) {
    std::cout << "Values: \n";
    for (auto const& value : req.values)
      std::cout << value << "\n";
  } else if (req.HasKvs()) {
    std::cout << "KeyValues: \n";
    for (auto const& kv : req.kvs)
      std::cout << "<" << kv.key << ", " << kv.value << ">\n";
  } else if (req.HasRange()) {
    std::cout << "Range: [" << req.range.left << "," << req.range.right << ")\n";
    std::cout << "DRange: [" << util::int2double(req.range.left) << ", " << util::int2double(req.range.right) << "]\n";
  } else if (req.HasCount()) {
    std::cout << "Count: " << req.count << "\n";
  } else if (req.HasVmembers()) {
    std::cout << "<Weight, Member>: \n";
    for (auto const& wm : req.vmembers)
      std::cout << "(" << wm.key << "," << wm.value << ")\n";
  }

  if (req.HasExpireTime()) {
    std::cout << "ExpireTime: " << req.expire_time << "\n";
  }

}