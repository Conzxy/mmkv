#ifndef _TERNARY_TREE_H_
#define _TERNARY_TREE_H_

#include <stddef.h>
#include <stdbool.h>

#ifndef INLINE
# if defined(__GNUC__)

#  if (__GNUC__ > 3) || ((__GNUC__ == 3) && (__GNUC_MINOR__ >= 1))
#   define INLINE __inline__ __attribute__((always_inline))
#  else
#   define INLINE __inline__
#  endif

# elif (defined(_MSC_VER) || defined(__WATCOMC__))
#  define INLINE __inline
# else
#  define INLINE 
# endif
#endif

#if (!defined(__cplusplus)) && (!defined(inline))
#define inline INLINE
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _TernaryNode {
  char c;                       /* character */
  bool store;                   /* store the string in the mid */
  struct _TernaryNode *left;    /* lower than the c */
  struct _TernaryNode *mid;     /* equal to the c */
  struct _TernaryNode *right;   /* higher than the c */
} TernaryNode;

/*-----------------------------------------*/
/* Ternary tree API                        */
/*-----------------------------------------*/

/** Callback to process match str of prefix search */
typedef void(*ternary_str_callback)(char const *, void*);

/**
 * \brief Add str to ternary tree
 * \param store Copy the \p str to the node -- true,
 *              Store the address only -- false
 * \return
 *   NULL -- failure
 *   duplicate entry or insearted new entry -- success
 */
char *ternary_add(TernaryNode **root, char const *str, bool store);

/**
 * \brief Search string in the ternary tree
 * \return
 *   NULL -- failure
 *   existed entry -- success
 */
char *ternary_search(TernaryNode const *root, char const *str);

/**
 * \brief Search matching strings in the tree according the prefix
 * \param cb The callback to process match strings
 * \param num The maximum number of match strings you want process
 */
void ternary_search_prefix_num(TernaryNode const *root, char const *prefix, ternary_str_callback cb, void *args, size_t num);

/**
 * \brief Search all matching strings in the tree according the prefix*/
inline void ternary_search_prefix(TernaryNode const *root, char const *prefix, ternary_str_callback cb, void *args) {
  ternary_search_prefix_num(root, prefix, cb, args, (size_t)-1);
}

/**
 * \brief Remove the \p str in the ternary tree
 * \return
 *   true -- success
 *   false -- failure
 */
bool ternary_remove(TernaryNode **root, char const *str);

/**
 * \brief Free the entire tree
 */
void ternary_free(TernaryNode **root);

#ifdef __cplusplus
}
#endif

#endif // _TENARY_TREE_H_
