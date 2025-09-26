#pragma once
#include <thread>
#include <atomic>
#include <zmq.hpp>


#include "student.hpp"
#include "data_loader.hpp"

namespace server {
	enum  TypeMode { Listener, Publisher };

	struct Options {
		TypeMode typeMode;
		std::string url;
		std::optional<std::string> dir;
	};

    class Server {
    public:
        explicit Server(const Options& opts)
            : options(opts) {
        }

        void run(std::atomic<bool>& running_flag) {
            if (options.typeMode == TypeMode::Listener) {
                // Сервер/Publisher: Чтение файлов и отправка данных
                listenerThread = std::thread(&Server::serverLoop, this, std::ref(running_flag));
            }
            else if (options.typeMode == TypeMode::Publisher) {
                // Клиент/Subscriber: Получение данных, сортировка и вывод
                publisherThread = std::thread(&Server::clientLoop, this, std::ref(running_flag));
            }

            // Основной поток блокируется, чтобы приложение не завершилось,
            // пока работают рабочие потоки.
            if (listenerThread.joinable()) listenerThread.join();
            if (publisherThread.joinable()) publisherThread.join();
        }

        void stop() { running = false; }

    private:
        Options options;
        std::atomic<bool> running;
        zmq::context_t zmq_context;

        std::thread listenerThread;
        std::thread publisherThread;

        void serverLoop(std::atomic<bool>& running_flag);
        void clientLoop(std::atomic<bool>& running_flag);
        void displayStudents(const std::vector<domain::Student>& students) const;
    };
}