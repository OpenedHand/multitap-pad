#include <fakekey/fakekey.h>
#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include "multitap-pad-remote.h"

typedef enum {
	NORMAL,
	CAPS,
	ALT
} MtpMode;

typedef struct {
	gchar *text;
	guint keysym;
	guint flags;
} MtpKey;

typedef struct {
	GtkWidget *window;
	gchar *orientation;
	gchar *position;
	
	MtpMode mode;
	gboolean change_mode;

	guint hide_idle;

	GtkWidget *alt_button;
	
	GList *buttons;
	
	FakeKey *fk;
} MtpData;

static gchar *orientation = NULL, *position = NULL;

static void
change_mode (MtpData *data)
{
	GList *b;
	const gchar *keyname;
	
	switch (data->mode) {
	    case NORMAL :
		data->mode = CAPS;
		keyname = "capskey";
		break;
	    case CAPS :
		data->mode = ALT;
		keyname = "altkey";
		break;
	    default :
	    case ALT :
		data->mode = NORMAL;
		keyname = "key";
		break;
	}
	
	for (b = data->buttons; b; b = b->next) {
		GtkWidget *button = (GtkWidget *)b->data;
		MtpKey *key = g_object_get_data (G_OBJECT (button), keyname);
		if (key && key->text) {
			GtkWidget *widget = g_object_get_data (
				(GObject *)button, "label");
			if (widget) {
				gtk_label_set_markup (GTK_LABEL (widget),
					key->text);
			} else {
				widget = g_object_get_data (
				(GObject *)button, "image");
				if (widget) {
					gtk_image_set_from_stock (
						GTK_IMAGE (widget), key->text,
						GTK_ICON_SIZE_BUTTON);
				}
			}
		}
	}
}

static gboolean
alt_keypress_timeout (GtkWidget *button)
{
	MtpData *data = g_object_get_data (G_OBJECT (button), "data");
	MtpKey *key = g_object_get_data (G_OBJECT (button), "key");
	MtpKey *altkey = g_object_get_data (G_OBJECT (button), "altkey");

	/* If this is the mode key, don't change mode */
	if (button == data->alt_button)
		data->change_mode = FALSE;

	/* Release old key, backspace and press new key */
	if (key && key->keysym) {
		fakekey_release (data->fk);
		fakekey_press_keysym (data->fk, XK_BackSpace, 0);
		fakekey_release (data->fk);
	}
	if (altkey && altkey->keysym)
		fakekey_press_keysym (data->fk, altkey->keysym, altkey->flags);
	
	g_object_set_data (G_OBJECT (button), "timeout", NULL);
	g_object_set_data (G_OBJECT (button), "data", NULL);
	
	return FALSE;
}

static gboolean
button_press_event_cb (GtkWidget *button, GdkEventButton *event, MtpData *data)
{
	MtpKey *key;
	
	if (event->type != GDK_BUTTON_PRESS) return FALSE;
	
	if (button == data->alt_button)
		data->change_mode = TRUE;

	switch (data->mode) {
	    case NORMAL :
		key = g_object_get_data (G_OBJECT (button), "key");
		break;
	    case CAPS :
		key = g_object_get_data (G_OBJECT (button), "capskey");
		break;
	    default :
	    case ALT :
		key = g_object_get_data (G_OBJECT (button), "altkey");
		break;
	}

	if (key && key->keysym && (button != data->alt_button)) {
		fakekey_press_keysym (data->fk, key->keysym, key->flags);
	}
	
	g_object_set_data (G_OBJECT (button), "data", data);
	g_object_set_data (G_OBJECT (button), "timeout", GUINT_TO_POINTER (
		g_timeout_add (1000,(GSourceFunc)alt_keypress_timeout,button)));
	
	return FALSE;
}

static gboolean
button_release_event_cb (GtkWidget *button, GdkEventButton *event,
			 MtpData *data)
{
	guint timeout_id;
	
	/* Remove alt keypress timeout */
	timeout_id = GPOINTER_TO_UINT (g_object_get_data (
		G_OBJECT (button), "timeout"));
	if (timeout_id) {
		g_source_remove (timeout_id);
		g_object_set_data (G_OBJECT (button), "timeout", NULL);
		g_object_set_data (G_OBJECT (button), "data", NULL);
	}
	
	if ((button == data->alt_button) && data->change_mode) {
		change_mode (data);
	}
	
	/* Release key */
	fakekey_release (data->fk);
	
	/* FakeKey seems to mess with the button, so guarantee release */
	gtk_button_released (GTK_BUTTON (button));

	return FALSE;
}

