/*
    Student Management System (Final)
    -------------------------------------------------
    Features:
    - Persistent storage in CSV: students.csv (auto-load on start, auto-save on changes)
    - Create / Read / Update / Delete (CRUD)
    - Search by Roll No. or Name (case-insensitive substring)
    - Sorting: by Roll, Name, or Average (Asc/Desc)
    - Statistics: class average, topper, lowest, grade distribution
    - Export: nicely formatted report.txt
    - Clean, menu-driven UI with validation

    Notes:
    - Name cannot contain commas (,) since we use CSV commas. Commas are auto-converted to spaces.
    - Marks allowed: 0..100
    - MAX_SUBJECTS per student can be adjusted.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_STUDENTS 1000
#define MAX_NAME 100
#define MAX_SUBJECTS 10
#define DATA_FILE "students.csv"
#define REPORT_FILE "report.txt"

typedef struct {
    int   roll;
    char  name[MAX_NAME];
    int   subjectCount;
    int   marks[MAX_SUBJECTS];
    float average;
    char  grade;
} Student;

static Student students[MAX_STUDENTS];
static int studentCount = 0;

/* -------------------- Utilities -------------------- */

void waitEnter() {
    printf("\nPress Enter to continue...");
    int c;
    while ((c = getchar()) != '\n' && c != EOF) {}
    getchar();
}

void clearScreen() {
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}

void safeGets(char *buf, int size) {
    if (fgets(buf, size, stdin)) {
        size_t n = strlen(buf);
        if (n && buf[n-1] == '\n') buf[n-1] = '\0';
    }
}

void sanitizeName(char *name) {
    // Replace commas with spaces to keep CSV simple
    for (char *p = name; *p; ++p) {
        if (*p == ',') *p = ' ';
    }
}

int inputIntInRange(const char *prompt, int min, int max) {
    int x;
    while (1) {
        printf("%s", prompt);
        if (scanf("%d", &x) == 1 && x >= min && x <= max) {
            // consume trailing newline
            int ch; while ((ch = getchar()) != '\n' && ch != EOF) {}
            return x;
        }
        printf("Invalid input. Please enter a number between %d and %d.\n", min, max);
        // flush
        int ch; while ((ch = getchar()) != '\n' && ch != EOF) {}
    }
}

int containsIgnoreCase(const char *haystack, const char *needle) {
    if (!*needle) return 1;
    size_t nlen = strlen(needle);
    for (const char *p = haystack; *p; ++p) {
        size_t i = 0;
        while (i < nlen && p[i] && tolower((unsigned char)p[i]) == tolower((unsigned char)needle[i])) {
            i++;
        }
        if (i == nlen) return 1;
    }
    return 0;
}

char calculateGrade(float avg) {
    if (avg >= 90.0f) return 'A';
    else if (avg >= 75.0f) return 'B';
    else if (avg >= 60.0f) return 'C';
    else if (avg >= 50.0f) return 'D';
    else return 'F';
}

void recompute(Student *s) {
    int sum = 0;
    for (int i = 0; i < s->subjectCount; ++i) sum += s->marks[i];
    s->average = (float)sum / (float)s->subjectCount;
    s->grade = calculateGrade(s->average);
}

int findMaxRoll() {
    int maxr = 0;
    for (int i = 0; i < studentCount; ++i)
        if (students[i].roll > maxr) maxr = students[i].roll;
    return maxr;
}

int findIndexByRoll(int roll) {
    for (int i = 0; i < studentCount; ++i)
        if (students[i].roll == roll) return i;
    return -1;
}

/* -------------- Persistence (CSV) ------------------ */
/*
   CSV format (one line per student):
   roll,name,subjectCount,marks_semicolon_separated,average,grade

   Example:
   1, Alice Johnson, 3, 85;90;78, 84.33, B
*/

void saveAll() {
    FILE *fp = fopen(DATA_FILE, "w");
    if (!fp) {
        printf("Error: cannot write to %s\n", DATA_FILE);
        return;
    }
    fprintf(fp, "roll,name,subjectCount,marks,average,grade\n");
    for (int i = 0; i < studentCount; ++i) {
        Student *s = &students[i];
        fprintf(fp, "%d,%s,%d,", s->roll, s->name, s->subjectCount);
        // marks list
        for (int j = 0; j < s->subjectCount; ++j) {
            fprintf(fp, "%d", s->marks[j]);
            if (j != s->subjectCount - 1) fprintf(fp, ";");
        }
        fprintf(fp, ",%.2f,%c\n", s->average, s->grade);
    }
    fclose(fp);
}

