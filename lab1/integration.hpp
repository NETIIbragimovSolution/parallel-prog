#pragma once

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

const double A = 1.0;
const double B = 3.0;

inline double f(double x) {
    double denom = 2.0 + 3.0 * x;
    return (1.0 + x) / (denom * denom);
}

inline double antiderivative(double x) {
    double u = 2.0 + 3.0 * x;
    return (std::log(u) - 1.0 / u) / 9.0;
}

inline double exact_integral() {
    return antiderivative(B) - antiderivative(A);
}

inline double partial_sum(long long start, long long end, double a, double h) {
    double sum = 0.0;
    for (long long i = start; i < end; ++i) {
        sum += f(a + i * h);
    }
    return sum;
}

inline std::vector<std::pair<long long, long long>> split_range(long long n, int threads) {
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

template <typename T>
T read_positive(const std::string& prompt) {
    while (true) {
        std::cout << prompt;
        T value;
        if (std::cin >> value && value > 0) {
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            return value;
        }
        if (std::cin.eof()) {
            throw std::runtime_error("ввод завершён до получения корректного значения");
        }
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        std::cout << "Ошибка: нужно целое число больше 0.\n";
    }
}

inline double median_of(std::vector<double> values) {
    std::sort(values.begin(), values.end());
    return values[values.size() / 2];
}

inline void print_accuracy(long long n, double result) {
    double exact = exact_integral();
    double error = std::fabs(result - exact);
    std::cout << "\n=== Точность ===\n";
    std::cout << std::setprecision(9) << "Точное значение интеграла: " << exact << "\n";
    std::cout << "Погрешность при n = " << n << ": " << error << "\n";
    std::cout << "Точность 1e-6 достигнута: " << (error < 1e-6 ? "да" : "нет") << "\n";
}
