#include <iostream>
#include <signal.h>

#include "CLI/CLI.hpp"
#include "server.hpp"

std::atomic<bool> g_is_running(true);

namespace {
	static void signalHandler(int signum) {
		// Регистрируем обработчик сигнала SIGINT (Ctrl+C)
		std::cout << "\n\n Received interrupt signal (" << signum << "). Shutting down gracefully..." << std::endl;
		g_is_running.store(false);
	}
}


int main(int argc, char** argv) {
	signal(SIGINT, signalHandler);

	CLI::App app{ "StudentApp - server/client for student data via ZeroMQ" };

	std::string mode;
	std::string dir;
	std::string url = "tcp://127.0.0.1:5555";

	app.add_option("-m,--mode", mode, "Mode: server or client")->required();
	app.add_option("-d,--dir", dir, "Directory with student files (server only)");
	app.add_option("-u,--url", url, "Connection URL (default tcp://127.0.0.1:5555)");

	try {
		app.parse(argc, argv);
	}
	catch (const CLI::ParseError& e) {
		return app.exit(e);
	}

	std::unique_ptr<server::Server> server_ptr;

	if (mode == "server") {
		if (dir.empty()) {
			std::cerr << "Error: directory required in server mode! Use -d or --dir." << std::endl;
			return 1;
		}
		std::cout << "Starting server (PUB) at " << url << " with data from " << dir << std::endl;
		server_ptr = std::make_unique<server::Server>(server::Options{ server::TypeMode::Listener, url, dir });
		server_ptr->run(g_is_running); // Передаем флаг в метод run
	}
	else if (mode == "client") {
		std::cout << "Starting client (SUB), listening at " << url << std::endl;
		server_ptr = std::make_unique<server::Server>(server::Options{ server::TypeMode::Publisher, url });
		server_ptr->run(g_is_running); // Передаем флаг в метод run
	}
	else {
		std::cerr << "Unknown mode: " << mode << ". Use 'server' or 'client'." << std::endl;
		return 1;
	}

	std::cout << "Application finished gracefully." << std::endl;
	return 0;
}
