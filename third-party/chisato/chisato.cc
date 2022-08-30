#include "chisato.h"

#define CHISATO_DEBUG__
#ifdef CHISATO_DEBUG__
#include <iostream>
#endif

#include <algorithm> // all_of

#define MAX_LINE_LEN 4096

namespace chisato {

/** 
 * Support integer, floating-point, and string
 */
enum ConfigType : uint8_t {
  CT_INT = 0, /** integer */
  CT_LONG,    /** long    */
  CT_FLOAT,   /** floating-point */
  CT_STR,     /** string */
  CT_BOOL,    /** boolean */
  CT_USR_DEF, /** user-defined type */
};

/** Callback with generic parameters */
struct ConfigClosure {
  ConfigCallback config_cb = nullptr;
  void *args = nullptr;
};

/** Config metadata */
struct ConfigData {
  ConfigType type;           /** interpret data */

  enum : uint8_t {
    CD_RAW = 0,      /** predefined data */
    CD_C_CALLBACK,   /** C callback */
    CD_CPP_CALLBACK, /** Cpp callback */
  } set_method;
  
  /* Decrease the space usage */
  union {
    void *data;                /** Binding the field data */
    ConfigClosure config_cs;   /** C style callback */
    ConfigFunction config_fn;  /** Cpp style callback */
  };

  ConfigData(ConfigType t, void *d)
    : type(t)
    , set_method(CD_RAW)
    , data(d)
  {
  }

  ConfigData(ConfigType t, ConfigClosure cc)
    : type(t)
    , set_method(CD_C_CALLBACK)
    , config_cs(cc)
  {
  }
  
  ConfigData(ConfigType t, ConfigFunction fn)
    : type(t)
    , set_method(CD_CPP_CALLBACK)
    , config_fn(fn)
  {
  }

  ConfigData(ConfigData && other) noexcept 
    : type(other.type)
    , set_method(other.set_method)
  {
    switch (other.set_method) {
      case CD_RAW:
        data = other.data;
        break;
      case CD_C_CALLBACK:
        config_cs = other.config_cs;
        break;
      case CD_CPP_CALLBACK:
        config_fn = std::move(other.config_fn);
        break;
    }
  }
  
