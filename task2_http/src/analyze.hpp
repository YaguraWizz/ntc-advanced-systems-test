#pragma once
#include "types.hpp"
#include <regex>
#include <functional>

namespace analyze {

    using types::Coordinate;
    using types::CoordinateMetadata;
    using types::CoordinateSet;

    // Контекст для парсера, содержащий всю необходимую информацию
    struct MatchContext {
        size_t abs_pos;
        size_t length;
        size_t sentence_index;
        std::string_view raw_match_sv;
    };

    // Парсер: функтор, который принимает smatch и контекст,
    // и возвращает пару координат и метаданных
    using ParserFn = std::function<std::optional<std::pair<Coordinate, CoordinateMetadata>>(const std::smatch&, const MatchContext&)>;

    class CoordinateAnalyzer {
    public:
        // Метод для регистрации новой стратегии парсинга
        void register_pattern(const std::string& pattern, ParserFn parser, int priority = 0);

        // Главный метод анализа текста
        CoordinateSet analyze(const std::string& text) const;
    private:
        struct Pattern {
            std::regex rx;
            ParserFn parser;
            int priority;
        };
        std::vector<Pattern> patterns_;
    };
}