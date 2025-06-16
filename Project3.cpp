#include <mpi.h>
#include <iostream>
#include <fstream>

#define MAX_NAME_LEN 100
#define MAX_MAJOR_LEN 50
#define MAX_TUPLES 10000
#define MAX_LINE_LEN 1024

struct Student {
    char name[MAX_NAME_LEN + 1];
    char major[MAX_MAJOR_LEN + 1];
    int grad_year;
};

static Student localData[MAX_TUPLES];
static int localCount = 0;

static int mystrlen(const char* s) {
    int len = 0;
    while (s[len]) len++;
    return len;
}

static void mystrcpy(char* d, const char* s) {
    int i = 0;
    for (; s[i]; i++) d[i] = s[i];
    d[i] = '\0';
}

static const char* mystrstr(const char* h, const char* n) {
    if (!h || !n) return 0;
    int hl = mystrlen(h), nl = mystrlen(n);
    if (nl == 0) return h;
    for (int i = 0; i <= hl - nl; i++) {
        int j = 0;
        for (; j < nl; j++) {
            if (h[i + j] != n[j]) break;
        }
        if (j == nl) return h + i;
    }
    return 0;
}

static int myatoi(const char* s) {
    int r = 0, i = 0, sign = 1;
    if (s[0] == '-') { sign = -1; i = 1; }
    for (; s[i]; i++) {
        if (s[i] >= '0' && s[i] <= '9') r = r * 10 + (s[i] - '0');
        else break;
    }
    return r * sign;
}

static bool extractQuoted(const char* str, char* out, int maxLen, int which) {
    int count = 0;
    const char* start = 0;
    const char* end = 0;
    for (int i = 0; str[i]; i++) {
        if (str[i] == '\'') {
            count++;
            if (count == (2 * which - 1)) start = str + i + 1;
            else if (count == (2 * which)) { end = str + i; break; }
        }
    }
    if (!start || !end) return false;
    int length = (int)(end - start);
    if (length > maxLen) length = maxLen;
    for (int i = 0; i < length; i++) out[i] = start[i];
    out[length] = '\0';
    return true;
}

static bool parseInsert(const char* q, char* name, char* major, int& year) {
    if (!mystrstr(q, "INSERT INTO students")) return false;
    if (!mystrstr(q, "VALUES(")) return false;
    if (!extractQuoted(mystrstr(q, "VALUES("), name, MAX_NAME_LEN, 1)) return false;
    if (!extractQuoted(mystrstr(q, "VALUES("), major, MAX_MAJOR_LEN, 2)) return false;
    const char* v = mystrstr(q, "VALUES(");
    v += 7;
    const char* c = v;
    while (*c) c++;
    while (c > v && *c != ')') c--;
    if (c == v) return false;
    int commas = 0;
    for (const char* p = v; p < c; p++) {
        if (*p == ',') commas++;
    }
    if (commas < 2) return false;
    int ccount = 0;
    const char* p = v;
    while (p < c && ccount < 2) {
        if (*p == ',') ccount++;
        p++;
    }
    while (*p == ' ' || *p == '\t') p++;
    year = myatoi(p);
    return true;
}

static bool parseSelect(const char* q, char* major, int& year) {
    if (!mystrstr(q, "SELECT")) return false;
    if (!mystrstr(q, "FROM students")) return false;
    if (!mystrstr(q, "WHERE")) return false;
    if (!mystrstr(q, "major='")) return false;
    if (!mystrstr(q, "graduation_year=")) return false;
    const char* m = mystrstr(q, "major='");
    m += 7;
    const char* mEnd = m;
    while (*mEnd && *mEnd != '\'') mEnd++;
    int mlen = (int)(mEnd - m);
    if (mlen > MAX_MAJOR_LEN) mlen = MAX_MAJOR_LEN;
    for (int i = 0; i < mlen; i++) major[i] = m[i];
    major[mlen] = '\0';

    const char* y = mystrstr(q, "graduation_year=");
    y += 16;
    year = myatoi(y);
    return true;
}

