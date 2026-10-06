// Лабораторная работа: параллельное численное интегрирование, POSIX threads.
// Вариант 1: f(x) = (1+x)/(2+3x)^2, интервал [1;3].
// Метод: левые прямоугольники. Замеры по потокам 1..max_threads.
// Отдельная программа (не делит процесс с std::thread-версией), чтобы
// замеры POSIX не зависели от разогрева/остывания CPU от другой технологии.

#include <algorithm>
#include <chrono>
#include <cstring>
#include <numeric>
#include <pthread.h>
#include <vector>

#include "integration.hpp"
#include "report.hpp"

using Clock = std::chrono::steady_clock;

const int REPEATS = 5;

struct PosixArg {
    long long start, end;
    double a, h;
};

// Результат отдаётся через возвращаемое значение потока (забирается в pthread_join).
void* posix_worker(void* p) {
    PosixArg* arg = static_cast<PosixArg*>(p);
    return new double(partial_sum(arg->start, arg->end, arg->a, arg->h));
}

// Возвращает время выполнения (сек), результат интегрирования кладёт в result.
double run_posix(long long n, int threads, double a, double h, double& result) {
    auto chunks = split_range(n, threads);
    std::vector<PosixArg> args(threads);
    std::vector<pthread_t> tids(threads);
    std::vector<double> partial(threads, 0.0);
    for (int i = 0; i < threads; ++i) {
        args[i] = {chunks[i].first, chunks[i].second, a, h};
    }

    auto t0 = Clock::now();
    int created = 0;
    int create_err = 0;
    for (; created < threads; ++created) {
        create_err = pthread_create(&tids[created], nullptr, posix_worker, &args[created]);
        if (create_err != 0) break;
    }
    // Дожидаемся уже запущенных потоков, даже если создание одного из них не удалось.
    int join_err = 0;
    for (int i = 0; i < created; ++i) {
        void* ret = nullptr;
        int err = pthread_join(tids[i], &ret);
        if (err != 0) {
            join_err = err;
            continue;
        }
        double* value = static_cast<double*>(ret);
        partial[i] = *value;
        delete value;
    }
    auto t1 = Clock::now();

    if (create_err != 0) {
        throw std::runtime_error("pthread_create не удалось (поток " + std::to_string(created + 1) +
                                 " из " + std::to_string(threads) + "): " + std::strerror(create_err));
    }
    if (join_err != 0) {
        throw std::runtime_error(std::string("pthread_join не удалось: ") + std::strerror(join_err));
    }

    double total = std::accumulate(partial.begin(), partial.end(), 0.0);
    result = total * h;

    return std::chrono::duration<double>(t1 - t0).count();
}

// Медиана по REPEATS запускам — устойчивее одиночного выброса, чем среднее.
double avg_time_posix(long long n, int threads, double a, double h, double& result) {
    std::vector<double> times(REPEATS);
    for (int r = 0; r < REPEATS; ++r) {
        times[r] = run_posix(n, threads, a, h, result);
    }
    return median_of(times);
}

int run() {
    std::cout << "Лабораторная работа: параллельное интегрирование методом левых прямоугольников (POSIX threads)\n";
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
        avg_time_posix(n, 1, A, h, warmup_result);
        avg_time_posix(n, std::max(main_threads, max_threads), A, h, warmup_result);
    }

    std::cout << "\n=== Тестирование производительности (каждый замер усреднён по "
               << REPEATS << " запускам) ===\n";

    auto posix_rows = benchmark(avg_time_posix, n, max_threads, A, h);
    print_table("POSIX threads:", posix_rows);
    write_csv("results_posix.csv", posix_rows);

    std::cout << "\nCSV сохранён в results_posix.csv\n";

    std::cout << "\n=== Основной расчёт (сравнение многопоточности и однопоточности) ===\n";
    double result_main = print_main_comparison("POSIX threads", avg_time_posix, n, main_threads, A, h);

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
