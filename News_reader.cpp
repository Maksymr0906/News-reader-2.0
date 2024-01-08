#include "News_reader.hpp"

News_reader::News_reader(io::io_context& io_context, io::ssl::context& ssl_context, const std::string& server_name, WINDOW* window)
	:io_context{ io_context }, socket{ io_context, ssl_context }, window{ window }, server_name{ server_name }, is_data_returns{}, using_TLS{}, resolver{ io_context } {
	resolver.async_resolve(server_name, "nntp", [this](const error_code& ec, const resolve_results& results) {start_connection(ec, results); });
}

std::string News_reader::get_line() {
	std::string line{};
	std::getline(std::istream(&buffer), line);
	line.pop_back();
	return line;
}

void News_reader::update_status(const std::string& line) {
	if (isdigit(line.at(0)) && isdigit(line.at(1)) && isdigit(line.at(2))) {
		const int status = (line.at(0) - '0') * 100 + (line.at(1) - '0') * 10 + (line.at(2) - '0');
		this->status = static_cast<Status>(status);
	}
	else {
		this->status = Status::NONE;
	}
}

void News_reader::display_message(const std::string& message) {
	waddstr(window, message + '\n');
	wrefresh(window);
	if (get_command() != "LIST") {
		napms(200);
	}
}

void News_reader::receive_line(std::function<void(const error_code& ec, size_t length)> handler) {
	if (using_TLS) {
		io::async_read_until(socket, buffer, '\r\n', handler);
	}
	else {
		io::async_read_until(socket.next_layer(), buffer, '\r\n', handler);
	}
}

void News_reader::send_line(std::function<void(const error_code& ec, size_t length)> handler) {
	if (using_TLS) {
		io::async_write(socket, io::buffer(command), handler);
	}
	else {
		io::async_write(socket.next_layer(), io::buffer(command), handler);
	}
}

void News_reader::start_connection(const error_code& ec, const resolve_results& results) {
	if (ec) {
		display_message("Error: " + ec.message() + " resolving.");
		return;
	}

	display_message("Starting connection to " + server_name);
	io::async_connect(socket.lowest_layer(), results,
		[this](const error_code& ec, const tcp::endpoint& ep) {
			handle_connect(ec, ep);
		}
	);
}

void News_reader::handle_connect(const error_code& ec, const tcp::endpoint& ep) {
	if (ec) {
		display_message("Error: " + ec.message() + " connect to server.");
		return;
	}

	display_message("Connected to " + server_name);
	receive_line([this](const error_code& ec, size_t length) {handle_greeting(ec, length); });
}

void News_reader::handle_greeting(const error_code& ec, size_t lenght) {
	if (ec) {
		display_message("Error: " + ec.message() + " greeting to server.");
		return;
	}

	std::string greeting_line = get_line();
	update_status(greeting_line);

	if (status == Status::SERVICE_AVAILABLE_POSTING_ALLOWED || status == Status::SERVICE_AVAILABLE_POSTING_PROHIBITED) {
		send_command("CAPABILITIES", Status::CAPABILITY_LIST_FOLLOWS, true);
	}
}

void News_reader::send_command(const std::string& command, Status expected_status, bool is_data_returns) {
	this->command = command + "\r\n";
	this->expected_status = expected_status;
	this->is_data_returns = is_data_returns;
	data_block.clear();
	send_line([this](const error_code& error, size_t bytes_transferred) {handle_command_written(error, bytes_transferred); });
}

void News_reader::handle_command_written(const error_code& ec, size_t length) {
	if (ec) {
		display_message("Error: " + ec.message() + " sending " + get_command() + " command");
		return;
	}

	display_message("Sending " + get_command() + " command");
	receive_line([this](const error_code& ec, size_t length) { handle_command_response(ec, length); });
}

void News_reader::handle_command_response(const error_code& ec, size_t length) {
	if (ec) {
		display_message("Error: " + ec.message() + " reading status for " + server_name);
		return;
	}

	std::string command_response_line = get_line();
	update_status(command_response_line);
	display_message(command_response_line);

	if (status != expected_status) {
		display_message("Error: status != expected status");
		return;
	}

	if (is_data_returns) {
		receive_line([this](const error_code& ec, size_t length) {handle_data_block_line(ec, length); });
	}
	else {
		process_command_success();
	}
}

void News_reader::handle_data_block_line(const error_code& ec, size_t length) {
	if (ec) {
		display_message("Error: " + ec.message() + " reading data block line.");
		return;
	}

	const std::string line = get_line();

	if (line != ".") {
		display_message(line);
		data_block.push_back(line);
		receive_line([this](const error_code& error, size_t bytes_transferred) {handle_data_block_line(error, bytes_transferred); });
	}
	else {
		process_data_block();
	}
}

void News_reader::process_data_block() {
	if (get_command() == "CAPABILITIES") {
		 bool is_supports_TLS{};
		 for (const auto& line : data_block) {
			 if (line == "STARTTLS") {
				 is_supports_TLS = true;
				 break;
			 }
		 }

		 if (is_supports_TLS) {
			 send_command("STARTTLS", Status::BEGIN_TLS_NEGOTIATION, false);
		 }
	}
	else if (get_command() == "LIST") {
		const std::string group_info = data_block.at(rand() % data_block.size());
		group.initialize(group_info);
		send_command("GROUP " + group.get_group_name(), Status::ARTICLE_NUMBERS_FOLLOW, false);
	}
	else if (get_command().find("ARTICLE") != std::string::npos) {
		send_command("QUIT", Status::CLOSING_CONNECTION, false);
	}
}

void News_reader::process_command_success() {
	if (get_command() == "STARTTLS") {
		socket.async_handshake(io::ssl::stream_base::client, [this](const error_code& error) {handle_TLS_handshake(error); });
	}
	if (get_command().find("GROUP") != std::string::npos) {
		const int article_number = (rand() % group.get_highest_article_number()) + group.get_lowest_article_number();
		send_command("ARTICLE " + std::to_string(article_number), Status::ARTICLE_FOLLOWS, true);
	}
}

void News_reader::handle_TLS_handshake(const error_code& ec) {
	if (ec) {
		display_message("Error:  " + ec.message() + " negotiating TLS " + server_name);
		return;
	}

	using_TLS = true;
	display_message("TLS connected.");
	send_command("LIST", Status::LIST_FOLLOWS, true);
}