void loadAll() {
    FILE *fp = fopen(DATA_FILE, "r");
    if (!fp) {
        // no existing file — start fresh
        studentCount = 0;
        return;
    }
    char line[1024];
    // skip header if present
    if (fgets(line, sizeof(line), fp)) {
        if (strncmp(line, "roll,", 5) != 0) {
            // header missing; line might be data—rewind to parse it
            fseek(fp, 0, SEEK_SET);
        }
    } else {
        fclose(fp);
        return;
    }

    studentCount = 0;
    while (fgets(line, sizeof(line), fp) && studentCount < MAX_STUDENTS) {
        // parse CSV fields
        // tokenize by comma: roll, name, subjectCount, marks_list, average, grade
        char *p = line;
        // strip newline
        size_t ln = strlen(p);
        if (ln && (p[ln-1] == '\n' || p[ln-1] == '\r')) p[--ln] = '\0';

        char *tok;
        // roll
        tok = strtok(p, ",");
        if (!tok) continue;
        Student s = {0};
        s.roll = atoi(tok);

        // name
        tok = strtok(NULL, ",");
        if (!tok) continue;
        strncpy(s.name, tok, MAX_NAME - 1);
        s.name[MAX_NAME - 1] = '\0';

        // subjectCount
        tok = strtok(NULL, ",");
        if (!tok) continue;
        s.subjectCount = atoi(tok);
        if (s.subjectCount < 1 || s.subjectCount > MAX_SUBJECTS) continue;

        // marks list (semicolon separated)
        tok = strtok(NULL, ",");
        if (!tok) continue;
        {
            char tmp[512];
            strncpy(tmp, tok, sizeof(tmp) - 1);
            tmp[sizeof(tmp) - 1] = '\0';
            int idx = 0;
            char *mtok = strtok(tmp, ";");
            while (mtok && idx < s.subjectCount) {
                s.marks[idx++] = atoi(mtok);
                mtok = strtok(NULL, ";");
            }
            if (idx != s.subjectCount) continue; // malformed line
        }

        // average
        tok = strtok(NULL, ",");
        if (!tok) continue;
        s.average = (float)atof(tok);

        // grade
        tok = strtok(NULL, ",");
        if (!tok) continue;
        s.grade = tok[0];

        students[studentCount++] = s;
    }
    fclose(fp);
}

/* -------------------- UI Helpers ------------------- */

void printBanner() {
    printf("==============================================\n");
    printf("     Student Management System – v1.0 (C)     \n");
    printf("==============================================\n");
}

void printTableHeader() {
    printf("\n%-6s  %-25s  %-8s  %-8s  %-5s\n", "Roll", "Name", "Subjects", "Average", "Grade");
    printf("------  -------------------------  --------  --------  -----\n");
}

void printStudentRow(const Student *s) {
    printf("%-6d  %-25.25s  %-8d  %-8.2f  %-5c\n",
           s->roll, s->name, s->subjectCount, s->average, s->grade);
}

/* -------------------- Core Actions ----------------- */

void addStudent() {
    if (studentCount >= MAX_STUDENTS) {
        printf("Cannot add more students (limit reached).\n");
        return;
    }

    Student s = {0};
    s.roll = findMaxRoll() + 1;

    printf("Enter student name: ");
    char buf[256];
    safeGets(buf, sizeof(buf));
    if (strlen(buf) == 0) {
        printf("Name cannot be empty.\n");
        return;
    }
    sanitizeName(buf);
    strncpy(s.name, buf, MAX_NAME - 1);

    s.subjectCount = inputIntInRange("Enter number of subjects (1-10): ", 1, MAX_SUBJECTS);

    for (int i = 0; i < s.subjectCount; ++i) {
        char prompt[64];
        snprintf(prompt, sizeof(prompt), "Enter marks for subject %d (0-100): ", i + 1);
        s.marks[i] = inputIntInRange(prompt, 0, 100);
    }
    recompute(&s);
    students[studentCount++] = s;
    saveAll();

    printf("\n✅ Added: Roll %d | %s | Avg: %.2f | Grade: %c\n", s.roll, s.name, s.average, s.grade);
}

void listAll() {
    if (studentCount == 0) {
        printf("No records to display.\n");
        return;
    }
    printTableHeader();
    for (int i = 0; i < studentCount; ++i) printStudentRow(&students[i]);
}

void searchByRoll() {
    if (studentCount == 0) { printf("No records.\n"); return; }
    int roll = inputIntInRange("Enter roll number: ", 1, 1000000000);
    int idx = findIndexByRoll(roll);
    if (idx < 0) { printf("No student with roll %d.\n", roll); return; }
    printTableHeader();
    printStudentRow(&students[idx]);
    // show marks detail
    printf("Marks: ");
    for (int i = 0; i < students[idx].subjectCount; ++i) {
        printf("%d", students[idx].marks[i]);
        if (i != students[idx].subjectCount - 1) printf(", ");
    }
    printf("\n");
}

