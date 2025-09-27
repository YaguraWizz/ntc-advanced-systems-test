#include <iostream>
#include <string>
#include <vector>
#include <cmath>
#include <sstream>
#include <algorithm>
#include <iomanip>
#include <regex>


#include "crow.h"
#include <nlohmann/json.hpp>
#include "CLI/CLI.hpp"

using json = nlohmann::json;

// --- 1. Структуры данных и константы ---

/**
 * @brief Структура для хранения данных об одной найденной координате.
 */
struct Coordinate {
    std::string original_text;      ///< Исходный текст координаты
    double lat_dd = 0.0;            ///< Широта в десятичных градусах (Decimal Degrees)
    double lon_dd = 0.0;            ///< Долгота в десятичных градусах (Decimal Degrees)
    std::string format;             ///< Определенный формат (DD, DMS, DDM, Mixed)
    bool is_valid = false;          ///< Флаг валидности (в пределах [-90, 90] и [-180, 180])
    std::string label;              ///< Выделенная метка/название
    std::string sentence_context;   ///< Предложение-контекст (до 200 символов)
};

// --- 2. Вспомогательные функции для парсинга и анализа ---

/**
 * @brief Преобразует градусы, минуты, секунды в десятичные градусы.
 * @return Значение в десятичных градусах.
 */
static double dms_to_dd(double deg, double min, double sec) {
    return deg + min / 60.0 + sec / 3600.0;
}

/**
 * @brief Нормализует строку компонента координаты и извлекает DD.
 *
 * @param geo_str Строка компонента (например, "76°00′00″ с.ш.", "N80.4551").
 * @param is_latitude Флаг, указывающий, является ли это широтой (true) или долготой (false).
 * @param format Ссылка для записи определенного формата.
 * @return Значение в десятичных градусах, 999.0 в случае ошибки/невалидности.
 */
static double normalize_and_validate_component(const std::string& geo_str, bool is_latitude, std::string& format) {
    std::string s = geo_str;
    // Заменяем русские запятые на точки и удаляем символы, которые могли быть захвачены
    std::replace(s.begin(), s.end(), ',', '.');

    // Находим направление (N, S, E, W, С, Ю, В, З)
    char direction = ' ';
    std::string clean_val;
    
    // Ищем направление и очищаем от знаков препинания и букв
    for (char c : s) {
        if (std::isalpha(static_cast<unsigned char>(c))) {
            char upper_c = std::toupper(static_cast<unsigned char>(c));
            if (is_latitude) {
                if (upper_c == 'N' || upper_c == 'S' || upper_c == 'C' || upper_c == 'Ю') {
                    direction = (direction == ' ') ? upper_c : direction;
                }
            } else { // Долгота
                if (upper_c == 'E' || upper_c == 'W' || upper_c == 'В' || upper_c == 'З') {
                    direction = (direction == ' ') ? upper_c : direction;
                }
            }
        }
        if (std::isdigit(c) || c == '.' || c == ' ' || c == '-') {
            clean_val += c;
        }
    }

    // Удаляем лишние пробелы и очищаем от множественных пробелов
    std::stringstream val_ss(clean_val);
    std::vector<double> parts;
    double p;
    while (val_ss >> p) {
        parts.push_back(p);
    }

    double dd = 0.0;

    if (parts.size() == 1) { // Decimal Degrees (DD)
        dd = parts[0];
        format = "DD";
    } else if (parts.size() == 2) { // Degrees Decimal Minutes (DDM) - Deg Min.min
        if (parts[1] >= 60.0) return 999.0; // Минуты >= 60
        dd = dms_to_dd(parts[0], parts[1], 0.0);
        format = "DDM";
    } else if (parts.size() == 3) { // Degrees Minutes Seconds (DMS) - Deg Min Sec
        if (parts[1] >= 60.0 || parts[2] >= 60.0) return 999.0; // Минуты/секунды >= 60
        dd = dms_to_dd(parts[0], parts[1], parts[2]);
        format = "DMS";
    } else {
        return 999.0; // Неизвестный формат
    }

    // Применяем знак, если есть направление (Юг/Запад -> отрицательное значение)
    if (direction == 'S' || direction == 'Ю' || direction == 'W' || direction == 'З') {
        dd = -std::abs(dd);
    } else {
        dd = std::abs(dd);
    }

    // Проверка на валидность (границы)
    double max_val = is_latitude ? 90.0 : 180.0;
    if (std::abs(dd) > max_val) {
        return 999.0; // Недопустимое значение
    }

    return dd;
}

