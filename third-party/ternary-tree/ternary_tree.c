#include "ternary_tree.h"

#include <stdlib.h> /* malloc(), free(), realloc() */
#include <assert.h>
#include <stddef.h> /* NULL, size_t */
#include <string.h> /* strdup(), strlen() */

typedef TernaryNode Node;

/*--------------------------------------*/
/* Stack API                            */
/*--------------------------------------*/

typedef struct _Stack {
  void **arr; /* Array */
  size_t top; /* Top index */
  size_t cap; /* Capacity */
} Stack;

inline static void stack_init(Stack *stk) {
  stk->arr = NULL;
  stk->top = 0;
  stk->cap = 0;
}

/* Expand to double size larger than old */
inline static size_t stack_get_new_size(Stack *stk) {
  return stk->cap == 0 ? 1 : stk->cap << 1;
}

inline static bool stack_push(Stack *stk, void *e) {
  if (!e) return false;

  if (stk->cap == stk->top) {
    const size_t new_size = stack_get_new_size(stk);
    void *new_arr = realloc(stk->arr, new_size * sizeof(stk->arr[0]));
    if (!new_arr)
      return false;
    stk->cap = new_size;
    stk->arr = (void**)new_arr;
  }

  stk->arr[stk->top++] = e;
  return true;
}

inline static void *stack_pop(Stack *stk) {
  if (stk->top > 0)
    return stk->arr[--stk->top];

  return NULL;
}

inline static void stack_free(Stack *stk) {
  free(stk->arr);
  stk->arr = NULL;
  stk->top = stk->cap = 0;
}

/* Get logical next node of \p node */
inline static Node **ternary_get_next_node(Node *node, char const **str) {
  assert(str);
  assert(*str);
  assert(node);

  const int diff = node->c - **str;
  if (diff < 0)
    return &(node->right);
  else if (diff > 0)
    return &(node->left);

  /* Equal to the current character */
  (*str)++; /* Forward */
  return &(node->mid);
}

inline static Node *ternary_new_node(char c) {
  Node *node = (Node*)malloc(sizeof(Node));
  if (!node) return NULL;

  node->c = c;
  node->store = false;
  node->left = node->mid = node->right = NULL;
  return node;
}

char *ternary_search(TernaryNode const *root, char const *str) {
  if (!str) return NULL;

  int diff = 0;
  while (root) {
    diff = root->c - *str;
    if (diff > 0)
      root = root->left;
    else if (diff < 0)
      root = root->right;
    else {
      /* Allow empty string */
      if (!*str) {
        /* The last node of path, i.e. matching string */
        if (!root->c)
          return (char *)root->mid;
        else break; /* Prefix of another string */
      }
      
      ++str;
      root = root->mid;
    }
  }
  
  /* No such string exists */
  return NULL;
}

char *ternary_add(Node **root, char const *str, bool store) {
  if (!root || !str) return NULL;

  Node **pcur = root;
  Node *cur = NULL;

  /* duplicate or store it in the last node */
  char const *str_dup = str;

  /* Check the str if is a duplicate */
  while ((cur = *pcur)) {
    /** string exists */
    if (!*str && !cur->c) {
      return (char*)(cur->mid);
    }

    pcur = ternary_get_next_node(cur, &str);
  }

  /* str may be a: 
   * * new string with unique suffix
   * * prefix of another string
   *
   * Construct a string path along the mid pointer.
   *
   * The last node store string in its mid pointer.
   * To support the property, it must be a special and
   * unique node.
   * Otherwise, the prefix of another string insert will be
   * difficult.
   *
   * To identify it is the last node, set its stored charater to
   * be 0 that is convenient for checking comparison of node->c and *str
   */
  while ((*pcur = ternary_new_node(*str))) {
    cur = *pcur; 
    if (!*str++) {
      assert(cur->c == 0);
      cur->store = store;
      cur->mid = (Node*)(store ? strdup(str_dup) : str_dup);
      return (char*)str;
    }
    pcur = &cur->mid;
  }

  return NULL;
}