void searchByName() {
    if (studentCount == 0) { printf("No records.\n"); return; }
    char q[128];
    printf("Enter name (or part of it): ");
    safeGets(q, sizeof(q));
    if (strlen(q) == 0) { printf("Query empty.\n"); return; }

    int hits = 0;
    printTableHeader();
    for (int i = 0; i < studentCount; ++i) {
        if (containsIgnoreCase(students[i].name, q)) {
            printStudentRow(&students[i]);
            hits++;
        }
    }
    if (!hits) printf("No matches for \"%s\".\n", q);
}

void updateStudent() {
    if (studentCount == 0) { printf("No records.\n"); return; }
    int roll = inputIntInRange("Enter roll number to update: ", 1, 1000000000);
    int idx = findIndexByRoll(roll);
    if (idx < 0) { printf("No student with roll %d.\n", roll); return; }

    Student *s = &students[idx];
    printf("\nEditing Roll %d (%s)\n", s->roll, s->name);
    printf("1) Update Name\n");
    printf("2) Update Subjects & Marks\n");
    printf("3) Cancel\n");
    int ch = inputIntInRange("Choose: ", 1, 3);

    if (ch == 1) {
        printf("New name: ");
        char buf[256];
        safeGets(buf, sizeof(buf));
        if (strlen(buf) == 0) { printf("Name unchanged.\n"); return; }
        sanitizeName(buf);
        strncpy(s->name, buf, MAX_NAME - 1);
    } else if (ch == 2) {
        s->subjectCount = inputIntInRange("Enter number of subjects (1-10): ", 1, MAX_SUBJECTS);
        for (int i = 0; i < s->subjectCount; ++i) {
            char prompt[64];
            snprintf(prompt, sizeof(prompt), "Enter marks for subject %d (0-100): ", i + 1);
            s->marks[i] = inputIntInRange(prompt, 0, 100);
        }
        recompute(s);
    } else {
        printf("Cancelled.\n");
        return;
    }

    recompute(s);
    saveAll();
    printf("✅ Updated successfully.\n");
}

void deleteStudent() {
    if (studentCount == 0) { printf("No records.\n"); return; }
    int roll = inputIntInRange("Enter roll number to delete: ", 1, 1000000000);
    int idx = findIndexByRoll(roll);
    if (idx < 0) { printf("No student with roll %d.\n", roll); return; }

    printf("Are you sure you want to delete Roll %d (%s)? (y/n): ", students[idx].roll, students[idx].name);
    int c = getchar(); int ch; while ((ch = getchar()) != '\n' && ch != EOF) {}
    if (c != 'y' && c != 'Y') { printf("Cancelled.\n"); return; }

    for (int i = idx; i < studentCount - 1; ++i) students[i] = students[i + 1];
    studentCount--;
    saveAll();
    printf("✅ Deleted.\n");
}

/* -------------------- Sorting ---------------------- */

int cmpRollAsc(const void *a, const void *b) {
    const Student *x = (const Student *)a, *y = (const Student *)b;
    return (x->roll - y->roll);
}
int cmpRollDesc(const void *a, const void *b) { return -cmpRollAsc(a,b); }

int cmpNameAsc(const void *a, const void *b) {
    const Student *x = (const Student *)a, *y = (const Student *)b;
    return strcasecmp(x->name, y->name);
}
int cmpNameDesc(const void *a, const void *b) { return -cmpNameAsc(a,b); }

int cmpAvgAsc(const void *a, const void *b) {
    const Student *x = (const Student *)a, *y = (const Student *)b;
    if (x->average < y->average) return -1;
    if (x->average > y->average) return  1;
    return 0;
}
int cmpAvgDesc(const void *a, const void *b) { return -cmpAvgAsc(a,b); }

void sortMenu() {
    if (studentCount == 0) { printf("No records.\n"); return; }
    printf("\nSort by:\n");
    printf("1) Roll (Asc)\n");
    printf("2) Roll (Desc)\n");
    printf("3) Name (Asc)\n");
    printf("4) Name (Desc)\n");
    printf("5) Average (Asc)\n");
    printf("6) Average (Desc)\n");
    int ch = inputIntInRange("Choose: ", 1, 6);

    switch (ch) {
        case 1: qsort(students, studentCount, sizeof(Student), cmpRollAsc); break;
        case 2: qsort(students, studentCount, sizeof(Student), cmpRollDesc); break;
        case 3: qsort(students, studentCount, sizeof(Student), cmpNameAsc); break;
        case 4: qsort(students, studentCount, sizeof(Student), cmpNameDesc); break;
        case 5: qsort(students, studentCount, sizeof(Student), cmpAvgAsc); break;
        case 6: qsort(students, studentCount, sizeof(Student), cmpAvgDesc); break;
    }
    saveAll();
    printf("✅ Sorted.\n");
}

