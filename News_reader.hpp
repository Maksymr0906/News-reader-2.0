#pragma once

#include "Curses.hpp"

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <vector>
#include <sstream>

namespace io = boost::asio;
using tcp = io::ip::tcp;
using error_code = boost::system::error_code;
using resolve_results = tcp::resolver::results_type;

enum class Status {
	NONE = 0,
	CAPABILITY_LIST_FOLLOWS = 101,
	SERVICE_AVAILABLE_POSTING_ALLOWED = 200,
	SERVICE_AVAILABLE_POSTING_PROHIBITED = 201,
	CLOSING_CONNECTION = 205,
	ARTICLE_NUMBERS_FOLLOW = 211,
	LIST_FOLLOWS = 215,
	ARTICLE_FOLLOWS = 220,
	ARTICLE_FOUND = 223,
	BEGIN_TLS_NEGOTIATION = 382,
	SERVICE_TEMPORARILY_UNAVAILABLE = 400,
	SERVICE_PERMANENTLY_UNAVAILABLE = 502
};

class News_reader {
	using Socket = io::ssl::stream<tcp::socket>;
private:
	io::io_context& io_context;
	Socket socket;
	Status expected_status;
	Status status;
	io::streambuf buffer;
	std::string server_name;
	std::string command;
	WINDOW* window;
	bool using_TLS;
	bool is_data_returns;
	tcp::resolver resolver;
	std::vector<std::string> data_block;
public:
	News_reader(io::io_context& io_context, io::ssl::context& ssl_context, const std::string& server_name, WINDOW* window);
	void start_connection(const error_code& error, const resolve_results& results);
};

