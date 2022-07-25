#include "response_printer.h"

#include <iostream>

#include "mmkv/protocol/mmbp_response.h"
#include "mmkv/protocol/status_code.h"
#include "mmkv/util/conv.h"

using namespace mmkv::client;
using namespace mmkv;
using namespace mmkv::protocol;

void ResponsePrinter::Printf(Command cmd, MmbpResponse* response) {
  switch (response->status_code) {
    case S_OK: {
      if (response->HasValue()) {
        std::cout << response->value << std::endl;
      } else if (response->HasValues()) {
        auto& values = response->values;
        int64_t n = values.size();
        if (n == 0) {
          std::cout << "{}" << std::endl;
        } else {
          int64_t i;
          std::cout << "{";
          for (i = 0; i < n-1; ++i) {
            std::cout << values[i] << ", ";
          }
          std::cout << values[i] << "}" << std::endl;
        }
      } else if (response->HasCount()) {
        if (cmd == VWEIGHT) {
          std::cout << "(double)" << util::int2double(response->count) << std::endl;
        } else {
          std::cout << "(integer)" << response->count << std::endl;
        }
      } else if (response->HasVmembers()) { 
        auto& wms = response->vmembers;
        size_t order = 0;
        for (auto const& wm : wms) {
          std::cout << "[" << order++ << "]: " << wm.value << "(" << wm.key << ")\n";
        }
      } else if (response->HasKvs()) { 
        unsigned int i = 0;
        for (auto const& kv : response->kvs) {
          std::cout << "[" << i++ << "]: " << "(" << kv.key << ", " << kv.value << ")\n";
        }
      } else {
        std::cout << "Success!" << std::endl;
      }
    }
      break;
      
    default:
      std::cout << GetStatusMessage((StatusCode)response->status_code) << std::endl;

  }
}
