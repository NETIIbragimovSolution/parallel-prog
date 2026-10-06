#!/usr/bin/env python3
"""
Строит графики и сводную таблицу (п.5.1-5.2 отчёта) по CSV,
которые генерирует ./integrate: results_posix.csv, results_stdthread.csv.

Запуск (из корня проекта, после ./integrate):
    python3 scripts/plot_results.py
"""

import csv
import os

import matplotlib.pyplot as plt

CSV_FILES = {
    "POSIX": "results_posix.csv",
    "std::thread": "results_stdthread.csv",
}
OUT_DIR = "plots"


def load(path):
    with open(path, newline="") as f:
        reader = csv.DictReader(f)
        rows = [
            {
                "threads": int(r["threads"]),
                "time_sec": float(r["time_sec"]),
                "result": float(r["result"]),
                "speedup": float(r["speedup"]),
                "efficiency_percent": float(r["efficiency_percent"]),
            }
            for r in reader
        ]
    return rows


def annotate_points(ax, xs, ys, fmt="{:.3f}", offset=(0, 8)):
    # Подписывает каждую точку её значением (чуть выше маркера).
    for x, y in zip(xs, ys):
        ax.annotate(fmt.format(y), (x, y), textcoords="offset points", xytext=offset,
                    ha="center", fontsize=8)


def offset_for(i):
    # Разносит подписи разных серий по вертикали, чтобы они не накладывались
    # друг на друга в точках, где значения серий близки.
    return (0, 8) if i % 2 == 0 else (0, -14)


def print_summary_table(data):
    # п.5.1: сводная таблица потоки/время/результат/ускорение/эффективность
    print("\n=== Сводная таблица (п.5.1) ===")
    for name, rows in data.items():
        print(f"\n{name}:")
        print(f"{'Потоки':<8}{'Время (с)':<14}{'Результат':<14}{'Ускорение':<12}{'Эффект-ть (%)':<14}")
        for r in rows:
            print(
                f"{r['threads']:<8}{r['time_sec']:<14.6f}{r['result']:<14.6f}"
                f"{r['speedup']:<12.3f}{r['efficiency_percent']:<14.2f}"
            )


def plot(data):
    os.makedirs(OUT_DIR, exist_ok=True)

    # 5.2.1 Время выполнения
    fig, ax = plt.subplots()
    for i, (name, rows) in enumerate(data.items()):
        xs = [r["threads"] for r in rows]
        ys = [r["time_sec"] for r in rows]
        ax.plot(xs, ys, marker="o", label=name)
        annotate_points(ax, xs, ys, fmt="{:.4f}", offset=offset_for(i))
    ax.set_xlabel("Число потоков")
    ax.set_ylabel("Время выполнения (с)")
    ax.set_title("Время выполнения")
    ax.legend()
    ax.grid(True)
    fig.savefig(f"{OUT_DIR}/1_time.png")

    # 5.2.2 Ускорение (+ идеальное ускорение для наглядности)
    fig, ax = plt.subplots()
    max_threads = max(r["threads"] for rows in data.values() for r in rows)
    ax.plot([1, max_threads], [1, max_threads], "--", color="gray", label="Идеальное")
    for i, (name, rows) in enumerate(data.items()):
        xs = [r["threads"] for r in rows]
        ys = [r["speedup"] for r in rows]
        ax.plot(xs, ys, marker="o", label=name)
        annotate_points(ax, xs, ys, fmt="{:.2f}", offset=offset_for(i))
    ax.set_xlabel("Число потоков")
    ax.set_ylabel("Ускорение")
    ax.set_title("Ускорение")
    ax.legend()
    ax.grid(True)
    fig.savefig(f"{OUT_DIR}/2_speedup.png")

    # 5.2.3 Эффективность использования потоков
    fig, ax = plt.subplots()
    for i, (name, rows) in enumerate(data.items()):
        xs = [r["threads"] for r in rows]
        ys = [r["efficiency_percent"] for r in rows]
        ax.plot(xs, ys, marker="o", label=name)
        annotate_points(ax, xs, ys, fmt="{:.1f}", offset=offset_for(i))
    ax.axhline(100, linestyle="--", color="gray", label="Идеальная (100%)")
    ax.set_xlabel("Число потоков")
    ax.set_ylabel("Эффективность (%)")
    ax.set_title("Эффективность использования потоков")
    ax.legend()
    ax.grid(True)
    fig.savefig(f"{OUT_DIR}/3_efficiency.png")

    # 5.2.4 Результаты вычислений (значение интеграла должно быть стабильным)
    fig, ax = plt.subplots()
    for i, (name, rows) in enumerate(data.items()):
        xs = [r["threads"] for r in rows]
        ys = [r["result"] for r in rows]
        ax.plot(xs, ys, marker="o", label=name)
        annotate_points(ax, xs, ys, fmt="{:.6f}", offset=offset_for(i))
    ax.set_xlabel("Число потоков")
    ax.set_ylabel("Значение интеграла")
    ax.set_title("Результаты вычислений")
    ax.legend()
    ax.grid(True)
    fig.savefig(f"{OUT_DIR}/4_results.png")

    # 5.2.5 Идеальное vs фактическое ускорение (отдельно по каждой технологии)
    fig, axes = plt.subplots(1, len(data), figsize=(6 * len(data), 4), squeeze=False)
    for ax, (name, rows) in zip(axes[0], data.items()):
        threads = [r["threads"] for r in rows]
        speedups = [r["speedup"] for r in rows]
        ax.plot(threads, threads, "--", color="gray", label="Идеальное")
        ax.plot(threads, speedups, marker="o", label="Фактическое")
        annotate_points(ax, threads, speedups, fmt="{:.2f}")
        ax.set_xlabel("Число потоков")
        ax.set_ylabel("Ускорение")
        ax.set_title(name)
        ax.legend()
        ax.grid(True)
    fig.suptitle("Сравнение идеального и фактического ускорения")
    fig.savefig(f"{OUT_DIR}/5_ideal_vs_actual.png")

    # 5.2.6 Эффективность относительно идеального ускорения (= efficiency %, тот же смысл,
    # но здесь явно как отношение факт/идеал ускорения в процентах)
    fig, ax = plt.subplots()
    for i, (name, rows) in enumerate(data.items()):
        xs = [r["threads"] for r in rows]
        ratio = [r["speedup"] / r["threads"] * 100 for r in rows]
        ax.plot(xs, ratio, marker="o", label=name)
        annotate_points(ax, xs, ratio, fmt="{:.1f}", offset=offset_for(i))
    ax.axhline(100, linestyle="--", color="gray", label="Идеал (100%)")
    ax.set_xlabel("Число потоков")
    ax.set_ylabel("Факт. ускорение / идеальное ускорение (%)")
    ax.set_title("Эффективность относительно идеального ускорения")
    ax.legend()
    ax.grid(True)
    fig.savefig(f"{OUT_DIR}/6_efficiency_vs_ideal.png")

    print(f"\nГрафики сохранены в {OUT_DIR}/")


def main():
    data = {}
    for name, path in CSV_FILES.items():
        if not os.path.exists(path):
            print(f"Файл {path} не найден. Сначала запустите ./integrate.")
            return
        data[name] = load(path)

    print_summary_table(data)
    plot(data)


if __name__ == "__main__":
    main()
