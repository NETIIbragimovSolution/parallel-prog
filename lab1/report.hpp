// Вывод результатов тестирования: таблица в консоль, CSV в файл,
// и сам прогон технологии (POSIX/std::thread) по числу потоков 1..max_threads.
#pragma once

#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

struct Row {
    int threads;
    double time_sec;
    double result;
    double speedup;
    double efficiency_percent;
};

void print_table(const std::string& title, const std::vector<Row>& rows);
void write_csv(const std::string& filename, const std::vector<Row>& rows);

// Прогоняет технологию (POSIX или std::thread) для потоков 1..max_threads
// и заполняет таблицу ускорения/эффективности относительно 1 потока.
// Шаблон — RunFn должна иметь сигнатуру double(long long, int, double, double, double&).
template <typename RunFn>
std::vector<Row> benchmark(RunFn run, long long n, int max_threads, double a, double h) {
    std::vector<Row> rows;
    double base_time = 0.0;
    for (int t = 1; t <= max_threads; ++t) {
        double result = 0.0;
        double time_sec = run(n, t, a, h, result);
        if (t == 1) base_time = time_sec;
        double speedup = base_time / time_sec;
        double efficiency = speedup / t * 100.0;
        rows.push_back({t, time_sec, result, speedup, efficiency});
    }
    return rows;
}

// Печатает "основной расчёт" (п.4 задания): сравнение однопоточного и
// main_threads-поточного запуска, фактическое ускорение против идеального
// (линейного, = main_threads) и эффективность относительно идеала.
// Возвращает результат интегрирования на main_threads потоках (для проверки точности).
template <typename RunFn>
double print_main_comparison(const std::string& label, RunFn run, long long n, int main_threads,
                              double a, double h) {
    double result_1 = 0.0, result_main = 0.0;
    double time_1 = run(n, 1, a, h, result_1);
    double time_main = run(n, main_threads, a, h, result_main);
    double speedup = time_1 / time_main;
    double ideal_speedup = static_cast<double>(main_threads);
    double efficiency_vs_ideal = speedup / ideal_speedup * 100.0;

    std::cout << std::fixed << std::setprecision(6);
    std::cout << "\n" << label << ":\n";
    std::cout << "  1 поток:            время = " << time_1 << " с, результат = " << result_1 << "\n";
    std::cout << "  " << main_threads << " поток(ов): время = " << time_main
               << " с, результат = " << result_main << "\n";
    std::cout << "  Фактическое ускорение:      " << speedup << "\n";
    std::cout << "  Идеальное ускорение (= N):  " << ideal_speedup << "\n";
    std::cout << "  Эффективность отн. идеала:  " << efficiency_vs_ideal << " %\n";

    return result_main;
}