static GtkWidget *
new_button (MtpKey key, MtpKey altkey, MtpKey capskey, gboolean icon,
	    MtpData *data)
{
	GtkWidget *button, *child;
	MtpKey *key_copy;
	
	button = gtk_button_new ();
	key_copy = g_memdup (&key, sizeof (MtpKey));
	g_object_set_data (G_OBJECT (button), "key", key_copy);
	key_copy = g_memdup (&altkey, sizeof (MtpKey));
	g_object_set_data (G_OBJECT (button), "altkey", key_copy);
	key_copy = g_memdup (&capskey, sizeof (MtpKey));
	g_object_set_data (G_OBJECT (button), "capskey", key_copy);
	
	if (icon) {
		child = gtk_image_new_from_stock (
			key.text, GTK_ICON_SIZE_BUTTON);
		g_object_set_data (G_OBJECT (button), "image", child);
	} else {
		child = gtk_label_new (NULL);
		gtk_label_set_justify (GTK_LABEL (child), GTK_JUSTIFY_CENTER);
		gtk_label_set_use_markup (GTK_LABEL (child), TRUE);
		gtk_label_set_markup (GTK_LABEL (child), key.text);
		g_object_set_data (G_OBJECT (button), "label", child);
	}
	
	gtk_container_add (GTK_CONTAINER (button), child);
	
	g_signal_connect (button, "button-press-event",
		G_CALLBACK(button_press_event_cb), data);
	g_signal_connect (button, "button-release-event",
		G_CALLBACK(button_release_event_cb), data);
	
	return button;
}

static gboolean
hide_window_idle (MtpData *data)
{
	gtk_widget_hide (data->window);
	data->hide_idle = 0;
	
	return FALSE;
}

static void
hide_window (MtpData *data)
{
	if (!data->hide_idle) {
		data->hide_idle = g_timeout_add (
			300, (GSourceFunc)hide_window_idle, data);
	}
}

static void
show_window (MtpData *data)
{
	if (data->hide_idle) {
		g_source_remove (data->hide_idle);
		data->hide_idle = 0;
	}
	gtk_widget_show (data->window);
}

static GdkFilterReturn
invoke_cb (GdkXEvent *xevent, GdkEvent *event, MtpData *data)
{
	switch (((XEvent *)xevent)->xclient.data.l[0]) {
	    case MTPRemoteShow :
		show_window (data);
		break;
	    case MTPRemoteHide :
		hide_window (data);
		break;
	    case MTPRemoteToggle :
		if (GTK_WIDGET_VISIBLE (data->window)) {
			hide_window (data);
		} else {
			show_window (data);
		}
		break;
	    case MTPRemoteNone :
		break;
	}
	
	return GDK_FILTER_REMOVE;
}

static void
screen_size_changed_cb (GdkScreen *screen, GtkWidget *window)
{
	/* Set size */
	if (orientation && (strcasecmp (orientation, "landscape") == 0)) {
		gtk_window_set_type_hint (GTK_WINDOW (window),
			GDK_WINDOW_TYPE_HINT_DOCK);
		gtk_window_resize (GTK_WINDOW (window), 1,
			gdk_screen_get_height (screen));
	} else {
		gtk_window_set_type_hint (GTK_WINDOW (window),
			GDK_WINDOW_TYPE_HINT_TOOLBAR);
		gtk_window_resize (GTK_WINDOW (window),
			gdk_screen_get_width (screen), 1);
	}

	/* Set position */
	if ((!position) || (strcasecmp (position, "south") == 0)) {
		gtk_window_set_gravity (GTK_WINDOW (window),
			GDK_GRAVITY_SOUTH_WEST);
	} else if (strcasecmp (position, "north") == 0) {
		gtk_window_set_gravity (GTK_WINDOW (window),
			GDK_GRAVITY_NORTH_WEST);
	} else if (strcasecmp (position, "east") == 0) {
		gtk_window_set_gravity (GTK_WINDOW (window),
			GDK_GRAVITY_SOUTH_EAST);
	} else if (strcasecmp (position, "west") == 0) {
		gtk_window_set_gravity (GTK_WINDOW (window),
			GDK_GRAVITY_NORTH_WEST);
	}
	gtk_window_move (GTK_WINDOW (window), 0, 0);
}