  ~ConfigData() noexcept {
    if (set_method == CD_CPP_CALLBACK) {
      config_fn.~ConfigFunction();
    }
  }
};


namespace detail {

struct StrSliceHash {
  size_t operator()(StrSlice str) const noexcept {
    size_t h = 5321;
    for (size_t i = 0; i < str.size(); ++i) {
      h = (h << 5) + h + str[i];
    }

    return h;
  }
};

} // detail

using ConfigMap = std::unordered_map<StrSlice, ConfigData, detail::StrSliceHash>;

/** <Config name> -> <Config metadata> */
static ConfigMap config_map;

/* Read a line from file */
static bool ReadLine(FILE* file, std::string& line, const bool need_newline=true) ;

#define STR2INT(ivar) \
  char *end = nullptr; \
  long res = strtol(value.data(), &end, 10); \
  if (res == 0 && end == value.data()) { \
    errmsg += "INVALID integer value of "; \
    errmsg.append(field.data(), field.size()); \
    return false; \
  } \
  (ivar) = res;

/** Add config */
static inline void AddConfig_(char const *field, ConfigData data) {
  config_map.emplace(field, std::move(data));
}

void AddConfig(char const *field, std::string *str) {
  AddConfig_(field, ConfigData{ CT_STR, str });
}

void AddConfig(char const *field, int *i) {
  AddConfig_(field, ConfigData{ CT_INT, i });
}

void AddConfig(char const *field, long *l) {
  AddConfig_(field, ConfigData{ CT_LONG, l });
}

void AddConfig(char const *field, double * d) {
  AddConfig_(field, ConfigData{ CT_FLOAT, d });
}

void AddConfig(char const *field, bool *b) {
  AddConfig_(field, ConfigData{ CT_BOOL, b });
}

void AddConfig(char const *field, void *args, ConfigCallback cb) {
  config_map.emplace(field, ConfigData {
    CT_USR_DEF,
    ConfigClosure {
      .config_cb = cb,
      .args = args,
    }
  });
}

void AddConfig(char const *field, ConfigFunction cb) {
  config_map.emplace(field, ConfigData {
    CT_USR_DEF,
    std::move(cb),
  });
}

bool Parse(char const *path, std::string &errmsg) {
  FILE* file = ::fopen(path, "r");
  errmsg.clear();

  if (!file) {
    errmsg += "File error: Failed to open the config file: ";
    errmsg += path;
    errmsg += "\nError message: ";
    errmsg += ::strerror(errno);
    return false;
  }

  std::string line;
  
  size_t lineno = 0; 
  /* FIXME Use state machine? */
  while (ReadLine(file, line, false)) {
    ++lineno;
    /* Get the comment position */
    size_t end_pos = -1;
    for (size_t i = line.size(); i > 0; --i) {
      if (line[i-1] == '#') {
        end_pos = i-1;
      }
    }
    
    if (end_pos != (size_t)-1)
      line.erase(end_pos, line.size()); 

    /** Get the colon position */ 
    size_t start_pos = 0;
    end_pos = line.find(':');
    if (end_pos != std::string::npos) {
      // Find the first position whose character is not space 
      for (; line[start_pos] == ' ' && start_pos != end_pos; ++start_pos);

      StrSlice field(line.data() + start_pos,
                     end_pos - start_pos);
      
      // Get the value slice
      for (start_pos = end_pos+1; line[start_pos] == ' ' && start_pos != line.size(); ++start_pos);
      
      StrSlice value(line.data() + start_pos,
                     line.size() - start_pos);
      
      auto iter = config_map.find(field);
      ConfigData *data = nullptr;
      if (iter != config_map.end()) {
        data = &iter->second;
        
        switch (data->set_method) {
          case ConfigData::CD_RAW: {
            switch (data->type) {
              case CT_INT: {
                STR2INT(*(int*)(data->data))
              }
              break;

              case CT_LONG: {
                STR2INT(*(long*)(data->data))
              }
              break;

              case CT_FLOAT: {
                char *end = nullptr;
                double res = strtod(value.data(), &end);
                if (res == 0 && end == value.data()) {
                  errmsg += "INVALID floating-point value of ";
                  errmsg.append(field.data(), field.size());
                }
                *(double*)(data->data) = res;
              }
              break;

              case CT_STR: {
                ((std::string*)(data->data))->assign(value.data(), value.size());
              }
              break;
              
              case CT_BOOL: {
                auto boolean_field = (bool*)(data->data);
                if (value.caseCmp(StrSlice("on", 2)) == 0) {
                  *boolean_field = true;
                } else if (value.caseCmp(StrSlice("off", 3)) == 0) {
                  *boolean_field = false;
                } else {
                  errmsg += "Syntax error: Invalid boolean field, "
                    "must be 'on' or 'off'(case-intensive)";
                  return false;
                }
              }
              break;

              case CT_USR_DEF: {
                assert(false && "Set callback to handle user-defined config type");
              }
              break;
            }
          }
          break;

          case ConfigData::CD_C_CALLBACK: {
            if (data->config_cs.config_cb && !data->config_cs.config_cb(value, data->config_cs.args)) {
              errmsg += "Value error: Invalid value of field '";
              errmsg.append(field.data(), field.size());
              errmsg += "'";
              return false;
            }
          }
          break;

          case ConfigData::CD_CPP_CALLBACK: {
            if (data->config_fn && !data->config_fn(value)) {
              errmsg += "Value error: Invalid value of field '";
              errmsg.append(field.data(), field.size());
              errmsg += "'";
              return false;
            }
          }
          break;
        } 
      } else {
        errmsg += path;
        errmsg += ':';
        errmsg += std::to_string(lineno);
        errmsg += "\nField error: Unknown field '";
        errmsg.append(field.data(), field.size());
        errmsg += '\'';
        return false;
      }
    } else if (!line.empty() && !std::all_of(line.begin(), line.end(), [](char c)  { return c == ' '; })) {
      errmsg += path;
      errmsg += ':';
      errmsg += std::to_string(lineno);
      errmsg += "\nSyntax error: No colon to split field and value";
      return false;
    }
  }

  if (::feof(file) == 0) {
    errmsg += "File error: Failed to read line ";
    errmsg += std::to_string(lineno);
    errmsg += " from ";
    errmsg += path;
    errmsg += "\nError Message: ";
    errmsg += strerror(errno);
    return false;
  }

  return true;
}

void Teardown() {
  config_map.clear();
}

void DebugPrint() {
#ifdef _CHISATO_DEBUG__
  for (auto const &config : config_map) {
    std::cout << std::string(config.first.data(), config.first.size()) << "\n";
  }
#endif
}

static inline bool ReadLine(FILE* file, std::string& line, const bool need_newline) {
  char buf[MAX_LINE_LEN];
  char* ret = NULL;
  size_t n = 0;

  line.clear();

  do {
    ret = ::fgets(buf, sizeof buf, file);

    if (ret == NULL) {
      if (::ferror(file)) {
        if (errno == EINTR) {
          continue;
        }
      }

      return false;
    }

    assert(ret == buf);
    n = strlen(buf);

    if (n >= 1 && buf[n-1] == '\n') {
      if (!need_newline) {
        if (n >= 2 && buf[n-2] == '\r') {
          line.append(buf, n - 2);
        }
        else {
          line.append(buf, n - 1);
        }
      } else {
        line.append(buf, n);
      }
      break;
    }

    line.append(buf, n);

  } while (n == (sizeof(buf) - 1));

  return true;
}

} // chisato
