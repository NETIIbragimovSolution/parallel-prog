// Общая часть для sequential.cpp и parallel.cpp: функция варианта, её
// первообразная (для точного значения интеграла) и метод левых прямоугольников.
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

// Вариант 1: f(x) = (1+x)/(2+3x)^2, интервал [1;3].
const double A = 1.0; // левая граница интервала
const double B = 3.0; // правая граница интервала

inline double f(double x) {
    double denom = 2.0 + 3.0 * x;
    return (1.0 + x) / (denom * denom);
}

// Первообразная F(x) = (1/9)(ln(2+3x) - 1/(2+3x)), используется только
// чтобы вывести точное значение интеграла и сравнить с ним погрешность.
inline double antiderivative(double x) {
    double u = 2.0 + 3.0 * x;
    return (std::log(u) - 1.0 / u) / 9.0;
}

inline double exact_integral() {
    return antiderivative(B) - antiderivative(A);
}

// Метод левых прямоугольников на индексах [start, end).
inline double partial_sum(long long start, long long end, double a, double h) {
    double sum = 0.0;
    for (long long i = start; i < end; ++i) {
        sum += f(a + i * h);
    }
    return sum;
}

// Делит n индексов на threads по возможности равных непрерывных блоков.
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

// Читает целое положительное число, пока пользователь не введёт корректное значение.
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

// Медиана вместо среднего устойчивее к единичным выбросам (например, когда
// один из повторов планировщик ОС случайно поставил на медленное E-ядро) —
// такой выброс не тянет результат на себя, как это делает среднее.
inline double median_of(std::vector<double> values) {
    std::sort(values.begin(), values.end());
    return values[values.size() / 2];
}

// Печатает точное значение интеграла, погрешность и проверку точности 10^-6.
inline void print_accuracy(long long n, double result) {
    double exact = exact_integral();
    double error = std::fabs(result - exact);
    std::cout << "\n=== Точность ===\n";
    std::cout << std::setprecision(9) << "Точное значение интеграла: " << exact << "\n";
    std::cout << "Погрешность при n = " << n << ": " << error << "\n";
    std::cout << "Точность 1e-6 достигнута: " << (error < 1e-6 ? "да" : "нет") << "\n";
}
