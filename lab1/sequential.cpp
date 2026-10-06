// Лабораторная работа: последовательное (однопоточное) численное
// интегрирование. Служит эталоном по времени для parallel.cpp.
// Вариант 1: f(x) = (1+x)/(2+3x)^2, интервал [1;3].
// Метод: левые прямоугольники.

#include <chrono>
#include <iomanip>
#include <iostream>

#include "integration.hpp"

using Clock = std::chrono::steady_clock;

const int REPEATS = 5;

// Прямой последовательный расчёт без единого потока.
double run_sequential(long long n, double a, double h, double& result) {
    auto t0 = Clock::now();
    double total = partial_sum(0, n, a, h);
    auto t1 = Clock::now();

    result = total * h;
    return std::chrono::duration<double>(t1 - t0).count();
}

// Медиана по REPEATS запускам — устойчивее одиночного выброса, чем среднее.
double avg_time_sequential(long long n, double a, double h, double& result) {
    std::vector<double> times(REPEATS);
    for (int r = 0; r < REPEATS; ++r) {
        times[r] = run_sequential(n, a, h, result);
    }
    return median_of(times);
}

int run() {
    std::cout << "Лабораторная работа: последовательное интегрирование методом левых прямоугольников\n";
    std::cout << "f(x) = (1+x)/(2+3x)^2, интервал [" << A << "; " << B << "]\n\n";

    long long n = read_positive<long long>("Введите количество разбиений n: ");
    double h = (B - A) / static_cast<double>(n);

    double result = 0.0;
    double time_sec = avg_time_sequential(n, A, h, result);

    std::cout << std::fixed << std::setprecision(6);
    std::cout << "\nВремя выполнения (среднее по " << REPEATS << " запускам): " << time_sec << " с\n";
    std::cout << "Результат: " << result << "\n";

    print_accuracy(n, result);

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
