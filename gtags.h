#ifndef _GTAGS_H_
#define _GTAGS_H_

#include <geanyplugin.h>

#define GLOBAL_PATH_DEFAULT	"/usr/local/bin/global"

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

extern void global_set(const gchar *path);
extern void global_find(gchar *rootdir, const gchar *extra_arg, const gchar *text, void *cb, find_type_t ft);

#endif
