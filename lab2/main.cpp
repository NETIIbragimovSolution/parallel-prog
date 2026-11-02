#include <chrono>
#include <condition_variable>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <vector>
using namespace std;
using namespace std::chrono;

const int WORKERS = 3;
const size_t QUEUE_MAX = 3;
const size_t BATCH = 10;

struct Scenario {
    string name;
    int manager_ms;
    int worker_ms[WORKERS];
    double us_per_division;
    int write_ms;
};

const Scenario SCENARIOS[] = {
    {"Разная скорость управляющего и исполнителей", 10, {100, 200, 400}, 0, 0},
    {"Зависимость от сложности задачи", 10, {10, 10, 10}, 0.5, 0},
    {"Пакетная запись в файл по 10 результатов", 5, {40, 40, 40}, 0, 300},
};

struct Task {
    int id;
    long long n;
};

queue<Task> tasks;
bool closed = false;
mutex queue_mtx;
condition_variable not_empty;
condition_variable not_full;
ofstream result_file;
mutex file_mtx;
mutex out_mtx;
int solved[WORKERS];
Scenario sc;
steady_clock::time_point start;

void log(const string& who, const string& msg) {
    lock_guard<mutex> lk(out_mtx);
    cout << "[" << setw(5) << duration_cast<milliseconds>(steady_clock::now() - start).count() << " мс] "
         << who << ": " << msg << "\n";
}

bool is_prime(long long n, long long& divisions) {
    divisions = 0;
    if (n < 2) return false;
    for (long long d = 2; d < n; ++d) {
        ++divisions;
        if (n % d == 0) return false;
    }
    return true;
}

void write_batch(const string& who, vector<string>& buffer) {
    unique_lock<mutex> lk(file_mtx, try_to_lock);
    if (!lk.owns_lock()) {
        log(who, "файл занят, ждёт");
        lk.lock();
    }
    log(who, "пишет порцию из " + to_string(buffer.size()) + " результатов");
    this_thread::sleep_for(milliseconds(sc.write_ms));
    for (const string& line : buffer) result_file << line << "\n";
    buffer.clear();
}

void worker(int id) {
    string me = "Исполнитель " + to_string(id);
    vector<string> buffer;

    while (true) {
        Task t;
        {
            unique_lock<mutex> lk(queue_mtx);
            not_empty.wait(lk, [] { return !tasks.empty() || closed; });
            if (tasks.empty()) break;
            t = tasks.front();
            tasks.pop();
        }
        not_full.notify_one();

        long long divisions;
        bool prime = is_prime(t.n, divisions);
        this_thread::sleep_for(milliseconds(sc.worker_ms[id - 1]) +
                               microseconds((long long)(divisions * sc.us_per_division)));
        solved[id - 1]++;

        string answer = prime ? "простое" : "составное";
        buffer.push_back("задача " + to_string(t.id) + ": " + to_string(t.n) + " — " + answer +
                         " (исполнитель " + to_string(id) + ")");
        log(me, to_string(t.n) + " — " + answer + ", делений " + to_string(divisions) + ", в буфере " +
                    to_string(buffer.size()) + "/" + to_string(BATCH));

        if (buffer.size() == BATCH) write_batch(me, buffer);
    }

    if (!buffer.empty()) write_batch(me, buffer);
    log(me, "завершает работу");
}

void run_scenario(int k) {
    sc = SCENARIOS[k - 1];
    closed = false;
    fill(begin(solved), end(solved), 0);
    filesystem::create_directories("output");
    result_file.open("output/results_" + to_string(k) + ".txt");
    ifstream in("input.txt");
    start = steady_clock::now();
    cout << "\n=== Сценарий " << k << ": " << sc.name << " ===\n";

    vector<thread> workers;
    for (int i = 1; i <= WORKERS; ++i) workers.emplace_back(worker, i);

    long long n;
    int id = 0;
    while (in >> n) {
        this_thread::sleep_for(milliseconds(sc.manager_ms));
        unique_lock<mutex> lk(queue_mtx);
        if (tasks.size() == QUEUE_MAX) log("Управляющий", "очередь полна, ждёт");
        not_full.wait(lk, [] { return tasks.size() < QUEUE_MAX; });
        tasks.push({++id, n});
        lk.unlock();
        not_empty.notify_one();
    }

    {
        lock_guard<mutex> lk(queue_mtx);
        closed = true;
    }
    not_empty.notify_all();
    log("Управляющий", "задачи кончились, очередь закрыта, ждёт исполнителей");

    for (thread& w : workers) w.join();
    result_file.close();
    log("Управляющий", "все исполнители завершены");
    for (int i = 0; i < WORKERS; ++i) cout << "Исполнитель " << i + 1 << " решил задач: " << solved[i] << "\n";
}

int main(int argc, char* argv[]) {
    int k = argc > 1 ? stoi(argv[1]) : 0;
    for (int i = 1; i <= 3; ++i)
        if (k == 0 || k == i) run_scenario(i);
}
