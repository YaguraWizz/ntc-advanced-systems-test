#include "data_loader.hpp"
#include <regex>
#include <iostream>
#include <fstream>
#include <filesystem>


namespace data_loader
{
    namespace fs = std::filesystem;
    using namespace std::string_literals;

    static std::optional<domain::Student> read_student_from_line(const std::string& line) {
        // Шаблон: ID + Пробелы + ФИО + Пробелы + Дата
        // Используем универсальный шаблон, не полагаясь на Unicode-свойства
        std::regex student_regex(R"((\d+)\s+([^\d]+)\s+(\d{1,2}\.\d{1,2}\.\d{4})\s*)");
        std::smatch matches;

        if (std::regex_match(line, matches, student_regex) && matches.size() == 4) {
            try {
                uint16_t id = static_cast<uint16_t>(std::stoul(matches[1].str()));
                std::string fio = matches[2].str();
                std::string date_str = matches[3].str();

                // Удаляем начальные и конечные пробелы из ФИО
                size_t first = fio.find_first_not_of(' ');
                size_t last = fio.find_last_not_of(' ');
                if (first == std::string::npos) {
                    fio = "";
                }
                else {
                    fio = fio.substr(first, (last - first + 1));
                }

                // Парсим дату
                std::chrono::year_month_day birth_date{};
                std::istringstream date_ss(date_str);
                date_ss >> std::chrono::parse("%d.%m.%Y", birth_date);

                if (!birth_date.ok()) {
                    std::cerr << "Validation Error (Date): Invalid date value in line: " << line << std::endl;
                    return std::nullopt;
                }

                if (fio.empty()) {
                    std::cerr << "Validation Error (FIO): FIO is empty or contains only spaces in line: " << line << std::endl;
                    return std::nullopt;
                }

                return domain::Student{ id, fio, birth_date };

            }
            catch (const std::exception& e) {
                std::cerr << "Parsing Error (Exception): " << e.what() << " in line: " << line << std::endl;
                return std::nullopt;
            }
        }

        std::cerr << "Parsing Error (Format): Invalid line format: " << line << std::endl;
        return std::nullopt;
    }

    // Функция для загрузки и объединения данных из всех файлов в директории
    std::unordered_set<domain::Student, domain::Student::hash> load_all_students(const std::string& dir_path) {
        // Используем unordered_set для автоматического объединения уникальных студентов
        std::unordered_set<domain::Student, domain::Student::hash> combined_students;

        // Проходим по всем файлам в директории
        try {
            for (const auto& entry : fs::directory_iterator(dir_path)) {
                if (entry.is_regular_file() && entry.path().extension() == ".txt") {
                    std::ifstream ifs(entry.path());
                    if (!ifs.is_open()) {
                        std::cerr << "Error: Could not open file " << entry.path().string() << std::endl;
                        continue;
                    }

                    std::string line;
                    while (std::getline(ifs, line)) {
                        if (auto student = read_student_from_line(line)) {
                            combined_students.insert(*student);
                        }
                    }
                }
            }
        }
        catch (const fs::filesystem_error& e) {
            std::cerr << "Filesystem Error: " << e.what() << std::endl;
        }

        return combined_students;
    }
}
