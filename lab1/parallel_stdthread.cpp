// Лабораторная работа: параллельное численное интегрирование, std::thread.
// Вариант 1: f(x) = (1+x)/(2+3x)^2, интервал [1;3].
// Метод: левые прямоугольники. Замеры по потокам 1..max_threads.
// Отдельная программа (не делит процесс с POSIX-версией), чтобы замеры
// std::thread не зависели от разогрева/остывания CPU от другой технологии.

#include <algorithm>
#include <chrono>
#include <numeric>
#include <thread>
#include <vector>

#include "integration.hpp"
#include "report.hpp"

using Clock = std::chrono::steady_clock;

const int REPEATS = 5;

// Параметры передаются потоку по отдельности, без объединения в структуру.
void thread_worker(long long start, long long end, double a, double h, double& out) {
    out = partial_sum(start, end, a, h);
}

double run_stdthread(long long n, int threads, double a, double h, double& result) {
    auto chunks = split_range(n, threads);
    std::vector<double> partial(threads, 0.0);
    std::vector<std::thread> workers;
    workers.reserve(threads);

    auto t0 = Clock::now();
    try {
        for (int i = 0; i < threads; ++i) {
            workers.emplace_back(thread_worker, chunks[i].first, chunks[i].second, a, h,
                                 std::ref(partial[i]));
        }
    } catch (...) {
        // Не удалось создать поток: дожидаемся уже запущенных, иначе std::terminate.
        for (auto& w : workers) w.join();
        throw;
    }
    for (auto& w : workers) w.join();
    auto t1 = Clock::now();

    double total = std::accumulate(partial.begin(), partial.end(), 0.0);
    result = total * h;

    return std::chrono::duration<double>(t1 - t0).count();
}

// Медиана по REPEATS запускам — устойчивее одиночного выброса, чем среднее.
double avg_time_stdthread(long long n, int threads, double a, double h, double& result) {
    std::vector<double> times(REPEATS);
    for (int r = 0; r < REPEATS; ++r) {
        times[r] = run_stdthread(n, threads, a, h, result);
    }
    return median_of(times);
}

int run() {
    std::cout << "Лабораторная работа: параллельное интегрирование методом левых прямоугольников (std::thread)\n";
    std::cout << "f(x) = (1+x)/(2+3x)^2, интервал [" << A << "; " << B << "]\n\n";

    long long n = read_positive<long long>("Введите количество разбиений n: ");
    int main_threads = read_positive<int>("Введите количество потоков для основного вычисления: ");
    int max_threads = read_positive<int>("Введите максимальное количество потоков для тестирования: ");

    double h = (B - A) / static_cast<double>(n);

    // Прогрев: сначала на 1 потоке, потом на максимальном числе потоков.
    // Без этого первый настоящий замер (1 поток) может случайно попасть на
    // медленное E-ядро (гибридная архитектура P+E) или на ещё не разогнанную
    // после старта процесса частоту CPU — и все ускорения относительно него
    // потом выглядят завышенными (эффективность растёт вместо падения).
    std::cout << "\nПрогрев CPU...\n";
    {
        double warmup_result = 0.0;
        avg_time_stdthread(n, 1, A, h, warmup_result);
        avg_time_stdthread(n, std::max(main_threads, max_threads), A, h, warmup_result);
    }

    std::cout << "\n=== Тестирование производительности (каждый замер усреднён по "
               << REPEATS << " запускам) ===\n";

    auto stdthread_rows = benchmark(avg_time_stdthread, n, max_threads, A, h);
    print_table("std::thread:", stdthread_rows);
    write_csv("results_stdthread.csv", stdthread_rows);

    std::cout << "\nCSV сохранён в results_stdthread.csv\n";

    std::cout << "\n=== Основной расчёт (сравнение многопоточности и однопоточности) ===\n";
    double result_main = print_main_comparison("std::thread", avg_time_stdthread, n, main_threads, A, h);

    print_accuracy(n, result_main);

    return 0;
}

int main() {
    try {
        return run();
    } catch (const std::exception& e) {
        std::cerr << "\nОшибка: " << e.what() << "\n";
        return 1;
    }
}