static int hashMajor(const char* major, int size) {
    unsigned long sum = 0;
    for (int i = 0; major[i]; i++) sum += (unsigned char)major[i];
    return (int)(sum % size);
}

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    double startTime = MPI_Wtime();

    char line[MAX_LINE_LEN];
    bool done = false;

    while (!done) {
        if (rank == 0) {
            if (!std::cin.getline(line, MAX_LINE_LEN)) {
                const char* e = "EXIT;";
                int len = mystrlen(e);
                MPI_Bcast(&len, 1, MPI_INT, 0, MPI_COMM_WORLD);
                MPI_Bcast((char*)e, len + 1, MPI_CHAR, 0, MPI_COMM_WORLD);
                done = true;
                break;
            }
            int len = mystrlen(line);
            MPI_Bcast(&len, 1, MPI_INT, 0, MPI_COMM_WORLD);
            MPI_Bcast(line, len + 1, MPI_CHAR, 0, MPI_COMM_WORLD);

            if (mystrstr(line, "EXIT")) {
                done = true;
            }
            else if (mystrstr(line, "INSERT")) {
                char n[MAX_NAME_LEN + 1];
                char mj[MAX_MAJOR_LEN + 1];
                int y;
                if (parseInsert(line, n, mj, y)) {
                    int target = hashMajor(mj, size);
                    if (target == 0 && localCount < MAX_TUPLES) {
                        mystrcpy(localData[localCount].name, n);
                        mystrcpy(localData[localCount].major, mj);
                        localData[localCount].grad_year = y;
                        localCount++;
                    }
                }
            }
            else if (mystrstr(line, "SELECT")) {
                char mj[MAX_MAJOR_LEN + 1];
                int y;
                if (parseSelect(line, mj, y)) {
                    for (int i = 0; i < localCount; i++) {
                        int ml = mystrlen(mj);
                        bool eq = (mystrlen(localData[i].major) == ml);
                        if (eq) {
                            for (int c = 0; c < ml; c++) {
                                if (localData[i].major[c] != mj[c]) { eq = false; break; }
                            }
                        }
                        if (eq && localData[i].grad_year == y) {
                            std::cout << localData[i].name << " | "
                                << localData[i].major << " | "
                                << localData[i].grad_year << "\n";
                        }
                    }

                    for (int nd = 1; nd < size; nd++) {
                        int cnt;
                        MPI_Recv(&cnt, 1, MPI_INT, nd, 3, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                        for (int m = 0; m < cnt; m++) {
                            int nlen; MPI_Recv(&nlen, 1, MPI_INT, nd, 4, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                            char nb[MAX_NAME_LEN + 1]; MPI_Recv(nb, nlen + 1, MPI_CHAR, nd, 4, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                            int mjlen; MPI_Recv(&mjlen, 1, MPI_INT, nd, 4, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                            char mb[MAX_MAJOR_LEN + 1]; MPI_Recv(mb, mjlen + 1, MPI_CHAR, nd, 4, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                            int yy; MPI_Recv(&yy, 1, MPI_INT, nd, 4, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                            std::cout << nb << " | " << mb << " | " << yy << "\n";
                        }
                    }
                }
            }

        }
        else {
            int len;
            MPI_Bcast(&len, 1, MPI_INT, 0, MPI_COMM_WORLD);
            if (len <= 0) continue;

            char cmdBuf[MAX_LINE_LEN];
            MPI_Bcast(cmdBuf, len + 1, MPI_CHAR, 0, MPI_COMM_WORLD);

            if (mystrstr(cmdBuf, "EXIT")) {
                done = true;
            }
            else if (mystrstr(cmdBuf, "INSERT")) {
                char n[MAX_NAME_LEN + 1];
                char mj[MAX_MAJOR_LEN + 1];
                int y;
                if (parseInsert(cmdBuf, n, mj, y)) {
                    int target = hashMajor(mj, size);
                    if (target == rank && localCount < MAX_TUPLES) {
                        mystrcpy(localData[localCount].name, n);
                        mystrcpy(localData[localCount].major, mj);
                        localData[localCount].grad_year = y;
                        localCount++;
                    }
                }
            }
            else if (mystrstr(cmdBuf, "SELECT")) {
                char mj[MAX_MAJOR_LEN + 1];
                int y;
                if (parseSelect(cmdBuf, mj, y)) {
                    int ccount = 0;
                    for (int i = 0; i < localCount; i++) {
                        int ml = mystrlen(mj);
                        bool eq = (mystrlen(localData[i].major) == ml);
                        if (eq) {
                            for (int c = 0; c < ml; c++) {
                                if (localData[i].major[c] != mj[c]) { eq = false; break; }
                            }
                        }
                        if (eq && localData[i].grad_year == y) ccount++;
                    }

                    MPI_Send(&ccount, 1, MPI_INT, 0, 3, MPI_COMM_WORLD);
                    for (int i = 0; i < localCount; i++) {
                        int ml = mystrlen(mj);
                        bool eq = (mystrlen(localData[i].major) == ml);
                        if (eq) {
                            for (int c = 0; c < ml; c++) {
                                if (localData[i].major[c] != mj[c]) { eq = false; break; }
                            }
                        }
                        if (eq && localData[i].grad_year == y) {
                            int nlen = mystrlen(localData[i].name);
                            MPI_Send(&nlen, 1, MPI_INT, 0, 4, MPI_COMM_WORLD);
                            MPI_Send(localData[i].name, nlen + 1, MPI_CHAR, 0, 4, MPI_COMM_WORLD);
                            int mjlen = mystrlen(localData[i].major);
                            MPI_Send(&mjlen, 1, MPI_INT, 0, 4, MPI_COMM_WORLD);
                            MPI_Send(localData[i].major, mjlen + 1, MPI_CHAR, 0, 4, MPI_COMM_WORLD);
                            MPI_Send(&localData[i].grad_year, 1, MPI_INT, 0, 4, MPI_COMM_WORLD);
                        }
                    }
                }
            }
        }
    }

    double endTime = MPI_Wtime();
    double elapsed = endTime - startTime;

    if (rank == 0) {
        std::cout << "Runtime: " << elapsed << " seconds." << std::endl;
    }

    size_t sizeOfStudent = sizeof(Student);
    long long localMemBytes = (long long)localCount * (long long)sizeOfStudent;
    long long sumMemBytes = 0;
    MPI_Reduce(&localMemBytes, &sumMemBytes, 1, MPI_LONG_LONG, MPI_SUM, 0, MPI_COMM_WORLD);

    if (rank == 0 && size > 0) {
        long long avgMemBytes = sumMemBytes / size;
        std::cout << "Average Memory Usage across all ranks: " << avgMemBytes << " bytes" << std::endl;
    }

    MPI_Finalize();
    return 0;
}
