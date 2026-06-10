#pragma once

#include <cctype>
#include <exception>
#include <format>
#include <fstream>
#include <print>
#include <ranges>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace csv {
class CSV {
  public:
    std::vector<std::string> fields;
    std::unordered_map<std::string, std::vector<double>> data;

    bool load(const CSV &csv) {
        data = csv.data;
        return true;
    };

    bool load(const std::string &path, bool index_col = false) {
        std::ifstream file(path);
        if (!file.is_open())
            throw std::runtime_error("File cannot be opened!");

        auto trim = [](auto &&substr) {
            return substr |
                   std::views::drop_while([](unsigned char c) { return std::isspace(c); }) |
                   std::views::reverse |
                   std::views::drop_while([](unsigned char c) { return std::isspace(c); }) |
                   std::views::reverse | std::ranges::to<std::string>();
        };

        std::string line;
        std::getline(file, line);

        fields = line | std::views::split(',') | std::views::transform(trim) |
                 std::ranges::to<std::vector<std::string>>();

        while (!fields.empty() && fields.back().empty())
            fields.pop_back();

        if (index_col)
            fields.erase(fields.begin());

        for (const auto &field : fields)
            data[field] = {};

        while (std::getline(file, line)) {
            auto tokens = line | std::views::split(',') | std::views::transform(trim) |
                          std::views::filter([](const std::string &s) { return !s.empty(); }) |
                          std::ranges::to<std::vector<std::string>>();

            for (const auto &[index, token] : tokens | std::views::enumerate) {
                if (index_col && index == 0)
                    continue;

                size_t field_index =
                    index_col ? static_cast<size_t>(index) - 1 : static_cast<size_t>(index);

                if (field_index >= fields.size())
                    break;

                try {
                    data.at(fields.at(field_index)).push_back(std::stod(token));
                } catch (std::exception &e) {
                    std::println("{}", e.what());
                    file.close();
                    return false;
                }
            }
        }

        file.close();
        return true;
    }

    bool save(const std::string &path) {
        std::ofstream file(path);
        if (file.is_open()) {
            size_t max_rows = 0;
            std::string line = "";
            for (const auto &key : fields) {
                const auto &vec = data.at(key);
                max_rows = std::max(max_rows, vec.size());
                line += key;
                line += ",";
            }
            line += "\n";
            file << line;
            for (size_t row = 0; row < max_rows; ++row) {
                line = "";
                for (size_t col = 0; col < fields.size(); ++col) {
                    const auto &vec = data.at(fields.at(col));
                    std::string value = (row < vec.size()) ? std::to_string(vec.at(row)) : "";
                    line += value;
                    line += ",";
                }
                line += "\n";
                file << line;
            }
            file.close();
            return true;
        } else {
            throw std::runtime_error("File cannot be opened!");
            return false;
        }
    }

    std::vector<double>& operator[](const std::string &field) {
        return data[field];
    }

    std::vector<double>& at(const std::string &field) { return data.at(field); }

    auto begin() const { return data.begin(); }
    auto end() const { return data.end(); }
    auto empty() const { return data.empty(); }
};
} // namespace csv

// Formatting and Printing
namespace csv {
inline std::ostream &operator<<(std::ostream &os, const CSV &csv_) {
    const auto &keys = csv_.fields;
    if (keys.empty())
        return os;

    std::vector<size_t> col_widths;
    col_widths.reserve(keys.size());
    size_t max_rows = 0;

    for (const auto &key : keys) {
        const auto &vec = csv_.data.at(key);
        max_rows = std::max(max_rows, vec.size());

        size_t width = key.size();
        for (const double v : vec)
            width = std::max(width, std::to_string(v).size());

        col_widths.push_back(width + 2);
    }

    for (size_t col = 0; col < keys.size(); ++col) {
        std::string key = keys.at(col);
        int w = static_cast<int>(col_widths.at(col));
        os << std::vformat("|{:^{}}", std::make_format_args(key, w));
    }
    os << "|\n";

    size_t total_w = col_widths.size();
    for (size_t w : col_widths)
        total_w += w;
    os << "|" << std::string(total_w - 1, '-') << "|\n";

    for (size_t row = 0; row < max_rows; ++row) {
        for (size_t col = 0; col < keys.size(); ++col) {
            const auto &vec = csv_.data.at(keys.at(col));
            std::string value = (row < vec.size()) ? std::to_string(vec.at(row)) : "--";
            int w = static_cast<int>(col_widths.at(col));
            os << std::vformat("|{:^{}}", std::make_format_args(value, w));
        }
        os << "|\n";
    }

    return os;
}
} // namespace csv
namespace std {
template <> struct formatter<csv::CSV> {
    constexpr auto parse(std::format_parse_context &ctx) { return ctx.begin(); }
    auto format(const csv::CSV &csv_, auto &ctx) const {
        auto out = ctx.out();

        const auto &keys = csv_.fields;
        if (keys.empty())
            return out;

        std::vector<size_t> col_widths;
        col_widths.reserve(keys.size());
        size_t max_rows = 0;

        for (const auto &key : keys) {
            const auto &vec = csv_.data.at(key);
            max_rows = std::max(max_rows, vec.size());

            size_t width = key.size();
            for (const double v : vec)
                width = std::max(width, std::to_string(v).size());

            col_widths.push_back(width + 2);
        }
        size_t w = 0;
        for (size_t col = 0; col < keys.size(); ++col) {
            out = std::format_to(out, "|{:^{}}", keys[col], col_widths[col]);
            w += col_widths[col];
        }
        out = std::format_to(out, "|\n");

        w += csv_.fields.size() - 1;
        std::string sep = "|";
        sep.append(w, '-');
        sep += "|\n";
        out = std::format_to(out, "{}", sep);

        for (size_t row = 0; row < max_rows; ++row) {
            for (size_t col = 0; col < keys.size(); ++col) {
                const auto &vec = csv_.data.at(keys[col]);
                std::string value = (row < vec.size()) ? std::to_string(vec[row]) : "--";
                out = std::format_to(out, "|{:^{}}", value, col_widths[col]);
            }
            out = std::format_to(out, "|\n");
        }
        return out;
    }
};
} // namespace std