/* -------------------- Statistics ------------------- */

void showStats() {
    if (studentCount == 0) { printf("No records.\n"); return; }

    float classSum = 0.0f;
    int gradeA=0, gradeB=0, gradeC=0, gradeD=0, gradeF=0;
    int topIdx = 0, lowIdx = 0;

    for (int i = 0; i < studentCount; ++i) {
        classSum += students[i].average;
        if (students[i].average > students[topIdx].average) topIdx = i;
        if (students[i].average < students[lowIdx].average) lowIdx = i;
        switch (students[i].grade) {
            case 'A': gradeA++; break;
            case 'B': gradeB++; break;
            case 'C': gradeC++; break;
            case 'D': gradeD++; break;
            default: gradeF++; break;
        }
    }

    float classAvg = classSum / studentCount;

    printf("\n--- Statistics ---\n");
    printf("Total students : %d\n", studentCount);
    printf("Class average  : %.2f\n", classAvg);
    printf("Topper         : Roll %d (%s) Avg %.2f\n", students[topIdx].roll, students[topIdx].name, students[topIdx].average);
    printf("Lowest         : Roll %d (%s) Avg %.2f\n", students[lowIdx].roll, students[lowIdx].name, students[lowIdx].average);
    printf("Grades         : A=%d, B=%d, C=%d, D=%d, F=%d\n", gradeA, gradeB, gradeC, gradeD, gradeF);
}

/* -------------------- Export Report ---------------- */

void exportReport() {
    if (studentCount == 0) { printf("No records to export.\n"); return; }
    FILE *fp = fopen(REPORT_FILE, "w");
    if (!fp) { printf("Error: cannot write report.\n"); return; }

    fprintf(fp, "==============================================\n");
    fprintf(fp, "          Student Management Report           \n");
    fprintf(fp, "==============================================\n\n");

    fprintf(fp, "%-6s  %-25s  %-8s  %-8s  %-5s\n", "Roll", "Name", "Subjects", "Average", "Grade");
    fprintf(fp, "------  -------------------------  --------  --------  -----\n");
    for (int i = 0; i < studentCount; ++i) {
        fprintf(fp, "%-6d  %-25.25s  %-8d  %-8.2f  %-5c\n",
                students[i].roll, students[i].name, students[i].subjectCount, students[i].average, students[i].grade);
    }

    // stats
    float classSum = 0.0f; int topIdx = 0, lowIdx = 0;
    for (int i = 0; i < studentCount; ++i) {
        classSum += students[i].average;
        if (students[i].average > students[topIdx].average) topIdx = i;
        if (students[i].average < students[lowIdx].average) lowIdx = i;
    }
    float classAvg = classSum / studentCount;

    fprintf(fp, "\n--- Summary ---\n");
    fprintf(fp, "Total students : %d\n", studentCount);
    fprintf(fp, "Class average  : %.2f\n", classAvg);
    fprintf(fp, "Topper         : Roll %d (%s) Avg %.2f\n", students[topIdx].roll, students[topIdx].name, students[topIdx].average);
    fprintf(fp, "Lowest         : Roll %d (%s) Avg %.2f\n", students[lowIdx].roll, students[lowIdx].name, students[lowIdx].average);

    fclose(fp);
    printf("✅ Exported report to '%s'\n", REPORT_FILE);
}

/* ---------------------- Menu ----------------------- */

void menu() {
    while (1) {
        clearScreen();
        printBanner();
        printf("\n1) Add Student\n");
        printf("2) View All\n");
        printf("3) Search by Roll\n");
        printf("4) Search by Name\n");
        printf("5) Update Student\n");
        printf("6) Delete Student\n");
        printf("7) Sort Records\n");
        printf("8) Statistics\n");
        printf("9) Export Report\n");
        printf("0) Exit\n");

        int choice = inputIntInRange("\nChoose an option: ", 0, 9);
        clearScreen();
        switch (choice) {
            case 1: printBanner(); addStudent();        waitEnter(); break;
            case 2: printBanner(); listAll();           waitEnter(); break;
            case 3: printBanner(); searchByRoll();      waitEnter(); break;
            case 4: printBanner(); searchByName();      waitEnter(); break;
            case 5: printBanner(); updateStudent();     waitEnter(); break;
            case 6: printBanner(); deleteStudent();     waitEnter(); break;
            case 7: printBanner(); sortMenu();          waitEnter(); break;
            case 8: printBanner(); showStats();         waitEnter(); break;
            case 9: printBanner(); exportReport();      waitEnter(); break;
            case 0: printf("Saving & exiting... Bye!\n"); saveAll(); return;
        }
    }
}

int main(void) {
    loadAll();
    menu();
    return 0;
}