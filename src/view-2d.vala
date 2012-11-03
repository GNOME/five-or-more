using Cairo;
using Gdk;
using Gtk;

namespace FiveOrMore
{
    public class View2D : DrawingArea
    {
        public GlinesBoard board { get; private set; }
        public double line_width { get; set; }

        public View2D(GlinesBoard board)
        {
            this.board = board;
            this.line_width = 1.0;

            this.vexpand = true;
            this.hexpand = true;
            this.add_events (Gdk.EventMask.BUTTON_PRESS_MASK | Gdk.EventMask.BUTTON_RELEASE_MASK | Gdk.EventMask.KEY_PRESS_MASK);

            this.board.changed.connect ((o) => this.queue_draw());
            //TODO: drop this later:
            this.board.info.connect ((o, info) => stdout.printf(info + "\n"));
        }


        public override bool draw(Context ctx)
        {
            int w = this.get_allocated_width();
            int h = this.get_allocated_height();

            double boxsize_h = (w - (1 + board.rows) * line_width) / board.rows;
            double boxsize_v = (h - (1 + board.cols) * line_width) / board.cols;

            //Draw the backgrounds first
            for (int i = 0; i < board.cols; i++)
                for (int j = 0; j < board.rows; j++)
                    this.draw_field(this, ctx, j, i);

            ctx.set_source_rgb(52, 95, 108);
            ctx.set_line_width(line_width);

            for (int i = 0; i < board.rows; i++)
            {
                ctx.move_to(0.5 + i * (boxsize_h + line_width), 0.5);
                ctx.line_to(0.5 + i * (boxsize_h + line_width), 0.5 + h);
            }

            for (int i = 0; i < board.cols; i++)
            {
                ctx.move_to(0.5 + 0, 0.5 + i * (boxsize_v + line_width));
                ctx.line_to(0.5 + w, 0.5 + i * (boxsize_v + line_width));
            }

            ctx.rectangle(0.5, 0.5, w - 0.5, h - 0.5);
            ctx.stroke();

            /* Cursor */
            if (board.show_cursor)
            {
                ctx.set_source_rgb(255, 0, 0);
                ctx.set_line_width(line_width);

                ctx.rectangle(board.cursor_x * (boxsize_h + line_width) + 1.5, board.cursor_y * (boxsize_v + line_width) + 1.5, (boxsize_h + line_width) - 2.5, (boxsize_v + line_width) - 2.5);
                ctx.stroke();
            }

            return true;
        }

        private void draw_field(Widget da, Context ctx, int x, int y)
        {
            int w = da.get_allocated_width();
            int h = da.get_allocated_height();

            double boxsizeH = (w - (1 + board.rows) * line_width) / board.rows;
            double boxsizeV = (h - (1 + board.cols) * line_width) / board.cols;

            var field = board.fields[x, y];

            ctx.set_source_rgb(50, 50, 50);

            ctx.rectangle(x * (boxsizeH + line_width) + 1.5, y * (boxsizeV + line_width) + 1.5, (boxsizeH + line_width) - 2.5, (boxsizeV + line_width) - 2.5);
            ctx.fill();

            if (field.piece != null)
            {
                this.draw_piece(da, ctx, field.piece, x, y);
            }
        }

        private void draw_piece(Widget da, Context ctx, GlinesPiece p, int x, int y)
        {
            int w = da.get_allocated_width();
            int h = da.get_allocated_height();

            double boxsizeH = (w - (1 + board.rows) * line_width) / board.rows;
            double boxsizeV = (h - (1 + board.cols) * line_width) / board.cols;

            //TODO: look up surfaces instead. This is just for debug:
            if (p.id == 0)
                ctx.set_source_rgb(255, 0, 0);
            else if (p.id == 1)
                ctx.set_source_rgb(0, 255, 0);
            else if (p.id == 2)
                ctx.set_source_rgb(0, 0, 255);
            else
                ctx.set_source_rgb(0, 0, 0);


            ctx.rectangle(x * (boxsizeH + line_width) + 1.5, y * (boxsizeV + line_width) + 1.5, (boxsizeH + line_width) - 2.5, (boxsizeV + line_width) - 2.5);
            ctx.fill();
        }

        public override bool key_press_event (EventKey e)
        {
            var  key = e.keyval;

            if (key == Gdk.Key.Up) this.board.cursor_y--;
            if (key == Gdk.Key.Down) this.board.cursor_y++;
            if (key == Gdk.Key.Left) this.board.cursor_x--;
            if (key == Gdk.Key.Right) this.board.cursor_x++;

            if (key == Gdk.Key.space) board.select_field(board.cursor_x, board.cursor_y);

            return true;
        }

        public override bool button_press_event (EventButton e)
        {
            double boxsize_x = this.get_allocated_width() / (board.rows * 1.0);
            int x = (int)(e.x / boxsize_x);

            double boxsize_y = this.get_allocated_height() / (board.cols * 1.0);
            int y = (int)(e.y / boxsize_y);

            board.place_cursor(x, y);
            board.select_field(x, y);

            return false;
        }
    }
}

