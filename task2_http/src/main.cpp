#include <iostream>
#include <string>
#include <vector>
#include <optional>
#include <regex>
#include <functional>
#include <cmath>
#include "crow.h"
#include "analyze.hpp"
#include "types.hpp"
#include "utils.hpp"
#include <nlohmann/json.hpp>

using namespace types;
using namespace analyze;
using namespace utils;

// Важно: эта вспомогательная функция должна быть определена здесь,
// чтобы иметь доступ к enum types::CoordFormat и функции валидации.
// Также она была перенесена из main.cpp в types.hpp в более ранних версиях,
// но для простоты я оставлю ее здесь.
static std::optional<std::pair<Coordinate, CoordinateMetadata>> create_coord_pair(
    double lat, double lon, CoordFormat format, const std::string& raw_match, const std::string& error = "")
{
    Coordinate coord;
    coord.lat = lat;
    coord.lon = lon;
    coord.format = format;
    // Используем статическую функцию валидации из types::Coordinate
    coord.valid = Coordinate::check_lat_lon_valid(lat, lon) && error.empty();

    CoordinateMetadata metadata;
    metadata.raw_match = raw_match;
    if (!error.empty()) {
        metadata.errors.push_back(error);
    }

    return std::make_pair(coord, metadata);
}

// Вспомогательная функция для проверки валидности координат
// В types.hpp этой функции не видно, поэтому я определю ее здесь или в utils.hpp.
// Предполагая, что она есть в types.hpp, как в последних версиях, используем ее оттуда.

// =============================================================================
// 1. DECIMAL - Формат: 45.123, -122.456 (самый общий)
// =============================================================================
static std::optional<std::pair<Coordinate, CoordinateMetadata>> parse_decimal_pair(const std::smatch& m, const MatchContext& ctx) {
    bool ok1 = false, ok2 = false;
    double lat = stod_norm(m[1].str(), ok1);
    double lon = stod_norm(m[3].str(), ok2); // Группа 3, так как группа 2 - разделитель

    std::string error;
    if (!ok1 || !ok2) error = "stod_conversion_failed";
    if (!Coordinate::check_lat_lon_valid(lat, lon)) error = "invalid_range";

    return create_coord_pair(lat, lon, CoordFormat::DECIMAL, std::string(ctx.raw_match_sv), error);
}

// =============================================================================
// 2. HEMI_DECIMAL - Формат: N45.123 W122.456 (со сторонами света)
// =============================================================================
static std::optional<std::pair<Coordinate, CoordinateMetadata>> parse_hemi_decimal(const std::smatch& m, const MatchContext& ctx) {
    bool ok1 = false, ok2 = false;
    // Группа 1: Полушарие широты, Группа 2: Широта (число)
    // Группа 4: Полушарие долготы, Группа 5: Долгота (число)
    double lat_val = stod_norm(m[2].str(), ok1);
    double lon_val = stod_norm(m[5].str(), ok2);
    char lat_hemi = std::toupper(m[1].str()[0]);
    char lon_hemi = std::toupper(m[4].str()[0]);

    if (lat_hemi == 'S') lat_val *= -1.0;
    if (lon_hemi == 'W' || lon_hemi == 'O') lon_val *= -1.0;

    std::string error;
    if (!ok1 || !ok2) error = "stod_conversion_failed";
    if (!Coordinate::check_lat_lon_valid(lat_val, lon_val)) error = "invalid_range";

    return create_coord_pair(lat_val, lon_val, CoordFormat::HEMI_DECIMAL, std::string(ctx.raw_match_sv), error);
}

// =============================================================================
// 3. DEG_MIN - Формат: 51° 12.32' N, 0° 5.12' E
// =============================================================================
static std::optional<std::pair<Coordinate, CoordinateMetadata>> parse_deg_min(const std::smatch& m, const MatchContext& ctx) {
    bool ok_lat_deg = false, ok_lat_min = false;
    bool ok_lon_deg = false, ok_lon_min = false;

    // Лат: Группа 1(градусы), Группа 2(минуты), Группа 3(полушарие)
    double lat_deg = stod_norm(m[1].str(), ok_lat_deg);
    double lat_min = stod_norm(m[2].str(), ok_lat_min);
    char lat_hemi = std::toupper(m[3].str()[0]);

    // Лон: Группа 4(градусы), Группа 5(минуты), Группа 6(полушарие)
    double lon_deg = stod_norm(m[4].str(), ok_lon_deg);
    double lon_min = stod_norm(m[5].str(), ok_lon_min);
    char lon_hemi = std::toupper(m[6].str()[0]);

    double lat_val = utils::dms_to_decimal(lat_deg, lat_min, 0.0, lat_hemi);
    double lon_val = utils::dms_to_decimal(lon_deg, lon_min, 0.0, lon_hemi);

    std::string error;
    if (!ok_lat_deg || !ok_lat_min || !ok_lon_deg || !ok_lon_min) error = "stod_conversion_failed";
    if (!Coordinate::check_lat_lon_valid(lat_val, lon_val)) error = "invalid_range";

    return create_coord_pair(lat_val, lon_val, CoordFormat::DEG_MIN, std::string(ctx.raw_match_sv), error);
}

