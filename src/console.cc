#include "console.h"

void ConsoleTablePrinter::PrintStaticLine(const std::string& text)
{
    std::cout << text << std::endl;
    ++static_lines_;
}

void ConsoleTablePrinter::PrintHeader(const std::vector<Column>& columns)
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
void ConsoleTablePrinter::PrintRow(const std::vector<std::string>& row)
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