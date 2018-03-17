#ifndef _GTAGS_H_
#define _GTAGS_H_

#include <geanyplugin.h>

struct tag {
	gchar *file;
	gint line;
	gchar *text;
};

typedef enum {
	FIND_DEF,
	FIND_REF,
	FIND_SYM,
	FIND_FILE,
	FIND_N,
} find_type_t;

extern void global_find(gchar *rootdir, const gchar *extra_arg, const gchar *text, void *cb, find_type_t ft);

#endif
