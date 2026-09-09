// Вывод результатов тестирования: таблица в консоль, CSV в файл,
// и сам прогон технологии (POSIX/std::thread) по числу потоков 1..max_threads.
#pragma once

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
