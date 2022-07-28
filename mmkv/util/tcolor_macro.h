#ifndef _MMKV_UTIL_TCOLOR_MACRO_H_
#define _MMKV_UTIL_TCOLOR_MACRO_H_

#ifdef NONE
#undef NONE
#endif
#define NONE "\e[0m"

#ifdef RESET
#undef RESET
#endif
#define RESET "\e[0m"

#ifdef BLACK
#undef BLACK
#endif
#define BLACK "\e[0;30m"

#ifdef L_BLACK
#undef L_BLACK
#endif
#define L_BLACK "\e[1;30m"

#ifdef RED 
#undef RED 
#endif
#define RED "\e[0;31m"

#ifdef L_RED 
#undef L_RED 
#endif
#define L_RED "\e[1;31m"

#ifdef GREEN
#undef GREEN
#endif
#define GREEN "\e[0;32m"


#ifdef L_GREEN
#undef L_GREEN
#endif
#define L_GREEN "\e[1;32m"

#ifdef BROWN
#undef BROWN
#endif
#define BROWN "\e[0;33m"


#ifdef YELLOW
#undef YELLOW
#endif
#define YELLOW "\e[1;33m"

#ifdef BLUE 
#undef BLUE 
#endif
#define BLUE "\e[0;34m"

#ifdef  L_BLUE
#undef  L_BLUE
#endif
#define L_BLUE "\e[1;34m"

#ifdef PURPLE 
#undef PURPLE 
#endif
#define PURPLE "\e[0;35m"

#ifdef L_PURPLE 
#undef L_PURPLE 
#endif
#define L_PURPLE "\e[1;35m"

#ifdef CYAN 
#undef CYAN 
#endif
#define CYAN "\e[0;36m"

#ifdef L_CYAN 
#undef L_CYAN 
#endif
#define L_CYAN "\e[1;36m"

#ifdef GRAY 
#undef GRAY 
#endif
#define GRAY "\e[0;37m"

#ifdef WHITE 
#undef WHITE
#endif
#define WHITE "\e[1;37m"

#ifdef BOLD 
#undef BOLD
#endif
#define BOLD "\e[1m"

#ifdef UNDERLINE 
#undef UNDERLINE
#endif
#define UNDERLINE "\e[4m"

#ifdef BLINK
#undef BLINK
#endif
#define BLINK "\e[5m"


#ifdef REVERSE
#undef REVERSE
#endif
#define REVERSE "\e[7m"


#ifdef HIDE 
#undef HIDE 
#endif
#define HIDE "\e[8m"


#ifdef CLEAR
#undef CLEAR
#endif
#define CLEAR "\e[2J"


#endif