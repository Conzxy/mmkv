#include "response_printer.h"

#include <iostream>

#include "mmkv/protocol/mmbp_response.h"
#include "mmkv/protocol/status_code.h"

using namespace mmkv::client;
using namespace mmkv::protocol;

void ResponsePrinter::Printf(MmbpResponse* response) {
  switch (response->status_code) {
    case S_OK: {
      if (response->HasValue()) {
        std::cout << response->value;
      } else if (response->HasValues()) {
        auto& values = response->values;
        std::cout << "{";
        size_t i;
        for (i = 0; i < values.size()-1; ++i) {
          std::cout << values[i] << ", ";
        }
        std::cout << values[i] << "}";
      } else if (response->HasCount()) {
        std::cout << response->count;
      } else {
        std::cout << "Success!";
      }
    }
      break;
      
    default:
      std::cout << GetStatusMessage((StatusCode)response->status_code);
  }

  std::cout << std::endl;
}
