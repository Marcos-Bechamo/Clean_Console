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

#include "src/console.h"

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
