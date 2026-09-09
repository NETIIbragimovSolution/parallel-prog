// Лабораторная работа: параллельное численное интегрирование.
// Вариант 1: f(x) = (1+x)/(2+3x)^2, интервал [1;3].
// Метод: левые прямоугольники.

#include <chrono>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <pthread.h>
#include <thread>
#include <vector>

// Функция по варианту и её первообразная

const double A = 1.0; // левая граница интервала
const double B = 3.0; // правая граница интервала

double f(double x) {
    double denom = 2.0 + 3.0 * x;
    return (1.0 + x) / (denom * denom);
}

// Первообразная F(x) = (1/9)(ln(2+3x) - 1/(2+3x)), используется только
// чтобы вывести точное значение интеграла и сравнить с ним погрешность.
double antiderivative(double x) {
    double u = 2.0 + 3.0 * x;
    return (std::log(u) - 1.0 / u) / 9.0;
}

double exact_integral() {
    return antiderivative(B) - antiderivative(A);
}

// Метод левых прямоугольников 

double partial_sum(long long start, long long end, double a, double h) {
    double sum = 0.0;
    for (long long i = start; i < end; ++i) {
        sum += f(a + i * h);
    }
    return sum;
}

// Делит n индексов на threads по возможности равных непрерывных блоков.
std::vector<std::pair<long long, long long>> split_range(long long n, int threads) {
    std::vector<std::pair<long long, long long>> chunks(threads);
    long long base = n / threads;
    long long remainder = n % threads;
    long long start = 0;
    for (int i = 0; i < threads; ++i) {
        long long len = base + (i < remainder ? 1 : 0);
        chunks[i] = {start, start + len};
        start += len;
    }
    return chunks;
}

using Clock = std::chrono::steady_clock;

// POSIX 

struct PosixArg {
    long long start, end;
    double a, h;
    double result;
};

void* posix_worker(void* p) {
    PosixArg* arg = static_cast<PosixArg*>(p);
    arg->result = partial_sum(arg->start, arg->end, arg->a, arg->h);
    return nullptr;
}

// Возвращает время выполнения (сек), результат интегрирования кладёт в result.
double run_posix(long long n, int threads, double a, double h, double& result) {
    auto chunks = split_range(n, threads);
    std::vector<PosixArg> args(threads);
    std::vector<pthread_t> tids(threads);
    for (int i = 0; i < threads; ++i) {
        args[i] = {chunks[i].first, chunks[i].second, a, h, 0.0};
    }

    auto t0 = Clock::now();
    for (int i = 0; i < threads; ++i) {
        pthread_create(&tids[i], nullptr, posix_worker, &args[i]);
    }
    for (int i = 0; i < threads; ++i) {
        pthread_join(tids[i], nullptr);
    }
    auto t1 = Clock::now();

    double total = 0.0;
    for (auto& arg : args) total += arg.result;
    result = total * h;

    return std::chrono::duration<double>(t1 - t0).count();
}

// std::thread 

double run_stdthread(long long n, int threads, double a, double h, double& result) {
    auto chunks = split_range(n, threads);
    std::vector<double> partial(threads, 0.0);
    std::vector<std::thread> workers;

    auto t0 = Clock::now();
    for (int i = 0; i < threads; ++i) {
        workers.emplace_back([&, i]() {
            partial[i] = partial_sum(chunks[i].first, chunks[i].second, a, h);
        });
    }
    for (auto& w : workers) w.join();
    auto t1 = Clock::now();

    double total = std::accumulate(partial.begin(), partial.end(), 0.0);
    result = total * h;

    return std::chrono::duration<double>(t1 - t0).count();
}

// Замер со усреднением по нескольким запускам 

const int REPEATS = 3;

double avg_time_posix(long long n, int threads, double a, double h, double& result) {
    double total_time = 0.0;
    for (int r = 0; r < REPEATS; ++r) {
        total_time += run_posix(n, threads, a, h, result);
    }
    return total_time / REPEATS;
}

double avg_time_stdthread(long long n, int threads, double a, double h, double& result) {
    double total_time = 0.0;
    for (int r = 0; r < REPEATS; ++r) {
        total_time += run_stdthread(n, threads, a, h, result);
    }
    return total_time / REPEATS;
}

// Таблица результатов тестирования (для CSV и консоли) 

struct Row {
    int threads;
    double time_sec;
    double result;
    double speedup;
    double efficiency_percent;
};

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

// Прогоняет технологию (POSIX или std::thread) для потоков 1..max_threads
// и заполняет таблицу ускорения/эффективности относительно 1 потока.
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

int main() {
    std::cout << "Лабораторная работа: параллельное интегрирование методом левых прямоугольников\n";
    std::cout << "f(x) = (1+x)/(2+3x)^2, интервал [" << A << "; " << B << "]\n\n";

    long long n;
    int main_threads, max_threads;

    std::cout << "Введите количество разбиений n: ";
    std::cin >> n;
    std::cout << "Введите количество потоков для основного вычисления: ";
    std::cin >> main_threads;
    std::cout << "Введите максимальное количество потоков для тестирования: ";
    std::cin >> max_threads;

    double h = (B - A) / static_cast<double>(n);

    std::cout << "\n=== Тестирование производительности (каждый замер усреднён по "
               << REPEATS << " запускам) ===\n";

    auto posix_rows = benchmark(run_posix, n, max_threads, A, h);
    print_table("POSIX threads:", posix_rows);
    write_csv("results_posix.csv", posix_rows);

    auto stdthread_rows = benchmark(run_stdthread, n, max_threads, A, h);
    print_table("std::thread:", stdthread_rows);
    write_csv("results_stdthread.csv", stdthread_rows);

    std::cout << "\nCSV сохранены в results_posix.csv и results_stdthread.csv\n";

    std::cout << "\n=== Основной расчёт (сравнение многопоточности и однопоточности) ===\n";

    double result_posix_1, result_posix_main, result_thread_1, result_thread_main;
    double time_posix_1 = run_posix(n, 1, A, h, result_posix_1);
    double time_posix_main = run_posix(n, main_threads, A, h, result_posix_main);
    double time_thread_1 = run_stdthread(n, 1, A, h, result_thread_1);
    double time_thread_main = run_stdthread(n, main_threads, A, h, result_thread_main);

    std::cout << std::fixed << std::setprecision(6);
    std::cout << "\nPOSIX threads:\n";
    std::cout << "  1 поток:            время = " << time_posix_1 << " с, результат = " << result_posix_1 << "\n";
    std::cout << "  " << main_threads << " поток(ов): время = " << time_posix_main
               << " с, результат = " << result_posix_main
               << ", ускорение = " << (time_posix_1 / time_posix_main) << "\n";

    std::cout << "\nstd::thread:\n";
    std::cout << "  1 поток:            время = " << time_thread_1 << " с, результат = " << result_thread_1 << "\n";
    std::cout << "  " << main_threads << " поток(ов): время = " << time_thread_main
               << " с, результат = " << result_thread_main
               << ", ускорение = " << (time_thread_1 / time_thread_main) << "\n";

    double exact = exact_integral();
    double error_main = std::fabs(result_posix_main - exact);
    std::cout << "\n=== Точность ===\n";
    std::cout << "Точное значение интеграла: " << std::setprecision(9) << exact << "\n";
    std::cout << "Погрешность при n = " << n << ": " << error_main << "\n";
    std::cout << "Точность 1e-2 достигнута: " << (error_main < 1e-2 ? "да" : "нет") << "\n";
    std::cout << "Точность 1e-3 достигнута: " << (error_main < 1e-3 ? "да" : "нет") << "\n";

    return 0;
}
