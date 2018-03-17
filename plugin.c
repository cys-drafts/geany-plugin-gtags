#include "gtags.h"

static GeanyData *geany_data;
static GeanyPlugin *geany_plugin;

#define geany_project_menu	(geany->main_widgets->project_menu)

static GtkWidget *menu_sep, *menu_find_file, *menu_find_def, *menu_find_ref, *menu_find_sym;

static gchar *project_rootdir;

static struct {
	GtkWidget *widget;
	GtkWidget *entry;
} find_dialog = {NULL, NULL};

static void menu_enable(gboolean enable);

/* utf8 */
static gchar *get_project_base_path(void)
{
	GeanyProject *project = geany->app->project;

	if (project && !EMPTY(project->base_path))
	{
		if (g_path_is_absolute(project->base_path))
			return g_strdup(project->base_path);
		else
		{	/* build base_path out of project file name's dir and base_path */
			gchar *path;
			gchar *dir = g_path_get_dirname(project->file_name);

			if (utils_str_equal(project->base_path, "./"))
				return dir;

			path = g_build_filename(dir, project->base_path, NULL);
			g_free(dir);
			return path;
		}
	}
	return NULL;
}
static gchar *get_current_word(void)
{
	GeanyDocument *doc = document_get_current();

	if (!doc)
		return NULL;

	if (sci_has_selection(doc->editor->sci))
		return sci_get_selection_contents(doc->editor->sci);
	else
		return editor_get_word_at_pos(doc->editor, -1, NULL);
}
static gint get_current_line(void)
{
	GeanyDocument *doc = document_get_current();

	if (!doc)
		return 0;

	return sci_get_current_line(doc->editor->sci) + 1;
}

static void find_dialog_hide(void)
{
	gtk_entry_set_text(GTK_ENTRY(find_dialog.entry), "");
	gtk_widget_hide(find_dialog.widget);
}
void enter_cb(GtkEntry *entry, gpointer user_data)
{
	gtk_dialog_response(GTK_DIALOG(find_dialog.widget), GTK_RESPONSE_ACCEPT);
}
static void find_dialog_create(void)
{
	find_dialog.widget = gtk_dialog_new_with_buttons("Gtags", GTK_WINDOW(geany->main_widgets->window), GTK_DIALOG_DESTROY_WITH_PARENT,
			"Cancel", GTK_RESPONSE_CANCEL, "Ok", GTK_RESPONSE_ACCEPT, NULL);
	GtkWidget *vbox = ui_dialog_vbox_new(GTK_DIALOG(find_dialog.widget));
	GtkWidget *entry = gtk_entry_new();

	gtk_entry_set_max_length(GTK_ENTRY(entry), 60);
	g_signal_connect(GTK_WIDGET(entry), "activate", G_CALLBACK(enter_cb), NULL);

	find_dialog.entry = entry;

	gtk_box_pack_start(GTK_BOX(vbox), entry, FALSE, TRUE, 0);
	gtk_widget_set_size_request(GTK_WIDGET(vbox), 300, 50);

	gtk_widget_show_all(vbox);
	gtk_widget_hide(find_dialog.widget);
}
static void on_project_open(G_GNUC_UNUSED GObject *obj, GKeyFile *config, G_GNUC_UNUSED gpointer user_data)
{
	//printf("project opened\n");

	project_rootdir = get_project_base_path();
	find_dialog_create();
	//printf("rootdir is %s\n", project_rootdir);

	menu_enable(TRUE);
}
static void on_project_close(GObject *obj, gpointer user_data)
{
	//printf("project closed\n");

	g_free(project_rootdir);
	project_rootdir = NULL;

	msgwin_clear_tab(MSG_MESSAGE);

	menu_enable(FALSE);
}

static PluginCallback signal_cbs[] = {
	{"project-open", (GCallback)&on_project_open, TRUE, NULL},
	{"project-close", (GCallback)&on_project_close, TRUE, NULL},
	{NULL, NULL, FALSE, NULL}
};