int
main (int argc, char **argv)
{
	MtpData data;
	GtkWidget *table, *button;
	GOptionContext *context;
	static gboolean daemon = FALSE;
	static gint plug = 0;
	
	static GOptionEntry entries[] = {
		{ "daemon", 'd', 0, G_OPTION_ARG_NONE, &daemon,
			"Run in 'daemon' mode (for remote control)", NULL },
		{ "orientation", 'o', 0, G_OPTION_ARG_STRING, &orientation,
			"<portrait|landscape>", NULL },
		{ "position", 'p', 0, G_OPTION_ARG_STRING, &position,
			"<north|south|east|west>", NULL },
		{ "xembed", 'x', 0, G_OPTION_ARG_INT, &plug,
			"Socket ID of an XEmbed socket to plug into", NULL },
		{ NULL }
	};

	context = g_option_context_new (" - A virtual, multi-tap input pad");
	g_option_context_add_main_entries (context, entries, NULL);
	g_option_context_add_group (context, gtk_get_option_group (TRUE));
	g_option_context_parse (context, &argc, &argv, NULL);
	gtk_init (&argc, &argv);
	
	/* Create FakeKey context */
	data.fk = fakekey_init (gdk_x11_get_default_xdisplay ());
	data.hide_idle = 0;
	
	/* Create main window */
	if (plug <= 0) {
		data.window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
		gtk_window_set_skip_pager_hint (GTK_WINDOW (data.window), TRUE);
		gtk_window_set_skip_taskbar_hint (
			GTK_WINDOW (data.window), TRUE);
		gtk_window_set_decorated (GTK_WINDOW (data.window), FALSE);
		gtk_window_set_accept_focus (GTK_WINDOW (data.window), FALSE);

		/* Set size and position */
		screen_size_changed_cb (gdk_screen_get_default (), data.window);
	
		g_signal_connect (gdk_screen_get_default (), "size-changed",
			G_CALLBACK (screen_size_changed_cb), data.window);

		g_signal_connect (data.window, "delete-event",
			G_CALLBACK (gtk_main_quit), NULL);
	} else {
		g_message ("Plugging into socket %d", plug);
		data.window = gtk_plug_new (plug);
		g_signal_connect (data.window, "destroy",
			G_CALLBACK (gtk_main_quit), NULL);
	}
	
	/* Add event filter for remote command */
	if (daemon) {
		gdk_window_set_events (gdk_get_default_root_window (),
			GDK_SUBSTRUCTURE_MASK);
		gdk_add_client_message_filter (
			gdk_atom_intern ("_MTP_IM_INVOKER_COMMAND", FALSE),
			(GdkFilterFunc)invoke_cb, &data);
	}
	
	/* Create keypad table */
	table = gtk_table_new (4, 7, TRUE);
	
	/* Create buttons */
	button = new_button ((MtpKey){ "<small>1\n.,-?!</small>", XK_period, 0},
		(MtpKey){ "1", XK_1, 0 },
		(MtpKey){ NULL, XK_period, 0}, FALSE, &data);
	gtk_table_attach_defaults (GTK_TABLE (table), button, 0, 2, 0, 1);
	data.buttons = g_list_prepend (NULL, button);

	button = new_button ((MtpKey){ "<small>2\nabc</small>", XK_a, 0 },
		(MtpKey){ "2", XK_2, 0 },
		(MtpKey){ "<small>2\nABC</small>", XK_A, 0 }, FALSE, &data);
	gtk_table_attach_defaults (GTK_TABLE (table), button, 2, 4, 0, 1);
	data.buttons = g_list_prepend (data.buttons, button);

	button = new_button ((MtpKey){ "<small>3\ndef</small>", XK_d, 0 },
		(MtpKey){ "3", XK_3, 0 },
		(MtpKey){ "<small>3\nDEF</small>", XK_D, 0 }, FALSE, &data);
	gtk_table_attach_defaults (GTK_TABLE (table), button, 4, 6, 0, 1);
	data.buttons = g_list_prepend (data.buttons, button);

	button = new_button ((MtpKey){ "<small>4\nghi</small>", XK_g, 0 },
		(MtpKey){ "4", XK_4, 0 },
		(MtpKey){ "<small>4\nGHI</small>", XK_G, 0 }, FALSE, &data);
	gtk_table_attach_defaults (GTK_TABLE (table), button, 0, 2, 1, 2);
	data.buttons = g_list_prepend (data.buttons, button);

	button = new_button ((MtpKey){ "<small>5\njkl</small>", XK_j, 0 },
		(MtpKey){ "5", XK_5, 0 },
		(MtpKey){ "<small>5\nJKL</small>", XK_5, 0 }, FALSE, &data);
	gtk_table_attach_defaults (GTK_TABLE (table), button, 2, 4, 1, 2);
	data.buttons = g_list_prepend (data.buttons, button);

	button = new_button ((MtpKey){ "<small>6\nmno</small>", XK_m, 0 },
		(MtpKey){ "6", XK_6, 0 }, 
		(MtpKey){ "<small>6\nMNO</small>", XK_M, 0 }, FALSE, &data);
	gtk_table_attach_defaults (GTK_TABLE (table), button, 4, 6, 1, 2);
	data.buttons = g_list_prepend (data.buttons, button);

	button = new_button ((MtpKey){ "<small>7\npqrs</small>", XK_p, 0 },
		(MtpKey){ "7", XK_7, 0 },
		(MtpKey){ "<small>7\nPQRS</small>", XK_P, 0 }, FALSE, &data);
	gtk_table_attach_defaults (GTK_TABLE (table), button, 0, 2, 2, 3);
	data.buttons = g_list_prepend (data.buttons, button);

	button = new_button ((MtpKey){ "<small>8\ntuv</small>", XK_t, 0 },
		(MtpKey){ "8", XK_8, 0 },
		(MtpKey){ "<small>8\nTUV</small>", XK_T, 0 }, FALSE, &data);
	gtk_table_attach_defaults (GTK_TABLE (table), button, 2, 4, 2, 3);
	data.buttons = g_list_prepend (data.buttons, button);

	button = new_button ((MtpKey){ "<small>9\nwxyz</small>", XK_w, 0 },
		(MtpKey){ "9", XK_9, 0 },
		(MtpKey){ "<small>9\nWXYZ</small>", XK_W, 0 }, FALSE, &data);
	gtk_table_attach_defaults (GTK_TABLE (table), button, 4, 6, 2, 3);
	data.buttons = g_list_prepend (data.buttons, button);

	/* Mode-changing button */
	data.mode = NORMAL;
	data.change_mode = FALSE;
	data.alt_button = new_button ((MtpKey){ "<small>*\n^</small>", 0, 0 },
		(MtpKey){ NULL, XK_asterisk, 0 },
		(MtpKey){ NULL, 0, 0 }, FALSE, &data);
	gtk_table_attach_defaults (GTK_TABLE (table),
		data.alt_button, 0, 2, 3, 4);

	button = new_button ((MtpKey){ "<small>0\n+</small>", XK_KP_Add, 0 },
		(MtpKey){ "0", XK_0, 0 },
		(MtpKey){ NULL, XK_KP_Add, 0 }, FALSE, &data);
	gtk_table_attach_defaults (GTK_TABLE (table), button, 2, 4, 3, 4);

	button = new_button ((MtpKey){ "<small>#\n_</small>", XK_space, 0 },
		(MtpKey){ "#\n<small>_</small>", XK_numbersign, 0 },
		(MtpKey){ "#\n<small>_</small>", XK_space, 0 }, FALSE, &data);
	gtk_table_attach_defaults (GTK_TABLE (table), button, 4, 6, 3, 4);

	button = new_button ((MtpKey){ GTK_STOCK_GO_BACK, XK_BackSpace, 0 },
		(MtpKey){ NULL, XK_BackSpace, FAKEKEYMOD_CONTROL },
		(MtpKey){ NULL, XK_BackSpace, 0 }, TRUE, &data);
	gtk_table_attach_defaults (GTK_TABLE (table), button, 6, 7, 0, 1);

	button = new_button ((MtpKey){ GTK_STOCK_GO_FORWARD, XK_Right, 0 },
		(MtpKey){ NULL, XK_Right, FAKEKEYMOD_CONTROL },
		(MtpKey){ NULL, XK_Right, 0 }, TRUE, &data);
	gtk_table_attach_defaults (GTK_TABLE (table), button, 6, 7, 1, 2);
	
	button = new_button ((MtpKey){ GTK_STOCK_OK, XK_Return, 0 },
		(MtpKey){ NULL, XK_Return, FAKEKEYMOD_CONTROL },
		(MtpKey){ NULL, XK_Return, 0 }, TRUE, &data);
	gtk_table_attach_defaults (GTK_TABLE (table), button, 6, 7, 2, 4);

	/* Pack and show widgets */
	gtk_container_add (GTK_CONTAINER (data.window), table);
	gtk_widget_show_all (table);

	/* Show window/plug */
	if (!daemon) gtk_widget_show_all (data.window);
	
	/* Start */
	gtk_main ();
	
	return 0;
}

