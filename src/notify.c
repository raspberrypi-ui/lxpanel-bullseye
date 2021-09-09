#include <stdio.h>
#include "private.h"
#include "panel.h"
#include "plugin.h"


#define HIDE_TIME_MS 5000

typedef struct {
    GtkWidget *popup;               /* Popup message */
    guint hide_timer;               /* Timer to hide message window */
} NotifyWindow;


static gboolean hide_message (NotifyWindow *nw)
{
    if (nw->hide_timer) g_source_remove (nw->hide_timer);
    if (nw->popup) gtk_widget_destroy (nw->popup);
    g_free (nw);
    return FALSE;
}

static gboolean notify_window_click (GtkWidget *widget, GdkEventButton *event, NotifyWindow *nw)
{
    hide_message (nw);
    return FALSE;
}

static void show_message (LXPanel *panel, GtkWidget *end, NotifyWindow *nw, char *str)
{
    GtkWidget *box, *item;
    gint x, y;

    /*
     * In order to get a window which looks exactly like a system tooltip, client-side decoration
     * must be requested for it. This cannot be done by any public API call in GTK+3.24, but there is an
     * internal call _gtk_window_request_csd which sets the csd_requested flag in the class' private data.
     * The code below is compatible with a hacked GTK+3 library which uses GTK_WINDOW_POPUP + 1 as the type
     * for a window with CSD requested. It should also not fall over with the standard library...
     */
    nw->popup = gtk_window_new (GTK_WINDOW_POPUP + 1);
    if (!nw->popup) nw->popup = gtk_window_new (GTK_WINDOW_POPUP);
    gtk_window_set_type_hint (GTK_WINDOW (nw->popup), GDK_WINDOW_TYPE_HINT_TOOLTIP);
    gtk_window_set_resizable (GTK_WINDOW (nw->popup), FALSE);

    GtkStyleContext *context = gtk_widget_get_style_context (nw->popup);
    gtk_style_context_add_class (context, GTK_STYLE_CLASS_TOOLTIP);

    box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_add (GTK_CONTAINER (nw->popup), box);

    item = gtk_label_new (str);
    gtk_box_pack_start (GTK_BOX (box), item, FALSE, FALSE, 0);

    gtk_widget_show_all (nw->popup);
    gtk_widget_hide (nw->popup);
    lxpanel_plugin_popup_set_position_helper (panel, end, nw->popup, &x, &y);
    gdk_window_move (gtk_widget_get_window (nw->popup), x, y);
    gdk_window_set_events (gtk_widget_get_window (nw->popup), gdk_window_get_events (gtk_widget_get_window (nw->popup)) | GDK_BUTTON_PRESS_MASK);
    g_signal_connect (G_OBJECT (nw->popup), "button-press-event", G_CALLBACK (notify_window_click), nw);
    gtk_window_present (GTK_WINDOW (nw->popup));
    nw->hide_timer = g_timeout_add (HIDE_TIME_MS, (GSourceFunc) hide_message, nw);
}

void lxpanel_notify (LXPanel *panel, char *message)
{
    GList *plugins;
    NotifyWindow *nw = g_new (NotifyWindow, 1);

    plugins = gtk_container_get_children (GTK_CONTAINER (panel->priv->box));
    show_message (panel, (GtkWidget *) (g_list_last (plugins))->data, nw, message);
    g_list_free (plugins);
}
