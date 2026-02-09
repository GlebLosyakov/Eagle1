#include <iostream>
#include <windows.h>
#include <vector>
#include <cmath>
#include <iomanip>

using namespace std;

struct ThreadData {
    double start, end, step, result;
};

// Чистая математическая функция без лишних задержек
DWORD WINAPI IntegratePart(LPVOID lpParam) {
    ThreadData* data = (ThreadData*)lpParam;
    double sum = 0.0;
    for (double x = data->start; x < data->end; x += data->step) {
        sum += sqrt(x) * data->step;
    }
    data->result = sum;
    return 0;
}

int main(int argc, char* argv[]) {
    setlocale(LC_ALL, "Russian");

    int cpuLimit; // По умолчанию 50%
    cout << "Введите уровень загруженности(в процентах):" << endl;
    cin >> cpuLimit;
    if (argc > 1) cpuLimit = atoi(argv[1]);

    // --- НАСТРОЙКА JOB OBJECT (Ваш Способ 1) ---
    HANDLE hJob = CreateJobObject(NULL, NULL);
    JOBOBJECT_CPU_RATE_CONTROL_INFORMATION info = {};
    info.ControlFlags = JOB_OBJECT_CPU_RATE_CONTROL_ENABLE | JOB_OBJECT_CPU_RATE_CONTROL_HARD_CAP;
    // CpuRate задается в долях от 10 000. 100% = 10000.
    info.CpuRate = cpuLimit * 100;

    SetInformationJobObject(hJob, JobObjectCpuRateControlInformation, &info, sizeof(info));
    AssignProcessToJobObject(hJob, GetCurrentProcess());
    // -------------------------------------------

    const double A = 1.0, B = 100.0, STEP = 0.000001;
    const int NUM_THREADS = 6;
    HANDLE threads[NUM_THREADS];
    ThreadData threadData[NUM_THREADS];
    double range = (B - A) / NUM_THREADS;

    for (int i = 0; i < NUM_THREADS; i++) {
        threadData[i].start = A + i * range;
        threadData[i].end = (i == NUM_THREADS - 1) ? B : (threadData[i].start + range);
        threadData[i].step = STEP;
        threads[i] = CreateThread(NULL, 0, IntegratePart, &threadData[i], 0, NULL);
    }

    WaitForMultipleObjects(NUM_THREADS, threads, TRUE, INFINITE);

    double total = 0;
    for (int i = 0; i < NUM_THREADS; i++) {
        total += threadData[i].result;
        CloseHandle(threads[i]);
    }
    CloseHandle(hJob);

    std::cout << "Результат: " << std::fixed << std::setprecision(6) << total << std::endl;
    return 0;
}