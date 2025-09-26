#pragma once	
#include <vector>
#include <string>
#include <algorithm>
#include <cmath>
#include <sstream>
#include <iostream>

namespace utils {

    static inline std::string normalize_text_for_parsing(const std::string& src) {
        std::string s = src;
        // �������� ��� �������, ���� ��� �� �������� ������������ ������
        for (size_t i = 0; i < s.length(); ++i) {
            if (s[i] == ',') {
                s[i] = '.';
            }
        }
        return s;
    }

    // �������������� DMS (�������, ������, �������) � ���������� ������
    static inline double dms_to_decimal(double deg, double min, double sec, char hemi) {
        double sign = 1.0;
        if (hemi == 'S' || hemi == 'W' || hemi == 's' || hemi == 'w') {
            sign = -1.0;
        }

        double decimal = deg + (min / 60.0) + (sec / 3600.0);
        return decimal * sign;
    }

    // �������������� ������ � double � ���������
    static inline double stod_norm(const std::string& s, bool& ok) {
        try {
            std::string temp = s;
            // �������� ������� �� ����� ��� �������������� ��������
            std::replace(temp.begin(), temp.end(), ',', '.');
            double val = std::stod(temp);
            ok = true;
            return val;
        }
        catch (...) {
            ok = false;
            return 0.0;
        }
    }

    // ��������, �������� �� ������ ������������ �����������
    static inline bool is_sentence_terminator(char c) {
        return c == '.' || c == '?' || c == '!' || c == '\n';
    }

    // ��������� ������ �� ����������� (���������� {start_pos, length})
    static inline std::vector<std::pair<size_t, size_t>> split_sentences(const std::string& text) {
        std::vector<std::pair<size_t, size_t>> sentences;
        size_t start = 0;
        for (size_t i = 0; i < text.length(); ++i) {
            if (is_sentence_terminator(text[i]) || i == text.length() - 1) {
                if (i > start) {
                    // +1 ��� ������� ���������� �������, ���� ��� �� ����������
                    sentences.push_back({ start, i - start + (i == text.length() - 1 ? 1 : 0) });
                }
                start = i + 1;
            }
        }
        return sentences;
    }

    // �������� �������� �����������
    static inline std::string make_snippet(const std::string& text, size_t match_pos, size_t match_len, size_t sentence_start, size_t sentence_len) {
        size_t snippet_start = std::max(sentence_start, match_pos > 50 ? match_pos - 50 : 0);
        size_t snippet_end = std::min(text.length(), sentence_start + sentence_len);

        // Ensure snippet_end is greater than snippet_start
        if (snippet_end <= snippet_start) {
            snippet_end = match_pos + match_len;
            snippet_start = match_pos;
        }

        return text.substr(snippet_start, snippet_end - snippet_start);
    }
}