static void search_match_str(Node const *root, ternary_str_callback cb, void *args, size_t *num, size_t max_num) {
  if (!root || *num >= max_num) return;
  
  search_match_str(root->left, cb, args, num, max_num);
  if (!root->c) {
    *num += 1; 
    cb((char const *)root->mid, args);
  } else
    search_match_str(root->mid, cb, args, num, max_num);
  search_match_str(root->right, cb, args, num, max_num);
}

void ternary_search_prefix_num(Node const *root, char const *prefix, ternary_str_callback cb, void *args, size_t num) {
  if (!root || !prefix) return;
  
  /* If prefix is empty string, process all strings */
  if (!*prefix) {
    size_t n = 0;
    search_match_str(root, cb, args, &n, num);
    return;
  }

  int diff = 0;

  while (root) {
    diff = root->c - *prefix;
    if (diff < 0)
      root = root->right;
    else if (diff > 0)
      root = root->left;
    else {
      /* Search matching string in the subtree rooted 
       * by the mid pointer of the node with the last 
       * character of str recursively
       *
       * Because prefix just a slice of existed string
       * in the tree, we must stop when meeting the last
       * character
       */
      root = root->mid;
      if (*(++prefix) == 0) {
        size_t n = 0;
        search_match_str(root, cb, args, &n, num);
        break;
      }
    }
    
    if (!*prefix)
      break;
  }
}

static void ternary_free_(Node *root) {
  if (root->left)
    ternary_free_(root->left);
  if (root->c && root->mid) {
      ternary_free_(root->mid);
  }
  if (root->right)
    ternary_free_(root->right);

  if (root->store)
    free(root->mid);
  free(root);
}

void ternary_free(Node **root) {
  if (!root) return;
  
  if (*root)
    ternary_free_(*root);
  else
    *root = NULL;
}

static bool ternary_node_is_no_child(Node *node) {
  return !(node->left || node->right || node->mid);
}

static void ternary_remove_(Node **root, Stack *stk) {
  /* Free the mid of the last node if insert in STORE mode  */
  assert(stk->top != 0);
  Node **pvictim = (Node **)stack_pop(stk);
  Node *victim = *pvictim;

  assert(!victim->c);
  if (victim->store)
    free(victim->mid);
  victim->mid = NULL; /* Must set NULL to make checking no child normally */

  while (ternary_node_is_no_child(victim)) {
    free(victim);
    
    if (pvictim == root) { 
      *root = NULL;
      return;
    }

    *pvictim = NULL;
    pvictim = (Node **)stack_pop(stk);
    victim = *pvictim;
  }
  
  /* Current, the victim is the first node with child */

  /* The node in a prefix path of another string,
   * we can't free it
   */
  if (victim->mid) return;
  
  if (victim->left && victim->right) {
    /* Transplant:
     *     victim
     *   /       \
     *  left     right
     * / | \
     * * * nil
     * left replace victim and set right
     */
    if (!victim->left->right) {
      victim->left->right = victim->right;
      *pvictim = victim->left;
    } else if (!victim->right->left) {
      /* symmetric case */
      victim->right->left = victim->left;
      *pvictim = victim->right;
    } else return;
    /*
     *     victim
     *    /      \
     *  left     right
     * /  | \
     * *  *  * 
     *
     * If victim->left->right is not NULL, 
     * ratation may be complex.
     *
     * Can't to transplant, just leave in the tree
     * and as a sentinel.
     */
  } else if (victim->right) {
    // left is NULL
    *pvictim = victim->right;
  } else if (victim->left) {
    // right is NULL
    *pvictim = victim->left;
  }

  free(victim);
}

bool ternary_remove(Node **root, char const *str) {
  if (!root || !str) return false;
  
  Node **pcur = root;
  Node *cur = NULL;
  Stack node_stk;
  stack_init(&node_stk);
  
  /* If str exists in the tree,
   * we must push all nodes in the path
   * to the stack since we don't know the longest
   * prefix of str in the tree.
   */
  while ((cur = *pcur)) {
    stack_push(&node_stk, pcur);
    /* The string path has pushed to stack */
    if (!*str && !cur->c) {
      ternary_remove_(root, &node_stk);
      stack_free(&node_stk);
      return true;
    }

    pcur = ternary_get_next_node(cur, &str);
  }
  
  stack_free(&node_stk);
  return false;
}
