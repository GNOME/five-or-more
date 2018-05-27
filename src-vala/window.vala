namespace FiveOrMore {
    [GtkTemplate (ui = "/org/gnome/five-or-more/ui/five-or-more-vala.ui")]
    public class Window : Gtk.ApplicationWindow {

        public Window (Gtk.Application app) {
            Object (application: app);
        }
    }
}