// =============================================================================
// 4. DEG_MIN_SEC - Формат: 51° 12' 32.21'' N, 0° 5' 12.3'' E
// =============================================================================
static std::optional<std::pair<Coordinate, CoordinateMetadata>> parse_deg_min_sec(const std::smatch& m, const MatchContext& ctx) {
    bool ok_lat_d = false, ok_lat_m = false, ok_lat_s = false;
    bool ok_lon_d = false, ok_lon_m = false, ok_lon_s = false;

    // Лат: Группа 1(град), Группа 2(мин), Группа 3(сек), Группа 4(полушарие)
    double lat_deg = stod_norm(m[1].str(), ok_lat_d);
    double lat_min = stod_norm(m[2].str(), ok_lat_m);
    double lat_sec = stod_norm(m[3].str(), ok_lat_s);
    char lat_hemi = std::toupper(m[4].str()[0]);

    // Лон: Группа 5(град), Группа 6(мин), Группа 7(сек), Группа 8(полушарие)
    double lon_deg = stod_norm(m[5].str(), ok_lon_d);
    double lon_min = stod_norm(m[6].str(), ok_lon_m);
    double lon_sec = stod_norm(m[7].str(), ok_lon_s);
    char lon_hemi = std::toupper(m[8].str()[0]);

    double lat_val = utils::dms_to_decimal(lat_deg, lat_min, lat_sec, lat_hemi);
    double lon_val = utils::dms_to_decimal(lon_deg, lon_min, lon_sec, lon_hemi);

    std::string error;
    if (!ok_lat_d || !ok_lat_m || !ok_lat_s || !ok_lon_d || !ok_lon_m || !ok_lon_s) error = "stod_conversion_failed";
    if (!Coordinate::check_lat_lon_valid(lat_val, lon_val)) error = "invalid_range";

    return create_coord_pair(lat_val, lon_val, CoordFormat::DEG_MIN_SEC, std::string(ctx.raw_match_sv), error);
}

// =============================================================================
// 5. GOOGLE_STYLE - Формат: 55.755831°, 37.617673°
// (Почти как DECIMAL, но с символом градуса)
// =============================================================================
static std::optional<std::pair<Coordinate, CoordinateMetadata>> parse_google_style(const std::smatch& m, const MatchContext& ctx) {
    // Группа 1: Широта, Группа 2: Долгота
    bool ok1 = false, ok2 = false;
    double lat = stod_norm(m[1].str(), ok1);
    double lon = stod_norm(m[2].str(), ok2);

    std::string error;
    if (!ok1 || !ok2) error = "stod_conversion_failed";
    if (!Coordinate::check_lat_lon_valid(lat, lon)) error = "invalid_range";

    return create_coord_pair(lat, lon, CoordFormat::GOOGLE_STYLE, std::string(ctx.raw_match_sv), error);
}


