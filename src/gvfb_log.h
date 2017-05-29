#ifndef _GVFB_LOG_H_
#define _GVFB_LOG_H_

#define LEVEL_0 0
#define LEVEL_1 1
#define LEVEL_2 2

#ifndef msg_out

#ifdef SHOW_DBG_MSG
#ifdef SHOW_FULL_DBG_MSG
#define msg_out(_level,...)                      \
    do {                                         \
        printf ("[%s]%d: ", __FILE__, __LINE__); \
        printf (__VA_ARGS__);                    \
        printf ("\n");                           \
    }                                            \
    while (0)
#else /* SHOW_FULL_DBG_MSG */
#define msg_out(_level,...)    \
    do {                       \
        printf (__VA_ARGS__);  \
        printf ("\n");         \
    }                          \
    while (0)
#endif /* SHOW_FULL_DBG_MSG */
#else  /* SHOW_DBG_MSG */
#define msg_out(_level,...)
#endif /* SHOW_DBG_MSG */

#endif

#endif /* _GVFB_LOG_H_ */

