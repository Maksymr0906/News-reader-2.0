#include "News_reader.hpp"

int main() {
    io::io_context io_context;
    io::ssl::context ssl_context{io::ssl::context::tlsv12_client};
    Curses curses;

    WINDOW* main_window = create_new_window(LINES - 1, COLS, 0, 0);
    scrollok(main_window, true);
    WINDOW* command_window = create_new_window(1, 0, LINES - 1, 0);
    waddstr(command_window, "Press a key to exit.");
    wrefresh(command_window);
    
    const std::string server_name{ "news.gmane.io" };
    News_reader news_reader{ io_context, ssl_context, server_name, main_window };

    io_context.run();
    wgetch(command_window);

    delwin(main_window);
    delwin(command_window);
}