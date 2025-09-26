#include "analyze.hpp"
#include "utils.hpp"
#include <map>





namespace analyze {

    using types::CoordFormat;
    using types::CoordSetType;

    // Метод для регистрации новой стратегии парсинга
    void CoordinateAnalyzer::register_pattern(const std::string& pattern, ParserFn parser, int priority) {
        patterns_.push_back({ std::regex(pattern, std::regex::icase | std::regex::optimize), parser, priority });
        // Сортируем по приоритету
        std::sort(patterns_.begin(), patterns_.end(), [](const Pattern& a, const Pattern& b) {
            return a.priority > b.priority;
        });
    }

    // Главный метод анализа текста
    CoordinateSet CoordinateAnalyzer::analyze(const std::string& text) const {
        CoordinateSet result;
        std::string normalized_text = utils::normalize_text_for_parsing(text);
        auto sentences = utils::split_sentences(text);

        std::vector<std::pair<Coordinate, CoordinateMetadata>> found_coords;
        std::map<size_t, size_t> matched_positions;

        for (const auto& pat : patterns_) {
            std::smatch m;
            std::string::const_iterator searchStart(text.cbegin());
            while (std::regex_search(searchStart, text.cend(), m, pat.rx)) {
                size_t abs_pos = (size_t)(m.position(0) + (searchStart - text.cbegin()));
                size_t length = m.length(0);

                bool is_overlap = false;
                for (const auto& existing_pos : matched_positions) {
                    if ((abs_pos >= existing_pos.first && abs_pos < existing_pos.first + existing_pos.second) ||
                        (abs_pos + length > existing_pos.first && abs_pos + length <= existing_pos.first + existing_pos.second)) {
                        is_overlap = true;
                        break;
                    }
                }
                if (is_overlap) {
                    searchStart = m.suffix().first;
                    continue;
                }

                size_t sentence_idx = 0;
                for (size_t i = 0; i < sentences.size(); ++i) {
                    if (abs_pos >= sentences[i].first && abs_pos < sentences[i].first + sentences[i].second) {
                        sentence_idx = i;
                        break;
                    }
                }

                MatchContext ctx{ abs_pos, length, sentence_idx, std::string_view(text).substr(abs_pos, length) };

                if (auto result_pair = pat.parser(m, ctx)) {
                    auto& [coord, metadata] = *result_pair;

                    auto& [ss, se] = sentences[sentence_idx];
                    metadata.sentence_snippet = utils::make_snippet(text, abs_pos, length, ss, se);

                    found_coords.push_back(*result_pair);
                    matched_positions[abs_pos] = length;
                }

                searchStart = m.suffix().first;
            }
        }

        std::sort(found_coords.begin(), found_coords.end(), [](const auto& a, const auto& b) {
            return a.second.raw_match.compare(b.second.raw_match) < 0;
            });

        result.coords = found_coords;
        return result;
    }

}