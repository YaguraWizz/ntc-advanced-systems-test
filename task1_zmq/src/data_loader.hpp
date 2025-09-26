#pragma once
#include <optional>
#include <unordered_set>
#include <string>

#include "student.hpp"

namespace data_loader
{
    // Функция для загрузки и объединения данных из всех файлов в директории
    std::unordered_set<domain::Student, domain::Student::hash> load_all_students(const std::string& dir_path);
}
