// SPDX-LICENSE-IDENTIFIER: Apache-2.0
#ifndef _MMKV_UTIL_TCOLOR_MACRO_H_
#define _MMKV_UTIL_TCOLOR_MACRO_H_

#ifdef NONE
#  undef NONE
#endif
#define NONE "\033[0m"

#ifdef RESET
#  undef RESET
#endif
#define RESET "\033[0m"

#ifdef BLACK
#  undef BLACK
#endif
#define BLACK "\033[0;30m"

#ifdef L_BLACK
#  undef L_BLACK
#endif
#define L_BLACK "\033[1;30m"

#ifdef RED
#  undef RED
#endif
#define RED "\033[0;31m"

#ifdef L_RED
#  undef L_RED
#endif
#define L_RED "\033[1;31m"

#ifdef GREEN
#  undef GREEN
#endif
#define GREEN "\033[0;32m"

#ifdef L_GREEN
#  undef L_GREEN
#endif
#define L_GREEN "\033[1;32m"

#ifdef BROWN
#  undef BROWN
#endif
#define BROWN "\033[0;33m"

#ifdef YELLOW
#  undef YELLOW
#endif
#define YELLOW "\033[1;33m"

#ifdef BLUE
#  undef BLUE
#endif
#define BLUE "\033[0;34m"

#ifdef L_BLUE
#  undef L_BLUE
#endif
#define L_BLUE "\033[1;34m"

#ifdef PURPLE
#  undef PURPLE
#endif
#define PURPLE "\033[0;35m"

#ifdef L_PURPLE
#  undef L_PURPLE
#endif
#define L_PURPLE "\033[1;35m"

#ifdef CYAN
#  undef CYAN
#endif
#define CYAN "\033[0;36m"

#ifdef L_CYAN
#  undef L_CYAN
#endif
#define L_CYAN "\033[1;36m"

#ifdef GRAY
#  undef GRAY
#endif
#define GRAY "\033[0;37m"

#ifdef WHITE
#  undef WHITE
#endif
#define WHITE "\033[1;37m"

#ifdef BOLD
#  undef BOLD
#endif
#define BOLD "\033[1m"

#ifdef UNDERLINE
#  undef UNDERLINE
#endif
#define UNDERLINE "\033[4m"

#ifdef BLINK
#  undef BLINK
#endif
#define BLINK "\033[5m"

#ifdef REVERSE
#  undef REVERSE
#endif
#define REVERSE "\033[7m"

#ifdef HIDE
#  undef HIDE
#endif
#define HIDE "\033[8m"

#ifdef CLEAR
#  undef CLEAR
#endif
#define CLEAR "\033[2J"

#endif