void show_tag(gpointer data, gpointer user_data)
{
	struct tag *tag = (struct tag *)data;
	find_type_t ft = (find_type_t)user_data;

	if (ft == FIND_FILE)
		msgwin_msg_add(COLOR_BLUE, -1, NULL, "%s", tag->file);
	else
		msgwin_msg_add(COLOR_BLUE, -1, NULL, "%s:%d\n%s", tag->file, tag->line, tag->text);
}
void input_cb(find_type_t ft)
{
	GtkEntryBuffer *buffer = gtk_entry_get_buffer(GTK_ENTRY(find_dialog.entry));
	gchar *txt = (gchar *)gtk_entry_buffer_get_text(buffer);

	g_strstrip(txt);
	if (!txt || !strcmp(txt, ""))
		return;

	//printf("input %s, find %d\n", txt, ft);

	msgwin_clear_tab(MSG_MESSAGE);
	msgwin_switch_tab(MSG_MESSAGE, TRUE);
	msgwin_set_messages_dir(project_rootdir);

	switch (ft) {
		case FIND_FILE:
		case FIND_DEF:
		case FIND_REF:
		case FIND_SYM:
			global_find(project_rootdir, NULL, txt, show_tag, ft);
			break;
		default:
			printf("bad find\n");
			break;
	}
}
static void show_find_dialog(find_type_t ft)
{
	if (ft != FIND_FILE) {
		gchar *word = get_current_word();
		if (word)
			gtk_entry_set_text(GTK_ENTRY(find_dialog.entry), word);
	}
	gtk_widget_grab_focus(GTK_WIDGET(find_dialog.entry));
	gint res = gtk_dialog_run(GTK_DIALOG(find_dialog.widget));
	if (res == GTK_RESPONSE_ACCEPT) {
		input_cb(ft);
	}
	find_dialog_hide();
}
static void on_find(GtkMenuItem *item, gpointer user_data)
{
	show_find_dialog((find_type_t)user_data);
}
static void menu_enable(gboolean enable)
{
	gtk_widget_set_sensitive(menu_find_def, enable);
	gtk_widget_set_sensitive(menu_find_ref, enable);
	gtk_widget_set_sensitive(menu_find_sym, enable);
	gtk_widget_set_sensitive(menu_find_file, enable);
}
static gboolean kb_callback(guint key_id)
{
	find_type_t ft = key_id;

	switch (key_id)	{
		case FIND_DEF:
		case FIND_REF:
		case FIND_SYM:
		{
			gchar *word = get_current_word();
			if (word)
				gtk_entry_set_text(GTK_ENTRY(find_dialog.entry), word);
			input_cb(ft);
			return TRUE;
		}
		case FIND_FILE:
		{
			on_find(NULL, (gpointer)ft);
			return TRUE;
		}
	}
	return FALSE;
}
static void menu_init(void)
{
	//printf("setup menus\n");

	GeanyKeyGroup *key_group = plugin_set_key_group(geany_plugin, "Gtags", FIND_N, kb_callback);

	menu_sep = gtk_separator_menu_item_new();
	gtk_widget_show(menu_sep);
	gtk_container_add(GTK_CONTAINER(geany_project_menu), menu_sep);

	menu_find_def = gtk_menu_item_new_with_mnemonic("Find Def");
	gtk_widget_show(menu_find_def);
	gtk_container_add(GTK_CONTAINER(geany_project_menu), menu_find_def);
	g_signal_connect(menu_find_def, "activate", G_CALLBACK(on_find), (gpointer)FIND_DEF);
	keybindings_set_item(key_group, FIND_DEF, NULL, 0, 0, "find_def", "Find Def", menu_find_def);

	menu_find_ref = gtk_menu_item_new_with_mnemonic("Find Ref");
	gtk_widget_show(menu_find_ref);
	gtk_container_add(GTK_CONTAINER(geany_project_menu), menu_find_ref);
	g_signal_connect(menu_find_ref, "activate", G_CALLBACK(on_find), (gpointer)FIND_REF);
	keybindings_set_item(key_group, FIND_REF, NULL, 0, 0, "find_ref", "Find Ref", menu_find_ref);

	menu_find_sym = gtk_menu_item_new_with_mnemonic("Find Symbol");
	gtk_widget_show(menu_find_sym);
	gtk_container_add(GTK_CONTAINER(geany_project_menu), menu_find_sym);
	g_signal_connect(menu_find_sym, "activate", G_CALLBACK(on_find), (gpointer)FIND_SYM);
	keybindings_set_item(key_group, FIND_SYM, NULL, 0, 0, "find_symbol", "Find Symbol", menu_find_sym);

	menu_find_file = gtk_menu_item_new_with_mnemonic("Find file");
	gtk_widget_show(menu_find_file);
	gtk_container_add(GTK_CONTAINER(geany_project_menu), menu_find_file);
	g_signal_connect(menu_find_file, "activate", G_CALLBACK(on_find), (gpointer)FIND_FILE);
	keybindings_set_item(key_group, FIND_FILE, NULL, 0, 0, "find_file", "Find File", menu_find_file);

	menu_enable(FALSE);
}
static void menu_clean(void)
{
	//printf("remove menus\n");

	gtk_widget_destroy(menu_sep);
	gtk_widget_destroy(menu_find_def);
	gtk_widget_destroy(menu_find_ref);
	gtk_widget_destroy(menu_find_file);
	gtk_widget_destroy(menu_find_sym);
}
static gboolean gtags_init(GeanyPlugin *plugin, gpointer pdata)
{
	//printf("niceday\n");

	geany_data = plugin->geany_data;
	geany_plugin = plugin;

	menu_init();

	return TRUE;
}

static void gtags_cleanup(GeanyPlugin *plugin, gpointer pdata)
{
	//printf("byebye\n");

	menu_clean();
}

G_MODULE_EXPORT
void geany_load_module(GeanyPlugin *plugin)
{
	plugin->info->name = "Gtags";
	plugin->info->description = "Code navigation using GNU Global";
	plugin->info->version = "1.0";
	plugin->info->author = "chaoys";

	plugin->funcs->init = gtags_init;
	plugin->funcs->cleanup = gtags_cleanup;
	plugin->funcs->callbacks = signal_cbs;

	GEANY_PLUGIN_REGISTER(plugin, 235);
}
