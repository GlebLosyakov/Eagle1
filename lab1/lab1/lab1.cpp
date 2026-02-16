#include <iostream>
#include <windows.h>
#include <vector>
#include <cmath>
#include <iomanip>

using namespace std;

struct ThreadData {
    double start, end, step, result;
};

// Функция расчета (численное интегрирование методом прямоугольников)
DWORD WINAPI IntegratePart(LPVOID lpParam) {
    ThreadData* data = (ThreadData*)lpParam;
    double sum = 0.0;
    // Используем обычный цикл для высокой нагрузки на CPU
    for (double x = data->start; x < data->end; x += data->step) {
        sum += sqrt(x) * data->step;
    }
    data->result = sum;
    return 0;
}

int main() { // Убрали argc и argv, так как они больше не используются
    setlocale(LC_ALL, "Russian");

    // --- 1. ОПРЕДЕЛЕНИЕ КОЛИЧЕСТВА ЯДЕР ---
    SYSTEM_INFO siSysInfo;
    GetSystemInfo(&siSysInfo);
    int numThreads = siSysInfo.dwNumberOfProcessors;
    
    cout << "Обнаружено ядер процессора: " << numThreads << endl;

    // --- 2. НАСТРОЙКА ОГРАНИЧЕНИЯ CPU ---
    int cpuLimit; 
    cout << "Введите уровень загруженности (в процентах, например 30): ";
    cin >> cpuLimit;

    HANDLE hJob = CreateJobObject(NULL, NULL);
    JOBOBJECT_CPU_RATE_CONTROL_INFORMATION info = {};
    info.ControlFlags = JOB_OBJECT_CPU_RATE_CONTROL_ENABLE | JOB_OBJECT_CPU_RATE_CONTROL_HARD_CAP;
    
    // CpuRate задается в единицах 0.01%. То есть 100% = 10000.
    info.CpuRate = cpuLimit * 100; 

    SetInformationJobObject(hJob, JobObjectCpuRateControlInformation, &info, sizeof(info));
    AssignProcessToJobObject(hJob, GetCurrentProcess());
    
    // ... далее идет остальной код (создание векторов и потоков)
    // --- 3. ПОДГОТОВКА И ЗАПУСК ПОТОКОВ ---
    const double A = 1.0, B = 100.0, STEP = 0.000001;

    // Используем векторы вместо массивов, так как размер numThreads динамический
    vector<HANDLE> threads(numThreads);
    vector<ThreadData> threadData(numThreads);

    double range = (B - A) / numThreads;

    for (int i = 0; i < numThreads; i++) {
        threadData[i].start = A + i * range;
        threadData[i].end = (i == numThreads - 1) ? B : (threadData[i].start + range);
        threadData[i].step = STEP;

        threads[i] = CreateThread(NULL, 0, IntegratePart, &threadData[i], 0, NULL);
    }

    // Ожидание завершения всех потоков
    WaitForMultipleObjects(numThreads, threads.data(), TRUE, INFINITE);

    // --- 4. СБОР РЕЗУЛЬТАТОВ ---
    double total = 0;
    for (int i = 0; i < numThreads; i++) {
        total += threadData[i].result;
        CloseHandle(threads[i]);
    }
    CloseHandle(hJob);

    cout << "------------------------------------" << endl;
    cout << "Количество использованных потоков: " << numThreads << endl;
    cout << "Итоговый результат: " << fixed << setprecision(6) << total << endl;
    cout << "------------------------------------" << endl;

    return 0;
}