/**
 * @brief Извлекает предложение, содержащее совпадение, обрезает до 200 символов.
 */
static std::string find_sentence_context(const std::string& text, size_t pos, size_t length) {
    // 1. Находим начало предложения
    size_t sentence_start = 0;
    // Ищем назад разделители предложений: .?! за которыми следует пробел или перенос строки
    for (size_t i = pos; i-- > 0; ) {
        if (i < text.size() - 1 && (text[i] == '.' || text[i] == '?' || text[i] == '!') && std::isspace(text[i+1])) {
            sentence_start = i + 2; // Переходим после разделителя и пробела
            break;
        }
        if (text[i] == '\n') {
             sentence_start = i + 1; // Начинаем с новой строки
             break;
        }
    }

    // 2. Находим конец предложения
    size_t sentence_end = text.find_first_of(".?!", pos + length);
    if (sentence_end == std::string::npos) {
        sentence_end = text.length();
    } else {
        sentence_end++; // Включаем знак препинания
    }


    std::string sentence = text.substr(sentence_start, sentence_end - sentence_start);

    // 3. Удаляем ведущие/конечные пробелы/переносы строки
    size_t first = sentence.find_first_not_of(" \t\n\r");
    size_t last = sentence.find_last_not_of(" \t\n\r");
    if (first == std::string::npos) return ""; 
    sentence = sentence.substr(first, (last - first + 1));

    // 4. Обрезка до 200 символов
    if (sentence.length() > 200) {
        sentence = sentence.substr(0, 197) + "...";
    }

    return sentence;
}

/**
 * @brief Извлекает потенциальную метку/название из текста перед совпадением.
 *
 * Ищет ключевые слова или слова с заглавной буквы перед двоеточием, тире или непосредственно перед координатой.
 */
static std::string find_label(const std::string& text, size_t pos) {
    size_t max_lookback = 40;
    size_t start = (pos > max_lookback) ? pos - max_lookback : 0;
    std::string lookback_text = text.substr(start, pos - start);

    // Паттерн для поиска слов (включая русские и латинские), предшествующих двоеточию или тире.
    // Ищем с конца, чтобы найти ближайший заголовок
    std::regex label_regex(R"(([^.,;!?\n\r]{1,15}\s*(?:[.:-]\s*)?[\s\S]*))", std::regex::icase);
    std::smatch match;

    std::string potential_label;

    // Ищем последние несколько слов
    if (std::regex_search(lookback_text, match, label_regex)) {
        potential_label = match[0].str();
        std::reverse(potential_label.begin(), potential_label.end()); // Обращаем обратно

        // Обрезаем, чтобы удалить все до первого не-пробельного символа с конца (игнорируя разделитель)
        size_t last_space = potential_label.find_last_not_of(" \t\n\r.:-");
        if (last_space != std::string::npos) {
            potential_label = potential_label.substr(0, last_space + 1);
        }

        // Выбираем только слова, которые могут быть метками (начинаются с заглавной буквы или являются ключевыми)
        std::stringstream ss(potential_label);
        std::string word, result_label;
        std::vector<std::string> keywords = {"Точка", "Мыс", "Вершина", "Цель", "Point"};
        while (ss >> word) {
            std::string upper_word = word;
            if (!upper_word.empty()) upper_word[0] = std::toupper(static_cast<unsigned char>(upper_word[0]));

            bool is_keyword = std::any_of(keywords.begin(), keywords.end(), [&](const std::string& kw){
                return upper_word.find(kw) == 0;
            });

            if (!word.empty() && (std::isupper(static_cast<unsigned char>(word[0])) || is_keyword)) {
                result_label += word + " ";
            }
        }
        if (!result_label.empty()) {
            result_label.pop_back();
            return result_label;
        }
    }

    return "";
}


/**
 * @brief Сравнивает две координаты с учетом погрешности (для замкнутого полигона).
 */
