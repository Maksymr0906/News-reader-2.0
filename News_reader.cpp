#include "News_reader.hpp"

News_reader::News_reader(io::io_context& io_context, io::ssl::context& ssl_context, const std::string& server_name, WINDOW* window)
	:io_context{ io_context }, socket{ io_context, ssl_context }, window{ window }, server_name{ server_name }, is_data_returns{}, using_TLS{}, resolver{ io_context } {
	resolver.async_resolve(server_name, "nntp", [this](const error_code& error, const resolve_results& results) {start_connection(error, results); });
}

void News_reader::start_connection(const error_code& error, const resolve_results& results) {
	
}