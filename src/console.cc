#include "console.h"

#include <iostream>
#include <string>

void ConsoleTablePrinter::PrintStatusLine(const IStatusPrint& status)
{
    // ANSI color codes
    constexpr const char* COLOR_RESET  = "\033[0m";
    constexpr const char* COLOR_RED    = "\033[31m";
    constexpr const char* COLOR_YELLOW = "\033[33m";
    constexpr const char* COLOR_GREEN  = "\033[32m";
    constexpr const char* COLOR_BLUE   = "\033[34m";
    const char* color = COLOR_RESET;
    const char* level_str = "VINFO";
    switch (status.level)
    {
        case VINFO: color = COLOR_BLUE;   level_str = "VINFO"; break;
        case INFO:  color = COLOR_GREEN;  level_str = "INFO";  break;
        case WARN:  color = COLOR_YELLOW; level_str = "WARN";  break;
        case ERROR: color = COLOR_RED;    level_str = "ERROR"; break;
    }
    // Format: [<level>][<location>] <data> - where the level is also color coded
    std::cout << "[" << color << level_str << COLOR_RESET << "]"
              << "[" << status.header << "] "
              << status.data
              << std::endl;
}

size_t ConsoleTablePrinter::newStatus(const IStatusPrint& status){
    // 1. Move cursor to end of the status rows
    CursorMove(status_start_+status_rows_.size());
    // 2. update and print status on bottom of status rows
    status_rows_.push_back(status);
    PrintStatusLine(status);
    // 3. update and redraw the table
    table_start_++;
    printTelemTable(true);
    return (size_t)status_rows_.size()-1;
}

size_t ConsoleTablePrinter::updateStatus(size_t index, const IStatusPrint& status){
    // 1. Move cursor to status row commanded from index
    CursorMove(status_start_+index);
    // 2. update and print new status into that status row
    PrintStatusLine(status);
    status_rows_[index] = status;
    return index;
}

bool ConsoleTablePrinter::addTelemetry(const ITelemetryPrint telem){
    table_rows_.push_back(telem);
    return table_rows_.size() < 2;
}

void ConsoleTablePrinter::printTelemTable(bool full_redraw)
{
    if (table_rows_.empty())
        return;

    // Latest telemetry for header layout
    const ITelemetryPrint& telem = table_rows_.back();
    std::string h_fmt = BuildFormat(telem.columns);
    auto header_args = BuildArgs(telem.columns);
    std::string header_line = fmt::vformat(h_fmt, header_args);
    size_t total_width = telem.columns.size() * column_width_;

    if (full_redraw)
    {
        // Print header + separators
        PrintRow(std::string(total_width, '='));
        PrintRow(header_line);
        PrintRow(std::string(total_width, '='));
    }
    else
    {
        // Move cursor to the start of the data area
        MoveCursorUp(static_cast<int>(current_displayed_table_rows_));
    }

    // Print newest rows (circular buffer ensures we only have max rows)
    for (const auto& row : table_rows_)  // oldest → newest
    {
        fmt::dynamic_format_arg_store<fmt::format_context> store;
        store.push_back(column_width_);  // {0} = column width

        for (const auto& cell : row.data)
            store.push_back(cell);

        std::string line = fmt::vformat(h_fmt, store);
        PrintRow(line);
    }

    current_displayed_table_rows_ = table_rows_.size();
}