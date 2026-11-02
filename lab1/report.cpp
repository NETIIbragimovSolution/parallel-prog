#include "report.hpp"

#include <fstream>
#include <iomanip>
#include <iostream>

void print_table(const std::string& title, const std::vector<Row>& rows) {
    std::cout << "\n" << title << "\n";
    std::cout << "Потоки    Время (с)     Результат       Ускорение   Эффект-ть (%)\n";
    std::cout << std::fixed << std::setprecision(6);
    for (const auto& r : rows) {
        std::cout << std::left << std::setw(10) << r.threads
                   << std::setw(14) << r.time_sec
                   << std::setw(16) << r.result
                   << std::setw(12) << r.speedup
                   << std::setw(16) << r.efficiency_percent << "\n";
    }
}

void write_csv(const std::string& filename, const std::vector<Row>& rows) {
    std::ofstream out(filename);
    out << "threads,time_sec,result,speedup,efficiency_percent\n";
    out << std::fixed << std::setprecision(9);
    for (const auto& r : rows) {
        out << r.threads << "," << r.time_sec << "," << r.result << ","
            << r.speedup << "," << r.efficiency_percent << "\n";
    }
}
