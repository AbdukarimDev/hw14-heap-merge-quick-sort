// 과제 14: 힙 정렬, 병합 정렬, 퀵 정렬(3가지 피벗) 성능 비교
// 컴파일: g++ -O2 -std=c++17 main.cpp -o main
#include <bits/stdc++.h>
using namespace std;
using Clock = chrono::steady_clock;

static long long g_cmp = 0;
static int g_depth = 0, g_maxDepth = 0;           // 재귀 깊이 추적
static mt19937 pivotRng(777);                      // 랜덤 피벗용 (시간·비교에 영향 없음)

const long long FRAME = 48;                        // 재귀 호출 1회당 스택 프레임 추정 크기(Byte)

// ---------------- 힙 정렬 (최대 힙, 제자리, 반복문 구현) ----------------
void siftDown(int* a, int i, int size) {
    int tmp = a[i];
    while (true) {
        int c = 2 * i + 1;
        if (c >= size) break;
        if (c + 1 < size) { g_cmp++; if (a[c + 1] > a[c]) c++; }   // 두 자식 비교
        g_cmp++;                                                    // 부모와 큰 자식 비교
        if (a[c] > tmp) { a[i] = a[c]; i = c; }
        else break;
    }
    a[i] = tmp;
}
void heapSort(int* a, int n) {
    for (int i = n / 2 - 1; i >= 0; i--) siftDown(a, i, n);        // 힙 구성
    for (int end = n - 1; end > 0; end--) {
        swap(a[0], a[end]);                                         // 최댓값을 뒤로
        siftDown(a, 0, end);                                        // 힙 속성 복구
    }
}
// 보조 변수: n, i, end, c, size, tmp (int 6개). 재귀 없음
const long long HEAP_MEM = 6 * sizeof(int);

// ---------------- 병합 정렬 ----------------
void mergeRec(int* a, int* tmp, int lo, int hi) {
    g_depth++; g_maxDepth = max(g_maxDepth, g_depth);
    if (lo < hi) {
        int mid = (lo + hi) / 2;
        mergeRec(a, tmp, lo, mid);
        mergeRec(a, tmp, mid + 1, hi);
        int i = lo, j = mid + 1, k = lo;
        while (i <= mid && j <= hi) {
            g_cmp++;
            if (a[i] <= a[j]) tmp[k++] = a[i++];
            else              tmp[k++] = a[j++];
        }
        while (i <= mid) tmp[k++] = a[i++];
        while (j <= hi)  tmp[k++] = a[j++];
        for (k = lo; k <= hi; k++) a[k] = tmp[k];
    }
    g_depth--;
}
long long mergeSort(int* a, int n) {
    vector<int> tmp(n);                                             // 임시 배열 (n x 4B)
    g_depth = g_maxDepth = 0;
    mergeRec(a, tmp.data(), 0, n - 1);
    return (long long)n * sizeof(int) + g_maxDepth * FRAME;
}

// ---------------- 퀵 정렬 ----------------
enum Pivot { FIRST, RANDOM, MEDIAN3 };

int medianOf3(int* a, int x, int y, int z) {        // 비교 최대 3회, 중앙값 인덱스 반환
    g_cmp++;
    if (a[x] < a[y]) {
        g_cmp++;
        if (a[y] < a[z]) return y;
        g_cmp++;
        return (a[x] < a[z]) ? z : x;
    } else {
        g_cmp++;
        if (a[x] < a[z]) return x;
        g_cmp++;
        return (a[y] < a[z]) ? z : y;
    }
}

void quickRec(int* a, int lo, int hi, Pivot pv) {
    g_depth++; g_maxDepth = max(g_maxDepth, g_depth);
    if (lo < hi) {
        int p;
        if (pv == FIRST) p = lo;
        else if (pv == RANDOM) p = lo + pivotRng() % (hi - lo + 1);
        else p = medianOf3(a, lo, (lo + hi) / 2, hi);
        swap(a[p], a[hi]);                                          // 피벗을 끝으로
        int pivot = a[hi], i = lo;
        for (int j = lo; j < hi; j++) {                             // Lomuto 분할
            g_cmp++;
            if (a[j] < pivot) swap(a[i++], a[j]);
        }
        swap(a[i], a[hi]);
        quickRec(a, lo, i - 1, pv);
        quickRec(a, i + 1, hi, pv);
    }
    g_depth--;
}
long long quickSort(int* a, int n, Pivot pv) {
    g_depth = g_maxDepth = 0;
    quickRec(a, 0, n - 1, pv);
    return g_maxDepth * FRAME;                                      // 보조 자료구조 없음, 스택만
}

// ---------------- 실험 ----------------
struct Row { string name; long long cmp; double ms; long long mem; bool ok; };

int main() {
    const int sizes[] = {1000, 2000, 4000, 8000, 16000, 20000};
    const int REPEAT = 10;                                          // 시간 평균용 반복 횟수
    mt19937 rng(2024);

    for (int n : sizes) {
        vector<int> orig(n);
        for (auto& x : orig) x = rng() % 100000;                    // 중복 허용
        vector<int> expect = orig;
        sort(expect.begin(), expect.end());

        auto run = [&](const string& name, function<long long(int*, int)> f) {
            Row r{name, 0, 0, 0, true};
            double total = 0;
            for (int t = 0; t < REPEAT; t++) {
                vector<int> a = orig;                               // 복사 시간은 측정 제외
                pivotRng.seed(777);
                g_cmp = 0;
                auto t0 = Clock::now();
                long long mem = f(a.data(), n);
                auto t1 = Clock::now();
                total += chrono::duration<double, milli>(t1 - t0).count();
                r.cmp = g_cmp; r.mem = mem;
                if (a != expect) r.ok = false;                      // 정렬 검증
            }
            r.ms = total / REPEAT;
            return r;
        };

        vector<Row> rows;
        rows.push_back(run("Heap Sort", [](int* a, int n) { heapSort(a, n); return HEAP_MEM; }));
        rows.push_back(run("Merge Sort", [](int* a, int n) { return mergeSort(a, n); }));
        rows.push_back(run("Quick (First)", [](int* a, int n) { return quickSort(a, n, FIRST); }));
        rows.push_back(run("Quick (Random)", [](int* a, int n) { return quickSort(a, n, RANDOM); }));
        rows.push_back(run("Quick (Median3)", [](int* a, int n) { return quickSort(a, n, MEDIAN3); }));

        printf("Data Size : %d\n\n", n);
        printf("%-17s%-15s%-12s%-15s%s\n", "Algorithm", "Comparisons", "Time(ms)", "Memory(Bytes)", "Sorted");
        printf("----------------------------------------------------------------------\n");
        for (auto& r : rows)
            printf("%-17s%-15lld%-12.4f%-15lld%s\n", r.name.c_str(), r.cmp, r.ms, r.mem, r.ok ? "OK" : "FAIL");
        printf("\n");
    }
    return 0;
}
