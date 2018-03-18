#include "gtags.h"
#include <stdlib.h>

static struct {
	GPid pid;
	GSList *result;
	const gchar *global_path;
} runtime = { 0, NULL };

#define CALL_FLAGS	(SPAWN_ASYNC | SPAWN_LINE_BUFFERED)

static void tag_free(gpointer data)
{
	struct tag *tag = (struct tag *)data;
	if (!tag)
		return;
	if (tag->file)
		g_free(tag->file);
	if (tag->text)
		g_free(tag->text);
	g_free(tag);
}
static void runtime_reset(void)
{
	if (runtime.pid != 0) {
		spawn_kill_process(runtime.pid, NULL);
	}
	runtime.pid = 0;
	g_slist_free_full(runtime.result, tag_free);
	runtime.result = NULL;
}
static void stderr_cb(GString *string, GIOCondition condition, gpointer data)
{
	//printf("stderr: %s\n", string->str);
}
static void file_stdout_cb(GString *string, GIOCondition condition, gpointer data)
{
	g_strstrip(string->str);
	if (!strcmp(string->str, ""))
		return;
	//printf("stdout: %s\n", string->str);
	struct tag *tag = g_malloc(sizeof(*tag));
	tag->file = g_strdup(string->str);
	tag->line = 1;
	tag->text = NULL;
	runtime.result = g_slist_prepend(runtime.result, tag);
}
static void file_exit_cb(GPid pid, gint status, gpointer user_data)
{
	if (SPAWN_WIFEXITED(status) && (SPAWN_WEXITSTATUS(status) == 0)) {
		g_slist_foreach(runtime.result, (GFunc)user_data, (gpointer)FIND_FILE);
		//printf("exited normally\n");
	}
	runtime.pid = 0;
}

/* str == standard ctags cxref format */
static void parse_tag(struct tag *tag, gchar *str)
{
	gchar *ptr;

	while (*str != ' ')
		str++;
	while (*str == ' ')
		str++;
	ptr = str;
	while (*str != ' ')
		str++;
	*str = 0;
	tag->line = atoi(ptr);
	str++;
	while (*str == ' ')
		str++;
	ptr = str;
	while (*str != ' ')
		str++;
	*str = 0;
	tag->file = g_strdup(ptr);
	str++;
	while (*str == ' ')
		str++;
	ptr = str;
	while (*str != '\n' && *str != 0)
		str++;
	*str = 0;
	tag->text = g_strdup(ptr);
}
static void tag_stdout_cb(GString *string, GIOCondition condition, gpointer data)
{
	g_strstrip(string->str);
	if (!strcmp(string->str, ""))
		return;
	//printf("stdout: %s\n", string->str);
	struct tag *tag = g_malloc(sizeof(*tag));
	parse_tag(tag, string->str);
	runtime.result = g_slist_prepend(runtime.result, tag);
}
static void tag_exit_cb(GPid pid, gint status, gpointer user_data)
{
	if (SPAWN_WIFEXITED(status) && (SPAWN_WEXITSTATUS(status) == 0)) {
		g_slist_foreach(runtime.result, (GFunc)user_data, (gpointer)FIND_DEF);
		//printf("exited normally\n");
	}
	runtime.pid = 0;
}

void global_find(gchar *rootdir, const gchar *extra_arg, const gchar *text, void *cb, find_type_t ft)
{
	int argc;
	gchar *argv[4];
	if (ft == FIND_FILE) {
		argv[0] = "-P";
	} else if (ft == FIND_DEF) {
		argv[0] = "-dx";
	} else if (ft == FIND_REF) {
		argv[0] = "-rx";
	} else if (ft == FIND_SYM) {
		argv[0] = "-sx";
	} else {
		return;
	}
	argc = 1;
	if (extra_arg) {
		argv[argc] = (gchar *)extra_arg;
		argc++;
	}
	argv[argc++] = (gchar *)text;
	argv[argc] = NULL;

	runtime_reset();

	gboolean res = spawn_with_callbacks(rootdir, runtime.global_path, argv, NULL, CALL_FLAGS, NULL, NULL,
		(ft == FIND_FILE)?file_stdout_cb:tag_stdout_cb, NULL, 0, stderr_cb, NULL, 0,
		(ft == FIND_FILE)?file_exit_cb:tag_exit_cb, cb, &runtime.pid, NULL);
	if (!res) {
		//printf("exec failed\n");
	}
}

void global_set(const gchar *path)
{
	runtime.global_path = path;
}
