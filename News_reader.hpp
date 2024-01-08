#pragma once

#include "Curses.hpp"

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <vector>
#include <sstream>
#include <time.h>

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

class Group {
private:
	std::string group_name;
	int lowest_article_number;
	int highest_article_number;
public:
	Group()
		:group_name{}, lowest_article_number{}, highest_article_number{} {}
	void initialize(const std::string& group_info) {
		std::istringstream iss(group_info);
		iss >> group_name >> highest_article_number >> lowest_article_number;
	}
	std::string get_group_name() const { return group_name; }
	int get_lowest_article_number() const { return lowest_article_number; }
	int get_highest_article_number() const { return highest_article_number; }
};

class News_reader {
	using Socket = io::ssl::stream<tcp::socket>;
private:
	io::io_context& io_context;
	Socket socket;
	Status expected_status;
	Status status;
	Group group;
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
	std::string get_line();
	std::string get_command() const { return command.substr(0, command.size() - 2); }
	void update_status(const std::string& line);
	void display_message(const std::string& message);
	void receive_line(std::function<void(const error_code& ec, size_t lenght)> handler);
	void send_line(std::function<void(const error_code& ec, size_t lenght)> handler);
	void send_command(const std::string& command, Status expected_status, bool is_data_returns);

	void start_connection(const error_code& error, const resolve_results& results);
	void handle_connect(const error_code& ec, const tcp::endpoint& ep);
	void handle_greeting(const error_code& ec, size_t lenght);
	void handle_command_written(const error_code& ec, size_t length);
	void handle_command_response(const error_code& ec, size_t length);
	void handle_data_block_line(const error_code& ec, size_t length);
	void process_command_success();
	void process_data_block();
	void handle_TLS_handshake(const error_code& ec);
};