// Функция для настройки анализатора
void setup_analyzer(analyze::CoordinateAnalyzer& analyzer) {
    // Базовые компоненты регулярных выражений:
    // **ВАЖНО**: Используем (?:...) для не-захватывающих групп, чтобы не засорять smatch,
    // если это не нужно для парсера.
    const std::string NUM_PATTERN = R"([+-]?\d{1,3}(?:[.,]\d+))"; // Должно быть десятичное число
    const std::string SEP_PATTERN = R"(?:\s*[,;\s]\s*)"; // Разделитель: пробел, запятая, точка с запятой
    const std::string HEMI_PATTERN = R"([NSWEONnsweo])"; // Полушария N, S, W, E, O (O для East/Ost)
    const std::string DEG_PATTERN = R"(\d{1,3})"; // Градусы (целое число)
    const std::string MIN_SEC_PATTERN = R"(\d{1,2}(?:[.,]\d+)?)"; // Минуты/Секунды (с возможной дробной частью)

    // --- ПАРСЕРЫ ---

    // 4. DEG_MIN_SEC - ПРИОРИТЕТ 50 (самый специфичный)
    // Формат: (D)° (M)' (S)'' (H), (D)° (M)' (S)'' (H)
    // Внимание: REGEX был исправлен для корректного экранирования скобок и символов:
    analyzer.register_pattern(
        R"((" + DEG_PATTERN + R")[°\s]\s*()" + MIN_SEC_PATTERN + R"(')\s*()" + MIN_SEC_PATTERN + R"([\\s'"]\s* ()" + HEMI_PATTERN + R"())" + // Лат: (D)(M)(S)(H) - Группы 1-4
        SEP_PATTERN +
        R"((" + DEG_PATTERN + R")[°\s]\s*()" + MIN_SEC_PATTERN + R"(')\s*()" + MIN_SEC_PATTERN + R"([\\s'"]\s* ()" + HEMI_PATTERN + R"())", // Лон: (D)(M)(S)(H) - Группы 5-8
        parse_deg_min_sec, 50);

        // 3. DEG_MIN - ПРИОРИТЕТ 40
        // Формат: (D)° (M)' (H), (D)° (M)' (H)
        // Внимание: REGEX был исправлен для корректного экранирования скобок и символов:
        analyzer.register_pattern(
            R"((" + DEG_PATTERN + R")[°\s]\s*()" + MIN_SEC_PATTERN + R"('[\s,]*()" + HEMI_PATTERN + R"())" + // Лат: (D)(M)(H) - Группы 1-3
            SEP_PATTERN +
            R"((" + DEG_PATTERN + R")[°\s]\s*()" + MIN_SEC_PATTERN + R"('[\s,]*()" + HEMI_PATTERN + R"())", // Лон: (D)(M)(H) - Группы 4-6
            parse_deg_min, 40);

        // 2. HEMI_DECIMAL - ПРИОРИТЕТ 30
        // Формат: (H)(ЧИСЛО) (H)(ЧИСЛО)
        analyzer.register_pattern(
            R"((" + HEMI_PATTERN + R")\s*()" + NUM_PATTERN + R"())" + SEP_PATTERN + R"(()" + HEMI_PATTERN + R"()\s*()" + NUM_PATTERN + R"())", // Лат: (H)(Ч) - Группы 1-2. Лон: (H)(Ч) - Группы 4-5
            parse_hemi_decimal, 30);

        // 5. GOOGLE_STYLE (более специфичный DECIMAL с символом градуса) - ПРИОРИТЕТ 20
        // Формат: (ЧИСЛО)° (ЧИСЛО)°
        analyzer.register_pattern(
            R"((" + NUM_PATTERN + R")[°]\s*)" + SEP_PATTERN + R"(\s*()" + NUM_PATTERN + R"()[°])", // (Ч) (Ч) - Группы 1, 2
            parse_google_style, 20);

        // 1. DECIMAL (простой) - ПРИОРИТЕТ 10
        // Формат: (ЧИСЛО) (ЧИСЛО)
        analyzer.register_pattern(
            R"((" + NUM_PATTERN + R")" + SEP_PATTERN + R"(()" + NUM_PATTERN + R"())", // (Ч) (Ч) - Группы 1, 3
            parse_decimal_pair, 10);
}

int main() {
    crow::SimpleApp server;

    // Создаем и настраиваем анализатор
    analyze::CoordinateAnalyzer analyzer;
    setup_analyzer(analyzer);

    // Определение маршрута для API-анализа
    CROW_ROUTE(server, "/analyze").methods(crow::HTTPMethod::Post)
        ([&analyzer](const crow::request& req) {
        crow::json::rvalue body = crow::json::load(req.body);
        if (!body || !body.has("text")) {
            return crow::response(crow::status::BAD_REQUEST, "Missing 'text' field in JSON body");
        }

        std::string text_to_analyze = body["text"].s();
        types::CoordinateSet result = analyzer.analyze(text_to_analyze);

        // Используем nlohmann::json для красивого вывода
        nlohmann::json j = result;
        return crow::response(crow::status::OK, j.dump(4));
            });

    // Определение маршрута для корневого URL ("/")
    CROW_ROUTE(server, "/")
        ([]() {
        return "Service is running. Use POST /analyze with JSON body {\"text\": \"your text here\"}.";
            });

    // Запускаем сервер на порту 5556
    std::cout << "Starting server on port 5556" << std::endl;
    server.port(5556).run();

    return 0;
}
