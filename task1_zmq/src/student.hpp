#pragma once
#include <string>
#include <chrono>
#include <format>
#include <nlohmann/json.hpp>

namespace domain {

	struct Student
	{
		uint16_t id{};
		std::string fio;
		std::chrono::year_month_day birth_date{};

		inline bool operator==(const Student& other) const {
			return fio == other.fio && birth_date == other.birth_date;
		}

		inline bool operator<(const Student& other) const {
			return fio < other.fio;
		}

		struct hash {
			size_t operator()(const domain::Student& s) const {
				size_t h1 = std::hash<std::string>{}(s.fio);
				size_t h2 = std::hash<int>{}(static_cast<int>(s.birth_date.year()));
				size_t h3 = std::hash<unsigned int>{}(static_cast<unsigned int>(s.birth_date.month()));
				size_t h4 = std::hash<unsigned int>{}(static_cast<unsigned int>(s.birth_date.day()));

				return h1 ^ (h2 << 1) ^ (h3 << 2) ^ (h4 << 3);
			}
		};
	};


	static void from_json(const nlohmann::json& j, Student& s) {
		// ID может отсутствовать при приеме данных, если мы его не передаем, но для полной структуры оставим
		if (j.contains("id")) {
			j.at("id").get_to(s.id);
		}
		j.at("fio").get_to(s.fio);

		std::string date_str = j.at("birth_date").get<std::string>();
		std::istringstream ss(date_str);

		// Парсинг даты из строки "ДД.ММ.ГГГГ"
		std::chrono::year_month_day date;
		ss >> std::chrono::parse("%d.%m.%Y", date);
		s.birth_date = date;

		if (!s.birth_date.ok()) {
			throw std::runtime_error("Invalid date format in JSON: " + date_str);
		}
	}

	static void to_json(nlohmann::json& j, const Student& s) {
		std::stringstream ss;
		// Форматируем дату обратно в ДД.ММ.ГГГГ
		ss << std::format("{:%d.%m.%Y}", s.birth_date);
		j = nlohmann::json{
			{"id", s.id},
			{"fio", s.fio},
			{"birth_date", ss.str()}
		};
	}
}