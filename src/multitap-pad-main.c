#include <fakekey/fakekey.h>
#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include "multitap-pad-remote.h"

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
new_button (const gchar *markup, guint keysym, guint flags,
	    guint altkeysym, guint altflags, FakeKey *fk)
{
	GtkWidget *button, *label;
	
	button = gtk_button_new ();
	
	label = gtk_label_new (NULL);
	gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_CENTER);
	gtk_label_set_use_markup (GTK_LABEL (label), TRUE);
	gtk_label_set_markup (GTK_LABEL (label), markup);
	
	gtk_container_add (GTK_CONTAINER (button), label);
	
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

static gboolean
client_event_cb (GtkWidget *widget, GdkEventClient *event)
{
	switch (event->data.l[0]) {
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
	return FALSE;
}

int
main (int argc, char **argv)
{
	FakeKey *fk;
	GtkWidget *window, *table, *button;
	
	gtk_init (&argc, &argv);
	
	/* Create FakeKey context */
	fk = fakekey_init (gdk_x11_get_default_xdisplay ());
	
	/* Add remote command atom */
	gdk_atom_intern_static_string ("_MTP_IM_INVOKER_COMMAND");
	
	/* Create main window */
	window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	gtk_window_set_type_hint (GTK_WINDOW (window),
		GDK_WINDOW_TYPE_HINT_DOCK);
	gtk_window_set_gravity (GTK_WINDOW (window), GDK_GRAVITY_SOUTH);
	gtk_window_set_skip_pager_hint (GTK_WINDOW (window), TRUE);
	gtk_window_set_skip_taskbar_hint (GTK_WINDOW (window), TRUE);
	gtk_window_set_decorated (GTK_WINDOW (window), FALSE);
	gtk_window_set_accept_focus (GTK_WINDOW (window), FALSE);

	g_signal_connect (window, "client-event",
		G_CALLBACK (client_event_cb), NULL);
	g_signal_connect (window, "delete-event",
		G_CALLBACK (gtk_main_quit), NULL);
	
	/* Create keypad table */
	table = gtk_table_new (4, 3, TRUE);
	
	/* Create buttons */
	button = new_button ("1\n<small>.,-?!</small>",
		XK_period, 0, XK_1, 0, fk);
	gtk_table_attach_defaults (GTK_TABLE (table), button, 0, 1, 0, 1);

	button = new_button ("2\n<small>abc</small>",
		XK_a, 0, XK_2, 0, fk);
	gtk_table_attach_defaults (GTK_TABLE (table), button, 1, 2, 0, 1);

	button = new_button ("3\n<small>def</small>",
		XK_d, 0, XK_3, 0, fk);
	gtk_table_attach_defaults (GTK_TABLE (table), button, 2, 3, 0, 1);

	button = new_button ("4\n<small>ghi</small>",
		XK_g, 0, XK_4, 0, fk);
	gtk_table_attach_defaults (GTK_TABLE (table), button, 0, 1, 1, 2);

	button = new_button ("5\n<small>jkl</small>",
		XK_j, 0, XK_5, 0, fk);
	gtk_table_attach_defaults (GTK_TABLE (table), button, 1, 2, 1, 2);

	button = new_button ("6\n<small>mno</small>",
		XK_m, 0, XK_6, 0, fk);
	gtk_table_attach_defaults (GTK_TABLE (table), button, 2, 3, 1, 2);

	button = new_button ("7\n<small>pqrs</small>",
		XK_p, 0, XK_7, 0, fk);
	gtk_table_attach_defaults (GTK_TABLE (table), button, 0, 1, 2, 3);

	button = new_button ("8\n<small>tuv</small>",
		XK_t, 0, XK_8, 0, fk);
	gtk_table_attach_defaults (GTK_TABLE (table), button, 1, 2, 2, 3);

	button = new_button ("9\n<small>wxyz</small>",
		XK_w, 0, XK_9, 0, fk);
	gtk_table_attach_defaults (GTK_TABLE (table), button, 2, 3, 2, 3);

	button = new_toggle_button ("*\n<small>^</small>",
		XK_Caps_Lock, 0, XK_asterisk, 0, fk);
	gtk_table_attach_defaults (GTK_TABLE (table), button, 0, 1, 3, 4);

	button = new_button ("0\n<small>+</small>",
		XK_KP_Add, 0, XK_0, 0, fk);
	gtk_table_attach_defaults (GTK_TABLE (table), button, 1, 2, 3, 4);

	button = new_button ("#\n<small>_</small>",
		XK_space, 0, XK_numbersign, 0, fk);
	gtk_table_attach_defaults (GTK_TABLE (table), button, 2, 3, 3, 4);
	
	/* Pack and show widgets */
	gtk_container_add (GTK_CONTAINER (window), table);
	gtk_widget_show_all (window);
	
	/* Start */
	gtk_main ();
	
	return 0;
}