static bool coords_match(const Coordinate& c1, const Coordinate& c2, double tolerance = 0.0001) {
    return std::abs(c1.lat_dd - c2.lat_dd) < tolerance &&
           std::abs(c1.lon_dd - c2.lon_dd) < tolerance;
}

// --- 3. Основной обработчик логики ---

/**
 * @brief Обрабатывает POST-запрос с текстом, извлекает и классифицирует координаты.
 * @param text Исходный текст для анализа.
 * @return JSON-объект с результатами.
 */
static json analyze_geo_text(const std::string& text) {
    // Комплексное регулярное выражение для поиска потенциальных пар координат.
    // Группы 1/3/5/7 - Опциональные направляющие буквы (N/S/C/Ю/E/W/В/З)
    // Группы 2/6 - Числовое значение координаты (поддерживает DD/DMS/DDM с разными разделителями . , ° ' " )
    // Группа 4 - Разделитель между координатами (пробелы, запятые, дефисы, а также слова 'и', 'или', 'через' и переносы строки)
    // NOTE: Переносы строки (\s*) включены в разделители.
    const std::regex GEO_PAIR_REGEX(
        R"(([NSСЮЕWВЗ]?)\s*([\d]{1,3}[°\s']?[\d]{0,2}[.,\s']?[\d]{0,6}[′"\s]?[.,\s']?[\d]{0,6}[″"\s']?)([NSСЮЕWВЗ]?)\s*([\s,\-\/\;]{1,10}|\b(?:и\s|или\s|через\s|и\sточка\s){1,4}\b)\s*([NSСЮЕWВЗ]?)\s*([\d]{1,3}[°\s']?[\d]{0,2}[.,\s']?[\d]{0,6}[′"\s]?[.,\s']?[\d]{0,6}[″"\s']?)([NSСЮЕWВЗ]?))"
        , std::regex::icase | std::regex::optimize
    );

    std::vector<Coordinate> found_coords;
    auto it = text.cbegin();
    std::smatch match;

    // Ищем все совпадения
    while (std::regex_search(it, text.cend(), match, GEO_PAIR_REGEX)) {
        size_t current_pos = std::distance(text.cbegin(), match[0].first);

        std::string lat_str = match[2].str();
        std::string lon_str = match[6].str();

        // Сбор направляющих символов для широты и долготы
        std::string dir_lat = match[1].str() + match[3].str(); 
        std::string dir_lon = match[5].str() + match[7].str();

        Coordinate coord;
        std::string format1, format2;

        // Попытка нормализации и валидации
        coord.lat_dd = normalize_and_validate_component(lat_str + dir_lat, true, format1);
        coord.lon_dd = normalize_and_validate_component(lon_str + dir_lon, false, format2);

        // Если оба компонента валидны, сохраняем результат
        if (coord.lat_dd != 999.0 && coord.lon_dd != 999.0) {
            coord.is_valid = true;
            coord.original_text = match.str();
            coord.format = (format1 == format2) ? format1 : "Mixed(" + format1 + "/" + format2 + ")";
            coord.sentence_context = find_sentence_context(text, current_pos, match.length());
            coord.label = find_label(text, current_pos);
            found_coords.push_back(coord);
        }

        // Перемещаем итератор для поиска следующего совпадения после текущего
        it = match[0].second;
    }

    // --- Классификация набора координат ---

    std::string coord_type;
    size_t count = found_coords.size();

    if (count <= 1) {
        coord_type = "Одиночные точки";
    } else { // count >= 2
        coord_type = "Линия";
        // Проверяем, является ли полигоном (первая и последняя совпадают, и точек >= 3)
        if (count >= 3 && coords_match(found_coords.front(), found_coords.back())) {
            coord_type = "Замкнутый полигон";
        }
    }

    // --- Формирование JSON ответа ---

    json response = {
        {"coordinate_type", coord_type},
        {"total_found", count},
        {"coordinates", json::array()}
    };

    for (const auto& coord : found_coords) {
        // Форматируем DD для вывода с 4 знаками после запятой
        std::ostringstream ss;
        ss << std::fixed << std::setprecision(4)
           << std::abs(coord.lat_dd) << (coord.lat_dd >= 0 ? "N" : "S") << " "
           << std::abs(coord.lon_dd) << (coord.lon_dd >= 0 ? "E" : "W");
        std::string normalized_dd = ss.str();

        response["coordinates"].push_back({
            {"original", coord.original_text},
            {"normalized_dd", normalized_dd},
            {"lat_dd", coord.lat_dd},
            {"lon_dd", coord.lon_dd},
            {"format", coord.format},
            {"is_valid", coord.is_valid},
            {"label", coord.label.empty() ? "Нет" : coord.label},
            {"sentence_context", coord.sentence_context}
        });
    }

    return response;
}

