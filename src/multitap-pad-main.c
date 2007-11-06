#include <fakekey/fakekey.h>
#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include "multitap-pad-remote.h"

static gchar *orientation = NULL, *position = NULL;

static gboolean
alt_keypress_timeout (GtkWidget *button)
{
	FakeKey *fk = g_object_get_data (G_OBJECT (button), "fk");
	guint keysym = GPOINTER_TO_UINT(
		g_object_get_data (G_OBJECT (button), "altkeysym"));
	guint flags = GPOINTER_TO_UINT(
		g_object_get_data (G_OBJECT (button), "altflags"));

	/* Release old key, backspace and press new key */
	fakekey_release (fk);
	fakekey_press_keysym (fk, XK_BackSpace, 0);
	fakekey_release (fk);
	fakekey_press_keysym (fk, keysym, flags);
	
	g_object_set_data (G_OBJECT (button), "timeout", NULL);
	
	return FALSE;
}

static gboolean
button_press_event_cb (GtkWidget *button, GdkEventButton *event, FakeKey *fk)
{
	guint keysym, flags;
	
	if (event->type != GDK_BUTTON_PRESS) return FALSE;

	keysym = GPOINTER_TO_UINT(g_object_get_data (
		G_OBJECT (button), "keysym"));
	flags = GPOINTER_TO_UINT(g_object_get_data (
		G_OBJECT (button), "flags"));
	
	fakekey_press_keysym (fk, keysym, flags);
	
	g_object_set_data (G_OBJECT (button), "timeout", GUINT_TO_POINTER (
#if GLIB_CHECK_VERSION(2,14,0)
		g_timeout_add_seconds (1, (GSourceFunc)
			alt_keypress_timeout, button)
#else
		g_timeout_add (1000, (GSourceFunc)alt_keypress_timeout, button)
#endif
		));
	
	return FALSE;
}

static gboolean
button_release_event_cb (GtkWidget *button, GdkEventButton *event, FakeKey *fk)
{
	guint timeout_id;
	
	/* Remove alt keypress timeout */
	timeout_id = GPOINTER_TO_UINT (g_object_get_data (
		G_OBJECT (button), "timeout"));
	if (timeout_id) {
		g_source_remove (timeout_id);
		g_object_set_data (G_OBJECT (button), "timeout", NULL);
	}
	
	/* Release key */
	fakekey_release (fk);
	
	/* FakeKey seems to mess with the button, so guarantee release */
	gtk_button_released (GTK_BUTTON (button));
	
	return FALSE;
}

static gboolean
toggle_alt_keypress_timeout (GtkWidget *button)
{
	FakeKey *fk = g_object_get_data (G_OBJECT (button), "fk");
	guint keysym = GPOINTER_TO_UINT(
		g_object_get_data (G_OBJECT (button), "keysym"));
	guint flags = GPOINTER_TO_UINT(
		g_object_get_data (G_OBJECT (button), "flags"));
	guint altkeysym = GPOINTER_TO_UINT(
		g_object_get_data (G_OBJECT (button), "altkeysym"));
	guint altflags = GPOINTER_TO_UINT(
		g_object_get_data (G_OBJECT (button), "altflags"));

	/* Press old key again, then press new key */
	fakekey_press_keysym (fk, keysym, flags);
	fakekey_release (fk);
	fakekey_press_keysym (fk, altkeysym, altflags);
	fakekey_release (fk);
	
	g_object_set_data (G_OBJECT (button), "timeout", NULL);
	
	/* Reverse toggle button state */
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button),
		!gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (button)));
	
	return FALSE;
}

static gboolean
toggle_button_press_event_cb (GtkWidget *button, GdkEventButton *event, FakeKey *fk)
{
	guint keysym, flags;

	if (event->type != GDK_BUTTON_PRESS) return FALSE;

	keysym = GPOINTER_TO_UINT(g_object_get_data (
		G_OBJECT (button), "keysym"));
	flags = GPOINTER_TO_UINT(g_object_get_data (
		G_OBJECT (button), "flags"));

	fakekey_press_keysym (fk, keysym, flags);
	fakekey_release (fk);
	
	g_object_set_data (G_OBJECT (button), "timeout", GUINT_TO_POINTER (
#if GLIB_CHECK_VERSION(2,14,0)
		g_timeout_add_seconds (1, (GSourceFunc)
			toggle_alt_keypress_timeout, button)
#else
		g_timeout_add (1000, (GSourceFunc)
			toggle_alt_keypress_timeout, button)
#endif
		));
	
	return FALSE;
}

