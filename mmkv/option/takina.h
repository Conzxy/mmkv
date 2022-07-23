#ifndef _TAKINA_TAKINA_H_
#define _TAKINA_TAKINA_H_

#include <string.h>         // ::strlen()
#include <assert.h>
#include <stddef.h>         // size_t
#include <stdio.h>          // ::snprintf()
#include <stdlib.h>         // exit()

#include <string>
#include <utility>          // std::move()
#include <unordered_map>
#include <vector>
#include <array>

// I don't want to introduce std::max()
#define TAKINA_MAX(x, y) (((x) < (y)) ? (y) : (x))

namespace takina {

enum OptType : uint8_t {
  OT_STR = 0,
  OT_FSTR, // fixed string
  OT_MSTR, // Multiple string
  OT_INT,
  OT_FINT,
  OT_MINT,
  OT_DOUBLE,
  OT_FDOUBLE,
  OT_MDOUBLE,
  OT_VOID, // No argument, use a boolean variable to indicates the option is set
};

struct OptionParameter {
  OptType type; // Reinterpret the param field
  void* param;  // pointer to the user-defined varaible
  int size = 0;
};

// Use aggregation initialization to create a OptionDescription object
struct OptionDescption {
  std::string sopt;       // short option
  std::string lopt;       // long option
  std::string desc;       // description of option
  std::string param_name{}; // name of parameters
};

using OptDesc = OptionDescption;

/*
 * !!!!!!
 * To make the libarary to be header-only(since the all functions is inline)
 * I set the all global varaible to static linkage
 * Therefore, I want user can include this file in the main file only(e.g. main.cc).
 * Because, static linkage can define separate entity into the separate traslation-unit and 
 * not violate the ODR rule.
 * i.e. it waste the memory space in the .code segment in the relocatable object file.
 * !!!!!!
 */

// Help message
static std::string help;
static bool has_help = false;

/*
 * To implement the AddUsage() and AddDescription() to position-of-call-independent,
 * split them from help
 * e.g.
 * ```cpp
 *  takina::AddDescription()
 *  takina::AddUsage()
 * ```
 * This is also OK
 */

// Usage message
static std::string usage;

// Description of process
static std::string description;

/*
 * The long_param_map store the parameter object,
 * and the short_param_map just store the pointer to it.
 * Instread approach:
 * ```cpp
 * std::vector<OptionParameter> params;
 * std::unordered_map<std::string, OptionParameter*> long_param_map;
 * std::unordered_map<std::string, OptionParameter*> short_param_map;
 * ```
 *
 * This make the process of long_param_map and short_param_map to same.
 * But the params is not necessary in fact.
 * Therefore, I don't define params.
 */ 

// long option -> OptionParameter
static std::unordered_map<std::string, OptionParameter> long_param_map;

// short option -> OptionParameter
static std::unordered_map<std::string, OptionParameter*> short_param_map;

// When there are some sections at least 1
// section -> { short, long, desc }[]
static std::unordered_map<std::string, std::vector<OptionDescption>> section_opt_map;

// There is no section
// { short, long, desc }[]
static std::vector<OptionDescption> options;

// To make the section output in the FIFO order
// since the section_opt_map is unordered(hash table)
static std::vector<std::string const*> sections;

// Record current section
// Define pointer to avoid redundancy
// static std::string const* cur_section;
// static std::string cur_option;


namespace detail {

void _AddOption(OptDesc&& desc, OptionParameter opt_param);
void _GenHelp();
bool _SetParamater(OptionParameter* param, char const* arg, int cur_arg_num, std::string const& cur_option, std::string* errmsg);

} // namespace detail

inline void AddUsage(std::string const& desc) {
  usage.reserve(7+desc.size()+1);
  usage = "Usage: ";
  usage += desc;
  usage += "\n\n";
}

inline void AddDescription(std::string const& desc) {
  description = desc;
  description += "\n\n";
}

inline void AddSection(std::string&& section) {
  auto res = section_opt_map.insert({std::move(section), {}});

  if (!res.second) {
    ::fprintf(stderr, "The section %s does exists!\n", section.c_str());
    return;
  }
  // FIXME
  //cur_section = &res.first->first;
  sections.emplace_back(&res.first->first);
}

inline void AddOption(OptDesc&& desc, bool* param) {
  *param = false;
  detail::_AddOption(std::move(desc), {OT_VOID, param});
}

#define _DEFINE_ADD_OPTION(_ptype, _type) \
inline void AddOption(OptDesc&& desc, _ptype *param) { \
  detail::_AddOption(std::move(desc), {_type, param}); \
}

_DEFINE_ADD_OPTION(std::string, OT_STR)
_DEFINE_ADD_OPTION(int, OT_INT)
_DEFINE_ADD_OPTION(double, OT_DOUBLE)
_DEFINE_ADD_OPTION(std::vector<std::string>, OT_MSTR)
_DEFINE_ADD_OPTION(std::vector<int>, OT_MINT)
_DEFINE_ADD_OPTION(std::vector<double>, OT_MDOUBLE)

#define _DEFINE_ADD_OPTION_FIXED(_ptype, _type) \
inline void AddOption(OptDesc&& desc, _ptype *param, int n) { \
  detail::_AddOption(std::move(desc), {_type, param, n}); \
}

_DEFINE_ADD_OPTION_FIXED(std::string, OT_FSTR)
_DEFINE_ADD_OPTION_FIXED(int, OT_FINT)
_DEFINE_ADD_OPTION_FIXED(double, OT_FDOUBLE)

#define _CHECK_OPTION_EXISTS(_map) \
          auto iter = _map.find(cur_option); \
          if (iter == _map.end()) { \
            *errmsg = "Option: "; \
            *errmsg += cur_option; \
            *errmsg += " is not an valid option. Please type --help to check all allowed options"; \
            return false; \
          }

#define _VOID_OPTION_PROCESS \
          if (cur_param->type == OT_VOID) { \
            *(bool*)(cur_param->param) = true; \
          }

inline bool CheckArgumentIsLess(OptionParameter* cur_param, std::string const& cur_option, int cur_arg_num, std::string* errmsg) {
  bool condition = false;
  if (cur_param) {
    switch (cur_param->type) {
      case OT_STR:case OT_INT:case OT_DOUBLE:
        condition = cur_arg_num == 0;
      break;
      case OT_FSTR:case OT_FINT:case OT_FDOUBLE:
        condition = cur_arg_num < cur_param->size;
      break;
    }
  }

  if (condition) {
    *errmsg = "Option: ";
    *errmsg += cur_option;
    *errmsg += ", the number of arguments is less than required";
    return false;
  }

  return true;
}

inline bool Parse(int argc, char** argv, std::string* errmsg) {
  // argv[0] is the name of process, just ignore it
  OptionParameter* cur_param = nullptr;
  std::string cur_option;
  int cur_arg_num = 0;
  detail::_GenHelp();  

  for (int i = 1; i < argc; ++i) {
    char const* arg = argv[i];
    const size_t len = ::strlen(arg);

    assert(len != 0);
    // Check if is a option
    // short option or long option
    if (arg[0] == '-' && len > 1) {
      if (!CheckArgumentIsLess(cur_param, cur_option, cur_arg_num, errmsg)) return false;
      cur_arg_num = 0;
      // long option
      if (arg[1] == '-') {
        cur_option = std::string(&arg[2], len-2);
        _CHECK_OPTION_EXISTS(long_param_map)

        cur_param = &iter->second;
        _VOID_OPTION_PROCESS
      } else {
        // short option
        cur_option = std::string(&arg[1], len-1);
        _CHECK_OPTION_EXISTS(short_param_map)

        cur_param = iter->second;
        _VOID_OPTION_PROCESS
      }
    } else {
      // Non option arguments
      cur_arg_num++;
      switch (cur_param->type) {
        case OT_STR:case OT_INT:case OT_DOUBLE:
        if (cur_arg_num > 1) {
          *errmsg = "To unary argument option: ";
          *errmsg += cur_option;
          *errmsg += ", the number of arguments more than one";
          return false;
        }
          break;
      }

      if (!detail::_SetParamater(cur_param, arg, cur_arg_num, cur_option, errmsg)) {
        return false;
      }
    }

    if (has_help) {
      ::fputs(help.c_str(), stdout);
      ::exit(0);
      return true;
    }
  }

  if (!CheckArgumentIsLess(cur_param, cur_option, cur_arg_num, errmsg)) return false;

  return true;
}

inline void DebugPrint() {
#ifdef _TAKINA_DEBUG_
  printf("======= Debug Print =======\n");
  printf("All long options: \n");
  for (auto const& long_opt : long_param_map) {
    printf("--%s\n", long_opt.first.c_str());
  }

  printf("All short options: \n");
  for (auto const& short_opt : short_param_map) {
    printf("-%s\n", short_opt.first.c_str());
  }
  puts("");
#endif
}

namespace detail {

inline void _AddOption(OptDesc&& desc, OptionParameter opt_param) {
  if (desc.lopt.empty()) return;

  auto res = long_param_map.insert({(desc.lopt), opt_param});
  if (!res.second) {
    ::fprintf(stderr, "The long option: %s does exists\n", desc.lopt.c_str());
    return;
  }

  if (!desc.sopt.empty()) {
    if (!short_param_map.insert({(desc.sopt), &res.first->second}).second) {
      ::fprintf(stderr, "The short option: %s does exists\n", desc.sopt.c_str());
      return;
    }

  }

  if (!sections.empty()) {
    section_opt_map[*sections.back()].push_back(std::move(desc));
  } else {
    options.push_back(std::move(desc));
  }
}

inline void _GenHelp() {
  AddOption({"", "help", "Display the help message"}, &has_help);

  help.reserve(usage.size()+description.size());
  help = usage;
  help += description;

  int longest_short_opt_length = 0; 
  int longest_long_opt_param_name_length = 0;

  if (!sections.empty()) {
    for (auto const& section_opt : section_opt_map) {
      auto& options = section_opt.second;
      for (auto const& option : options) {
        longest_long_opt_param_name_length = TAKINA_MAX(longest_long_opt_param_name_length, 
                                                        (int)(option.param_name.size()+option.lopt.size()));
        longest_short_opt_length = TAKINA_MAX(longest_short_opt_length, (int)option.sopt.size());
      }
    }
  } else {
    for (auto const& option : options) {
      longest_long_opt_param_name_length = TAKINA_MAX(longest_long_opt_param_name_length, 
                                                      (int)(option.param_name.size()+option.lopt.size()));
      longest_short_opt_length = TAKINA_MAX(longest_short_opt_length, (int)option.sopt.size());
    }
  }

#define _PUT_OPTIONS(_opts) \
  char buf[4096]; \
  std::string lopt_param; \
  lopt_param.reserve(longest_long_opt_param_name_length+1); \
  for (auto const& opt : _opts) { \
    std::string format; \
    if (opt.sopt.empty()) { \
      format = " %-*s "; \
    } else { \
      format = "-%-*s,"; \
    } \
    format += " --%-*s   %s\n"; \
    lopt_param = opt.lopt; \
    lopt_param += " "; \
    lopt_param += opt.param_name; \
    ::snprintf(buf, sizeof buf, format.c_str(), \
      longest_short_opt_length, opt.sopt.c_str(), \
      longest_long_opt_param_name_length+1, lopt_param.c_str(), \
      opt.desc.c_str()); \
    help += buf; \
  }

  help += "Options: \n\n";
  if (!sections.empty()) {
    for (auto const& section : sections) {
      help += *section;
      help += ":\n";

      auto const& opts = section_opt_map[*section];
      _PUT_OPTIONS(opts)
      help += "\n";
    }
    help.pop_back();
  } else {
    _PUT_OPTIONS(options)
  }
}

#define ERRMSG_SET_COMMON \
        *errmsg = "Option: "; \
        *errmsg += cur_option; \
        *errmsg += '\n';

inline bool _Str2Int(int* param, char const* arg, std::string const& cur_option, std::string* errmsg) {
  char* end = nullptr;
  const auto res = ::strtol(arg, &end, 10);
  if (res == 0 && end == arg) {
    ERRMSG_SET_COMMON
    *errmsg += "syntax error: this is not a valid integer argument";
    return false;
  }
  *param = res;
  return true;
}

inline bool _Str2Double(double* param, char const* arg, std::string const& cur_option, std::string* errmsg) {
  char* end = nullptr;
  const auto res = ::strtod(arg, &end);
  if (res == 0 && end == arg) {
    ERRMSG_SET_COMMON
    *errmsg += "Syntax error: this is not a valid float-pointing number argument";
    return false;
  }
  *param = res;
  return true;
}

inline bool _SetParamater(OptionParameter* param, char const* arg, int cur_arg_num, std::string const& cur_option, std::string* errmsg) {
  switch (param->type) {
    case OT_STR:
      *(std::string*)(param->param) = arg;
      break;
    case OT_INT: {
      int res;
      if (!_Str2Int(&res, arg, cur_option, errmsg)) return false;
      *(int*)(param->param) = res;
    }
      break;
    case OT_DOUBLE: {
      double res;
      if (!_Str2Double(&res, arg, cur_option, errmsg)) return false;
      *(double*)(param->param) = res;
    }
      break;
    case OT_MSTR: {
      ((std::vector<std::string>*)(param->param))->emplace_back(arg);
      break;
    }
      break;
    case OT_MINT: {
      int res;
      if (!_Str2Int(&res, arg, cur_option, errmsg)) return false;
      ((std::vector<int>*)(param->param))->emplace_back(res);
    }
      break;
    case OT_MDOUBLE: {
      double res;
      if (!_Str2Double(&res, arg, cur_option, errmsg)) return false;
      ((std::vector<double>*)(param->param))->emplace_back(res);
    }
      break;

#define _FIXED_ARGUMENTS_ERR_ROUTINE \
      if (cur_arg_num > param->size) { \
        *errmsg = "Option: "; \
        *errmsg += cur_option; \
        *errmsg += ", The number of arguments more than required"; \
        return false; \
      }

    case OT_FSTR: {
      _FIXED_ARGUMENTS_ERR_ROUTINE
      auto arr = (std::string*)(param->param);
      arr[cur_arg_num-1] = arg;
    }
      break;
    case OT_FINT: {
      _FIXED_ARGUMENTS_ERR_ROUTINE
      auto arr = (int*)(param->param);
      int res;
      if (!_Str2Int(&res, arg, cur_option, errmsg)) return false;
      arr[cur_arg_num-1] = res;
    }
      break;
    case OT_FDOUBLE: {
      _FIXED_ARGUMENTS_ERR_ROUTINE
      auto arr = (double*)(param->param);
      double res;
      if (!_Str2Double(&res, arg, cur_option, errmsg)) return false;
      arr[cur_arg_num-1] = res;
    }
      break;
  }

  return true;
}

template<typename T>
inline void _DestroyObject(T* obj) {
  obj->~T();
}

inline void _Teardown(std::string* str) {
  str->clear();
  str->shrink_to_fit();
}

template<typename T>
inline void _Teardown(std::vector<T>* vec) {
  vec->clear();
  vec->shrink_to_fit();
}

template<typename T>
inline void _Teardown(T* cont) {
  cont->clear();
}

} // namespace detail

#define _TAKINA_DESTROY_OBJ(obj) (detail::_DestroyObject(obj))
#define _TAKINA_TEARDOWN(obj) (detail::_Teardown(obj))

inline void Teardown() {
#if 1
  _TAKINA_TEARDOWN(&help);
  _TAKINA_TEARDOWN(&usage);
  _TAKINA_TEARDOWN(&description);
  _TAKINA_TEARDOWN(&long_param_map);
  _TAKINA_TEARDOWN(&short_param_map);
  _TAKINA_TEARDOWN(&section_opt_map);
  _TAKINA_TEARDOWN(&sections);
  _TAKINA_TEARDOWN(&options);
#else
  _TAKINA_DESTROY_OBJ(&help);
  _TAKINA_DESTROY_OBJ(&usage);
  _TAKINA_DESTROY_OBJ(&description);
  _TAKINA_DESTROY_OBJ(&long_param_map);
  _TAKINA_DESTROY_OBJ(&short_param_map);
  _TAKINA_DESTROY_OBJ(&section_opt_map);
  _TAKINA_DESTROY_OBJ(&sections);
  _TAKINA_DESTROY_OBJ(&options);
#endif
}


} // namespace takina

#endif // _TAKINA_TAKINA_H_