// --- 4. Main функция с CLI11 и Crow ---


int main(int argc, char* argv[]) {
    // --- CLI11 Setup ---
    CLI::App app{ "Geo Coordinate Analysis HTTP Service using Crow and CLI11" };

    std::string host = "127.0.0.1";
    int port = 8080;
    // Новый параметр для статического контента
    std::string static_path = "static";

    app.add_option("--host", host, "Хост для прослушивания (по умолчанию: 127.0.0.1)")
        ->type_name("HOST");
    app.add_option("--port", port, "Порт для прослушивания (по умолчанию: 8080)")
        ->type_name("PORT");
    app.add_option("--static-path", static_path, "Путь к каталогу статического контента (по умолчанию: static)")
        ->type_name("PATH");

    try {
        app.parse(argc, argv);
    }
    catch (const CLI::ParseError& e) {
        return app.exit(e);
    }

    try {
        // --- Crow Setup ---
        crow::SimpleApp crow_app;

        // 1. Роут для корневой страницы (/)
        // Явно отдаем index.html при запросе корня.
        CROW_ROUTE(crow_app, "/")
            ([&](const crow::request& req) {
            std::string index_file_path = static_path + "/index.html";
            std::ifstream file(index_file_path);

            if (file.is_open()) {
                std::stringstream buffer;
                buffer << file.rdbuf();
                crow::response res(200, buffer.str());
                res.set_header("Content-Type", "text/html; charset=utf-8");
                return res;
            }
            else {
                return crow::response(404, "{\"error\": \"Не найден файл index.html в статическом каталоге: " + static_path + "\"}");
            }
                });

        // 2. Роут для анализа текста (POST /analyze) - остается без изменений
        CROW_ROUTE(crow_app, "/analyze")
            .methods("POST"_method)
            ([&](const crow::request& req) {
            // Проверяем Content-Type
            if (req.get_header_value("Content-Type").find("application/json") == std::string::npos) {
                return crow::response(400, "{\"error\": \"Необходим Content-Type: application/json\"}");
            }

            // Парсим JSON тела запроса
            json req_json;
            try {
                req_json = json::parse(req.body);
            }
            catch (const json::parse_error& e) {
                return crow::response(400, "{\"error\": \"Неверный формат JSON: " + std::string(e.what()) + "\"}");
            }

            // Проверяем наличие поля "text"
            if (!req_json.contains("text") || !req_json["text"].is_string()) {
                return crow::response(400, "{\"error\": \"Требуется строковое поле 'text' в теле запроса.\"}");
            }

            std::string input_text = req_json["text"].get<std::string>();

            // Запускаем анализ
            json result_json = analyze_geo_text(input_text);

            // Возвращаем результат
            crow::response res(200, result_json.dump(4));
            res.set_header("Content-Type", "application/json; charset=utf-8");
            return res;
                });

        // Стартуем сервер
        std::cout << "Запуск Geo-аналитического HTTP-сервиса на " << host << ":" << port << "..." << std::endl;
        std::cout << "Статический контент раздается из каталога: " << static_path << std::endl;
        std::cout << "Веб-интерфейс доступен по адресу: http://" << host << ":" << port << std::endl;
        std::cout << "API /analyze ожидает POST-запросы с полем 'text'." << std::endl;
        std::cout << "Для остановки нажмите Ctrl+C." << std::endl;

        crow_app.bindaddr(host).port(port).multithreaded().run();
    } catch (const std::exception& e) {
        std::cerr << "Ошибка при запуске сервера: " << e.what() << std::endl;
        return 1;
	}

    return 0;
}
