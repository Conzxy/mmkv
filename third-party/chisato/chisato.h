#ifndef _CHISATO_CHISATO_H_
#define _CHISATO_CHISATO_H_

#include <string.h>         // memcmp
#include <assert.h>
#include <stdint.h>

#include <functional>       // std::function
#include <string>
#include <unordered_map>

#include "str_slice.h"

namespace chisato {

/* Handle user-defined config type field
 * void* is the generic parameter */
typedef bool(*ConfigCallback)(StrSlice, void*);
/* lambda or std::bind can capture arguments */
typedef std::function<bool(StrSlice)> ConfigFunction;

/** Add string config field */
void AddConfig(char const *field, std::string *str);
/** Add int config field */
void AddConfig(char const *field, int *i);
/** Add long config field */
void AddConfig(char const *field, long *l);
/** Add floating-point config field */
void AddConfig(char const *field, double *d);
/** Add boolean config field */
void AddConfig(char const *field, bool *b);

/**
 * User-defined config type 
 * Callback is C style 
 * \param field Config field name
 * \param args Generic arguments of \p cb
 * \param cb Callback to handle \p field
 */
void AddConfig(char const *field, void *args, ConfigCallback cb);

/* User-defined config type
 * Callback style is std::function<>
 */
void AddConfig(char const *field, ConfigFunction cb);

/**
 * \brief Parse the config file
 * \param path Path of config file
 * \param errmsg Error message store
 * \return 
 *  indicates sucess or failure
 */
bool Parse(char const *path, std::string &errmsg);

inline bool Parse(std::string const& path, std::string &errmsg) {
  return Parse(path.c_str(), errmsg);
}

/** Free all resources used for parsing config */
void Teardown();

void DebugPrint();

} // namespace chisato

#endif // _CHISATO_CHISATO_H_