static gboolean
toggle_button_release_event_cb (GtkWidget *button, GdkEventButton *event,
				FakeKey *fk)
{
	guint timeout_id;
	
	/* Remove alt keypress timeout */
	timeout_id = GPOINTER_TO_UINT (g_object_get_data (
		G_OBJECT (button), "timeout"));
	if (timeout_id) {
		g_source_remove (timeout_id);
		g_object_set_data (G_OBJECT (button), "timeout", NULL);
	}
	
	return FALSE;
}

static void
set_button_data (GtkWidget *button, guint keysym, guint flags, guint altkeysym,
		 guint altflags, FakeKey *fk)
{
	g_object_set_data (G_OBJECT (button), "fk", fk);
	g_object_set_data (G_OBJECT (button),
		"keysym", GUINT_TO_POINTER (keysym));
	g_object_set_data (G_OBJECT (button),
		"flags", GUINT_TO_POINTER (flags));
	g_object_set_data (G_OBJECT (button),
		"altkeysym", GUINT_TO_POINTER (altkeysym));
	g_object_set_data (G_OBJECT (button),
		"altflags", GUINT_TO_POINTER (altflags));
}

static GtkWidget *
new_button (const gchar *text, gboolean stock, guint keysym, guint flags,
	    guint altkeysym, guint altflags, FakeKey *fk)
{
	GtkWidget *button;
	
	if (stock) {
		button = gtk_button_new_from_stock (text);
	} else {
		GtkWidget *label;
		button = gtk_button_new ();
		label = gtk_label_new (NULL);
		gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_CENTER);
		gtk_label_set_use_markup (GTK_LABEL (label), TRUE);
		gtk_label_set_markup (GTK_LABEL (label), text);
		gtk_container_add (GTK_CONTAINER (button), label);
	}
	
	set_button_data (button, keysym, flags, altkeysym, altflags, fk);

	g_signal_connect (button, "button-press-event",
		G_CALLBACK(button_press_event_cb), fk);
	g_signal_connect (button, "button-release-event",
		G_CALLBACK(button_release_event_cb), fk);
	
	return button;
}

static GtkWidget *
new_toggle_button (const gchar *markup, guint keysym, guint flags,
		   guint altkeysym, guint altflags, FakeKey *fk)
{
	GtkWidget *button, *label;
	
	button = gtk_toggle_button_new ();
	
	label = gtk_label_new (NULL);
	gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_CENTER);
	gtk_label_set_use_markup (GTK_LABEL (label), TRUE);
	gtk_label_set_markup (GTK_LABEL (label), markup);
	
	gtk_container_add (GTK_CONTAINER (button), label);
	
	set_button_data (button, keysym, flags, altkeysym, altflags, fk);

	g_signal_connect (button, "button-press-event",
		G_CALLBACK(toggle_button_press_event_cb), fk);
	g_signal_connect (button, "button-release-event",
		G_CALLBACK(toggle_button_release_event_cb), fk);
	
	return button;
}

