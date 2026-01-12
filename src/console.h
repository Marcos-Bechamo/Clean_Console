#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#include <fmt/format.h>
#include <fmt/args.h>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <chrono>
#include <vector>

enum class ColumnAlign {
    Left,
    Center
};

struct Column {
    std::string title;
    ColumnAlign align;
};

class ConsoleTablePrinter {
public:
    ConsoleTablePrinter(std::shared_ptr<spdlog::logger> logger,
                        int width,
                        size_t max_rows = 12)
        : logger_(std::move(logger)),
          column_width_(width),
          max_rows_(max_rows),
          header_printed_(false),
          current_displayed_rows_(0),
          static_lines_(0) {}

    // -----------------------------
    // NEW: Print static line above table
    // -----------------------------
    void PrintStaticLine(const std::string& text);

    void PrintHeader(const std::vector<Column>& columns);

    // Add a new telemetry row (scrolling)
    void PrintRow(const std::vector<std::string>& row);

private:
    void PrintLine(const std::string& line, bool is_data_row)
    {
        std::cout << "\033[K"; // clear line
        if (is_data_row)
            std::cout << "[" << line << "]" << std::endl;
        else
            std::cout << line << std::endl;
    }

    void MoveCursorUp(int n)
    {
        if (n > 0)
            std::cout << "\033[" << n << "A";
    }

    std::string BuildFormat(const std::vector<Column>& columns) const
    {
        std::string fmt_str;
        for (size_t i = 0; i < columns.size(); ++i)
        {
            size_t index = i + 1; // {0} = width
            if (columns[i].align == ColumnAlign::Center)
                fmt_str += fmt::format("{{{}:^{{0}}}}", index);
            else
                fmt_str += fmt::format("{{{}:<{{0}}}}", index);
        }
        return fmt_str;
    }

    fmt::dynamic_format_arg_store<fmt::format_context>
    BuildArgs(const std::vector<Column>& columns) const
    {
        fmt::dynamic_format_arg_store<fmt::format_context> store;
        store.push_back(column_width_);
        for (const auto& c : columns)
            store.push_back(c.title);
        return store;
    }

private:
    std::shared_ptr<spdlog::logger> logger_;
    int column_width_;
    size_t max_rows_;

    bool header_printed_;
    size_t current_displayed_rows_;
    size_t static_lines_; // NEW: count of static lines (not redrawn)

    std::string format_;
    fmt::dynamic_format_arg_store<fmt::format_context> header_args_;
    std::vector<std::vector<std::string>> rows_;
    std::vector<Column> columns_;
};