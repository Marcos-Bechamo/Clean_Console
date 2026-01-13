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
#include "src/console_base.h"
#include "src/thread_manager.h"

int main()
{
    auto logger = spdlog::stdout_color_mt("console");
    logger->set_pattern("%v");

    ConsoleTablePrinter printer(logger, 12, 5);

    auto tm = ThreadManager();

    // -----------------------------
    // Static lines (printed once)
    // -----------------------------
    printer.print_banner("v1.2.3", "1/13/2026 @ 10:42");
    tm.PostStatus(printer, {ConsoleLevels::INFO, "Console", "Starting Application"});
    tm.PostStatus(printer, {ConsoleLevels::WARN, "Sixdof", "Starting simulation"});
    tm.PostStatus(printer, {ConsoleLevels::ERROR, "Controller", "not running simulation"});

    std::vector<Column> header = {
        {"Time(s)", ColumnAlign::Center},
        {"Agl(m)", ColumnAlign::Left},
        {"Ias(m/s)", ColumnAlign::Left}
    };

    for (double i=0; i < 6; ++i){
        auto data = printer.convert_data({i,i,i});
        tm.PostTelem(printer, {header, data});
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    tm.PostStatus(printer, {ConsoleLevels::INFO, "Console", "running application"});
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
}