static GdkFilterReturn
invoke_cb (GdkXEvent *xevent, GdkEvent *event, GtkWidget *widget)
{
	switch (((XEvent *)xevent)->xclient.data.l[0]) {
	    case MTPRemoteShow :
		gtk_widget_show (widget);
		break;
	    case MTPRemoteHide :
		gtk_widget_hide (widget);
		break;
	    case MTPRemoteToggle :
		if (GTK_WIDGET_VISIBLE (widget)) gtk_widget_hide (widget);
		else gtk_widget_show (widget);
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
	FakeKey *fk;
	GtkWidget *window, *table, *button;
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
	fk = fakekey_init (gdk_x11_get_default_xdisplay ());
	
	/* Create main window */
	if (plug <= 0) {
		window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
		gtk_window_set_skip_pager_hint (GTK_WINDOW (window), TRUE);
		gtk_window_set_skip_taskbar_hint (GTK_WINDOW (window), TRUE);
		gtk_window_set_decorated (GTK_WINDOW (window), FALSE);
		gtk_window_set_accept_focus (GTK_WINDOW (window), FALSE);

		/* Set size and position */
		screen_size_changed_cb (gdk_screen_get_default (), window);
	
		g_signal_connect (gdk_screen_get_default (), "size-changed",
			G_CALLBACK (screen_size_changed_cb), window);

		g_signal_connect (window, "delete-event",
			G_CALLBACK (gtk_main_quit), NULL);
	} else {
		g_message ("Plugging into socket %d", plug);
		window = gtk_plug_new (plug);
		g_signal_connect (window, "destroy",
			G_CALLBACK (gtk_main_quit), NULL);
	}
	
	/* Add event filter for remote command */
	if (daemon) {
		gdk_window_set_events (gdk_get_default_root_window (),
			GDK_SUBSTRUCTURE_MASK);
		gdk_add_client_message_filter (
			gdk_atom_intern ("_MTP_IM_INVOKER_COMMAND", FALSE),
			(GdkFilterFunc)invoke_cb, window);
	}
	
	/* Create keypad table */
	table = gtk_table_new (5, 6, TRUE);
	
	/* Create buttons */
	button = new_button ("1\n<small>.,-?!</small>", FALSE,
		XK_period, 0, XK_1, 0, fk);
	gtk_table_attach_defaults (GTK_TABLE (table), button, 0, 2, 0, 1);

	button = new_button ("2\n<small>abc</small>", FALSE,
		XK_a, 0, XK_2, 0, fk);
	gtk_table_attach_defaults (GTK_TABLE (table), button, 2, 4, 0, 1);

	button = new_button ("3\n<small>def</small>", FALSE,
		XK_d, 0, XK_3, 0, fk);
	gtk_table_attach_defaults (GTK_TABLE (table), button, 4, 6, 0, 1);

	button = new_button ("4\n<small>ghi</small>", FALSE,
		XK_g, 0, XK_4, 0, fk);
	gtk_table_attach_defaults (GTK_TABLE (table), button, 0, 2, 1, 2);

	button = new_button ("5\n<small>jkl</small>", FALSE,
		XK_j, 0, XK_5, 0, fk);
	gtk_table_attach_defaults (GTK_TABLE (table), button, 2, 4, 1, 2);

	button = new_button ("6\n<small>mno</small>", FALSE,
		XK_m, 0, XK_6, 0, fk);
	gtk_table_attach_defaults (GTK_TABLE (table), button, 4, 6, 1, 2);

	button = new_button ("7\n<small>pqrs</small>", FALSE,
		XK_p, 0, XK_7, 0, fk);
	gtk_table_attach_defaults (GTK_TABLE (table), button, 0, 2, 2, 3);

	button = new_button ("8\n<small>tuv</small>", FALSE,
		XK_t, 0, XK_8, 0, fk);
	gtk_table_attach_defaults (GTK_TABLE (table), button, 2, 4, 2, 3);

	button = new_button ("9\n<small>wxyz</small>", FALSE,
		XK_w, 0, XK_9, 0, fk);
	gtk_table_attach_defaults (GTK_TABLE (table), button, 4, 6, 2, 3);

	button = new_toggle_button ("*\n<small>^</small>",
		XK_Caps_Lock, 0, XK_asterisk, 0, fk);
	gtk_table_attach_defaults (GTK_TABLE (table), button, 0, 2, 3, 4);

	button = new_button ("0\n<small>+</small>", FALSE,
		XK_KP_Add, 0, XK_0, 0, fk);
	gtk_table_attach_defaults (GTK_TABLE (table), button, 2, 4, 3, 4);

	button = new_button ("#\n<small>_</small>", FALSE,
		XK_space, 0, XK_numbersign, 0, fk);
	gtk_table_attach_defaults (GTK_TABLE (table), button, 4, 6, 3, 4);

	button = new_button (GTK_STOCK_GO_BACK, TRUE,
		XK_BackSpace, 0, XK_BackSpace, FAKEKEYMOD_CONTROL, fk);
	gtk_table_attach_defaults (GTK_TABLE (table), button, 0, 3, 4, 5);
	
	button = new_button (GTK_STOCK_OK, TRUE,
		XK_Return, 0, XK_Return, FAKEKEYMOD_CONTROL, fk);
	gtk_table_attach_defaults (GTK_TABLE (table), button, 3, 6, 4, 5);

	/* Pack and show widgets */
	gtk_container_add (GTK_CONTAINER (window), table);
	gtk_widget_show_all (table);

	/* Show window/plug */
	if (!daemon) gtk_widget_show_all (window);
	
	/* Start */
	gtk_main ();
	
	return 0;
}

