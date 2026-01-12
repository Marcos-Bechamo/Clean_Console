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
    void PrintStaticLine(const std::string& text)
    {
        std::cout << text << std::endl;
        ++static_lines_;
    }

    void PrintHeader(const std::vector<Column>& columns)
    {
        columns_ = columns;
        format_ = BuildFormat(columns);
        header_args_ = BuildArgs(columns);

        // NEW: separator between static lines and header
        size_t total_width = 2 + columns_.size() * column_width_;
        PrintLine(std::string(total_width, '='), false);

        // Print header line
        std::string header_line = fmt::vformat(format_, header_args_);
        PrintLine(header_line, false);

        // Print separator line between header and data
        PrintLine(std::string(total_width, '='), false);

        header_printed_ = true;
        current_displayed_rows_ = 0;
    }


    // Add a new telemetry row (scrolling)
    void PrintRow(const std::vector<std::string>& row)
    {
        if (row.empty() || !header_printed_)
            return;

        // Insert new row at the top
        rows_.insert(rows_.begin(), row);

        // Keep only max_rows_ rows
        if (rows_.size() > max_rows_)
            rows_.pop_back();

        // Move cursor up ONLY over existing data rows
        if (current_displayed_rows_ > 0)
            MoveCursorUp(static_cast<int>(current_displayed_rows_));

        // Reprint data rows
        for (const auto& r : rows_)
        {
            fmt::dynamic_format_arg_store<fmt::format_context> store;
            store.push_back(column_width_);  // {0} = width

            for (const auto& cell : r)
                store.push_back(cell);

            std::string line = fmt::vformat(format_, store);
            PrintLine(line, true);
        }

        current_displayed_rows_ = rows_.size();
    }

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

int main()
{
    auto logger = spdlog::stdout_color_mt("console");
    logger->set_pattern("%v");

    ConsoleTablePrinter printer(logger, 12, 12);

    // -----------------------------
    // Static lines (printed once)
    // -----------------------------
    printer.PrintStaticLine("Aircraft: TREY_");
    printer.PrintStaticLine("Mode: HIL");
    printer.PrintStaticLine("Status: OK");

    std::vector<Column> header = {
        {"Time(s)", ColumnAlign::Center},
        {"Agl(m)", ColumnAlign::Left},
        {"Ias(m/s)", ColumnAlign::Left},
        {"Ground(m/s)", ColumnAlign::Left},
        {"FR_rpm", ColumnAlign::Left}
    };

    printer.PrintHeader(header);

    for (int i = 0; i < 20; ++i)
    {
        printer.PrintRow({
            fmt::format("{:.1f}", 12.0 + i * 0.1),
            fmt::format("{}", 500 + i * 5),
            fmt::format("{}", 25 + i),
            fmt::format("{}", 30 + i),
            fmt::format("{}", 1200 + i * 10)
        });
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
}
