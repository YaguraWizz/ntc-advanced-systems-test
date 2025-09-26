#include "server.hpp"
#include <iostream>

namespace server {


    void Server::serverLoop(std::atomic<bool>& running_flag) {
        try {
            zmq::socket_t publisher(zmq_context, zmq::socket_type::pub);
            publisher.bind(options.url);
            std::cout << "ZMQ PUB Server bound to: " << options.url << std::endl;

            auto students_set = data_loader::load_all_students(*options.dir);
            std::vector<domain::Student> students_list(students_set.begin(), students_set.end());
            std::cout << "Total unique students found: " << students_list.size() << std::endl;

            nlohmann::json j_students = students_list;
            std::string message_data = j_students.dump();

            // Используем переданный флаг
            while (running_flag) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                zmq::message_t message(message_data.begin(), message_data.end());
                publisher.send(message, zmq::send_flags::none);

                std::cout << "Published student data (" << message_data.size()
                    << " bytes). Waiting for next update..." << std::endl;

                std::this_thread::sleep_for(std::chrono::seconds(5));
            }
        }
        catch (const zmq::error_t& e) {
            std::cerr << "ZMQ Error (Server): " << e.what() << std::endl;
        }
        catch (const std::exception& e) {
            std::cerr << "Standard Exception (Server): " << e.what() << std::endl;
        }
    }


    void Server::clientLoop(std::atomic<bool>& running_flag) {
        try {
            zmq::socket_t subscriber(zmq_context, zmq::socket_type::sub);
            subscriber.connect(options.url);
            std::cout << "ZMQ SUB Client connected to: " << options.url << std::endl;
            subscriber.set(zmq::sockopt::subscribe, "");

            // Используем переданный флаг
            while (running_flag) {
                zmq::message_t received_message;
                auto result = subscriber.recv(received_message, zmq::recv_flags::none);

                if (result.has_value()) {
                    std::string message_data(static_cast<char*>(received_message.data()), received_message.size());
                    std::cout << "\nReceived new data batch (" << message_data.size() << " bytes)." << std::endl;

                    std::vector<domain::Student> students;
                    try {
                        nlohmann::json j = nlohmann::json::parse(message_data);
                        students = j.get<std::vector<domain::Student>>();
                    }
                    catch (const std::exception& e) {
                        std::cerr << "JSON Deserialization Error: " << e.what() << std::endl;
                        continue;
                    }

                    std::sort(students.begin(), students.end(), [](const domain::Student& a, const domain::Student& b) {
                        return a.fio < b.fio;
                        });
                    displayStudents(students);
                }
            }
        }
        catch (const zmq::error_t& e) {
            std::cerr << "ZMQ Error (Client): " << e.what() << std::endl;
        }
        catch (const std::exception& e) {
            std::cerr << "Standard Exception (Client): " << e.what() << std::endl;
        }
    }

    void Server::displayStudents(const std::vector<domain::Student>& students) const {
        std::cout << "\n=======================================================" << std::endl;
        std::cout << "       Sorted Student List (Total: " << students.size() << ")" << std::endl;
        std::cout << "=======================================================" << std::endl;

        for (size_t i = 0; i < students.size(); ++i) {
            std::stringstream ss;
            ss << std::format("{:%d.%m.%Y}", students[i].birth_date);

            std::cout << std::left
                << std::setw(3) << (i + 1) << ". "
                << std::setw(30) << students[i].fio
                << " | "
                << std::setw(10) << ss.str()
                << " (ID: " << students[i].id << ")"
                << std::endl;
        }
        std::cout << "=======================================================" << std::endl;
    }
}