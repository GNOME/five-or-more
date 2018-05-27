public class FiveOrMoreApp: Gtk.Application {
    private Gtk.Window window;

    public FiveOrMoreApp () {
        Object (application_id: "org.gnome.five-or-more", flags: ApplicationFlags.FLAGS_NONE);
    }

    public override void activate () {
        window = new FiveOrMore.Window(this);
        window.present ();
    }

    public static int main (string[] args) {
        Environment.set_application_name (_("Five or More"));
        Gtk.Window.set_default_icon_name ("five-or-more");

        var app = new FiveOrMoreApp ();
        return app.run (args);
    }
}
