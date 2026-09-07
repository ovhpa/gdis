#include <glib.h>

#define WIN32_DIR 16895

/* rindex() is a BSD extension (equivalent to strrchr()).
 * Declared here for win32 compatibility; not used on macOS/Linux.
 * Will be removed entirely when win32 support is dropped. */
#if defined(_WIN32) || !defined(__STRICT_ANSI__)
extern char *rindex(const char *, int);
#endif

GSList *my_scandir(const char *, int, int);
