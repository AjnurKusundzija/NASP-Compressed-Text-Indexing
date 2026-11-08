#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>

#include "benchmark/BenchmarkRunner.h"

int main(int argc, char** argv) {
    try {
        const size_t bytes = argc > 1 ? std::stoull(argv[1]) : 10'000'000;
        const std::string csv = argc > 2 ? argv[2] : "benchmark.csv";

        fmi::BenchmarkSettings settings;
        settings.repetitions = 5;
        settings.patternsPerLength = 10;
        settings.buildRepetitions = 3;
        const std::string text = fmi::BenchmarkRunner::generateLog(bytes, 1);
        const fmi::BenchmarkReport report = fmi::BenchmarkRunner(settings).run(text);
        fmi::BenchmarkRunner::writeCsv(report, csv);

        std::cout << "Benchmark text: " << text.size() << " bytes\n";
        std::cout << "CSV: " << csv << "\n\n";
        for (const fmi::BenchRow& row : report.rows) {
            std::cout << std::left << std::setw(24) << row.experiment << " "
                      << std::setw(28) << row.series << " median=" << row.median << " " << row.unit << "\n";
        }
    } catch (const std::exception& error) {
        std::cerr << "error: " << error.what() << "\n";
        return 1;
    }
}
