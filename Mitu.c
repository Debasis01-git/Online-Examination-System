//CUTM  All-in-one Online Exam System 

//MADE BY : DEBASIS MALLICK
//REGISTRATION NO : 250301370205

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

#ifdef _WIN32
  #include <conio.h>
  #include <windows.h>
  #define sleep_ms(ms) Sleep(ms)
#else
  #include <sys/select.h>
  #include <unistd.h>
  #include <limits.h>
  #include <sys/stat.h>
  #define sleep_ms(ms) usleep((ms)*1000)
#endif

#define MAX_Q 1000
#define LINE_SZ 512
#define ADMIN_CONF "admin.conf"
#define QUESTIONS_FILE "questions.txt"

typedef struct {
    char q[512];
    char A[256], B[256], C[256], D[256];
    char ans; 
} MCQ;

typedef struct {
    int bank_index;   
    int order[4];     
    int correct_shown; 
    char user_choice;  
    double mark_awarded;
} PresentedQ;

static MCQ bank[MAX_Q];
static int qcount = 0;

static void rtrim(char *s) {
    size_t n = strlen(s);
    while (n && (s[n-1]=='\n' || s[n-1]=='\r' || s[n-1]==' ' || s[n-1]=='\t')) {
        s[n-1] = '\0'; n--;
    }
}

int load_questions(const char *filename) {
    FILE *fp = fopen(filename, "r");
    if (!fp) return 0;
    char line[LINE_SZ];
    MCQ curr;
    memset(&curr,0,sizeof(curr));
    int in_block = 0;
    int idx = 0;
    while (fgets(line, sizeof(line), fp)) {
        rtrim(line);
        if (line[0] == '\0') {
            if (in_block && curr.q[0] && curr.ans && idx < MAX_Q) {
                bank[idx++] = curr;
            }
            memset(&curr,0,sizeof(curr));
            in_block = 0;
            continue;
        }
        char *p = line;
        while (*p==' '||*p=='\t') p++;
        if (p[0]=='Q' && (p[1]==' '||p[1]=='\t')) strncpy(curr.q, p+2, sizeof(curr.q)-1);
        else if (p[0]=='A' && (p[1]==' '||p[1]=='\t')) strncpy(curr.A, p+2, sizeof(curr.A)-1);
        else if (p[0]=='B' && (p[1]==' '||p[1]=='\t')) strncpy(curr.B, p+2, sizeof(curr.B)-1);
        else if (p[0]=='C' && (p[1]==' '||p[1]=='\t')) strncpy(curr.C, p+2, sizeof(curr.C)-1);
        else if (p[0]=='D' && (p[1]==' '||p[1]=='\t')) strncpy(curr.D, p+2, sizeof(curr.D)-1);
        else if (strncmp(p,"ANS",3)==0 && (p[3]==' '||p[3]=='\t')) {
            char *qchar = p+4;
            while (*qchar==' '||*qchar=='\t') qchar++;
            if (*qchar) curr.ans = (char)toupper((unsigned char)*qchar);
        }
        in_block = 1;
    }
    if (in_block && curr.q[0] && curr.ans && idx < MAX_Q) bank[idx++] = curr;
    fclose(fp);
    qcount = idx;
    return idx;
}

int write_all_questions(const char *filename) {
    FILE *fp = fopen(filename, "w");
    if (!fp) return 0;
    for (int i=0;i<qcount;i++) {
        fprintf(fp, "Q %s\n", bank[i].q);
        fprintf(fp, "A %s\n", bank[i].A);
        fprintf(fp, "B %s\n", bank[i].B);
        fprintf(fp, "C %s\n", bank[i].C);
        fprintf(fp, "D %s\n", bank[i].D);
        fprintf(fp, "ANS %c\n\n", bank[i].ans);
    }
    fclose(fp);
    return 1;
}

int append_question_to_file(const char *filename, MCQ *m) {
    FILE *fp = fopen(filename, "a");
    if (!fp) return 0;
    fprintf(fp, "Q %s\n", m->q);
    fprintf(fp, "A %s\n", m->A);
    fprintf(fp, "B %s\n", m->B);
    fprintf(fp, "C %s\n", m->C);
    fprintf(fp, "D %s\n", m->D);
    fprintf(fp, "ANS %c\n\n", m->ans);
    fclose(fp);
    return 1;
}

/* Admin password management */
int read_admin_password(char *out, size_t n) {
    FILE *f = fopen(ADMIN_CONF, "r");
    if (!f) {
        /* default password */
        strncpy(out, "admin", n-1);
        out[n-1] = '\0';
        return 1;
    }
    if (!fgets(out, (int)n, f)) { fclose(f); return 0; }
    rtrim(out);
    fclose(f);
    return 1;
}

int save_admin_password(const char *pass) {
    FILE *f = fopen(ADMIN_CONF, "w");
    if (!f) return 0;
    fprintf(f, "%s\n", pass);
    fclose(f);
    return 1;
}

void shuffle_int(int *a, int n) {
    for (int i = n-1; i > 0; --i) {
        int j = rand() % (i+1);
        int t = a[i]; a[i] = a[j]; a[j] = t;
    }
}

char read_answer_timed(int timeout_seconds) {
#ifdef _WIN32
    DWORD start = GetTickCount();
    DWORD timeout_ms = (DWORD)timeout_seconds * 1000;
    while (GetTickCount() - start < timeout_ms) {
        if (_kbhit()) {
            int ch = _getch();
            if (ch == 0 || ch == 0xE0) {int ch2 = _getch(); (void)ch2; continue; }
            if (ch == '\r') { putchar('\n'); continue; }
            putchar((char)ch); putchar('\n');
            char c = (char)toupper((unsigned char)ch);
            if (c >= 'A' && c <= 'D') return c;
            else return '-';
        }
        Sleep(50);
    }
    return '-';
#else
    fd_set set;
    struct timeval tv;
    FD_ZERO(&set);
    FD_SET(STDIN_FILENO, &set);
    tv.tv_sec = timeout_seconds;
    tv.tv_usec = 0;
    int rv = select(STDIN_FILENO+1, &set, NULL, NULL, &tv);
    if (rv <= 0) return '-';
    char buf[64];
    if (!fgets(buf, sizeof(buf), stdin)) return '-';
    char *p = buf;
    while (*p && isspace((unsigned char)*p)) p++;
    if (!*p) return '-';
    char c = (char)toupper((unsigned char)*p);
    if (c >= 'A' && c <= 'D') return c;
    return '-';
#endif
}

void timestamp_str(char *out, size_t n) {
    time_t t = time(NULL);
    struct tm *tm = localtime(&t);
    strftime(out, n, "%Y%m%d_%H%M%S", tm);
}

int save_results_file(const char *dirname, const PresentedQ *presented, int count, double total_score, double max_score, int time_limit, int shuffle_opts) {
    (void)dirname;
    char ts[64]; timestamp_str(ts, sizeof(ts));
    char fname[256];
    snprintf(fname, sizeof(fname), "results_%s.txt", ts);
    FILE *f = fopen(fname, "w");
    if (!f) return 0;
    fprintf(f, "Exam Results - %s\n", ts);
    fprintf(f, "Questions presented: %d\nTime per question: %d\nShuffle options: %s\n\n", count, time_limit, shuffle_opts ? "Yes":"No");
    for (int i=0;i<count;i++) {
        const PresentedQ *p = &presented[i];
        MCQ *m = &bank[p->bank_index];
        char your = p->user_choice ? p->user_choice : '-';
        char correct_letter = 'A' + p->correct_shown;
        const char *chosen_txt = "(none)";
        if (your >= 'A' && your <= 'D') {
            int idx = your - 'A'; 
            int src = p->order[idx];
            if (src==0) chosen_txt = m->A;
            else if (src==1) chosen_txt = m->B;
            else if (src==2) chosen_txt = m->C;
            else if (src==3) chosen_txt = m->D;
        }
        const char *correct_txt = NULL;
        {
            int src = p->order[p->correct_shown];
            if (src==0) correct_txt = m->A;
            else if (src==1) correct_txt = m->B;
            else if (src==2) correct_txt = m->C;
            else if (src==3) correct_txt = m->D;
        }
        fprintf(f, "Q%d: %s\n", i+1, m->q);
        fprintf(f, "Your: %c  (%s)\n", your, chosen_txt);
        fprintf(f, "Correct: %c  (%s)\n", correct_letter + '0' - '0', correct_txt);
        fprintf(f, "Mark awarded: %.2f\n\n", p->mark_awarded);
    }
    fprintf(f, "Total Score: %.2f / %.2f\n", total_score, max_score);
    fclose(f);
    return 1;
}

void admin_add_question_interactive() {
    MCQ m;
    memset(&m,0,sizeof(m));
    char buf[512];
    printf("\n--- Add new question ---\n");
    printf("Enter question text: ");
    fflush(stdout);
    if (!fgets(buf, sizeof(buf), stdin)) return;
    rtrim(buf); strncpy(m.q, buf, sizeof(m.q)-1);

    printf("Option A: "); if (!fgets(buf, sizeof(buf), stdin)) return; rtrim(buf); strncpy(m.A, buf, sizeof(m.A)-1);
    printf("Option B: "); if (!fgets(buf, sizeof(buf), stdin)) return; rtrim(buf); strncpy(m.B, buf, sizeof(m.B)-1);
    printf("Option C: "); if (!fgets(buf, sizeof(buf), stdin)) return; rtrim(buf); strncpy(m.C, buf, sizeof(m.C)-1);
    printf("Option D: "); if (!fgets(buf, sizeof(buf), stdin)) return; rtrim(buf); strncpy(m.D, buf, sizeof(m.D)-1);

    char ansline[32];
    printf("Correct option letter (A/B/C/D): ");
    if (!fgets(ansline, sizeof(ansline), stdin)) return;
    rtrim(ansline);
    char ch = (char)toupper((unsigned char)ansline[0]);
    if (!(ch>='A' && ch<='D')) { printf("Invalid answer. Aborting.\n"); return; }
    m.ans = ch;

    if (!append_question_to_file(QUESTIONS_FILE, &m)) {
        printf("Failed to append to %s\n", QUESTIONS_FILE);
    } else {
        printf("Question added successfully.\n");
        load_questions(QUESTIONS_FILE);
    }
}

void admin_list_questions() {
    load_questions(QUESTIONS_FILE);
    printf("\n--- Question Bank (%d) ---\n", qcount);
    for (int i=0;i<qcount;i++) {
        printf("%d) %s [ANS: %c]\n", i+1, bank[i].q, bank[i].ans);
    }
    printf("-------------------------\n");
}

void admin_delete_question() {
    load_questions(QUESTIONS_FILE);
    admin_list_questions();
    printf("Enter the question number to delete (0 to cancel): ");
    int num = 0;
    if (scanf("%d", &num) != 1) { while(getchar()!= '\n'); return; }
    while (getchar() != '\n');
    if (num <= 0 || num > qcount) { printf("Cancelled or invalid index.\n"); return; }
    int idx = num - 1;
    for (int i=idx;i<qcount-1;i++) bank[i] = bank[i+1];
    qcount--;
    if (!write_all_questions(QUESTIONS_FILE)) {
        printf("Failed to write file.\n");
    } else {
        printf("Deleted question %d and updated %s.\n", num, QUESTIONS_FILE);
    }
}

void admin_menu() {
    char saved_pass[128];
    read_admin_password(saved_pass, sizeof(saved_pass));
    char input[128];

    printf("\nEnter admin password: ");
    if (!fgets(input, sizeof(input), stdin)) return;
    rtrim(input);
    if (strcmp(input, saved_pass) != 0) {
        printf("Wrong password.\n"); return;
    }
    int choice = -1;
    do {
        printf("\n--- Admin Menu ---\n");
        printf("1. List questions\n2. Add question\n3. Delete question\n4. Change admin password\n0. Exit admin\nChoice: ");
        if (scanf("%d", &choice) != 1) { while(getchar()!= '\n'); break; }
        while (getchar() != '\n'); 
        if (choice == 1) admin_list_questions();
        else if (choice == 2) admin_add_question_interactive();
        else if (choice == 3) admin_delete_question();
        else if (choice == 4) {
            char np[128];
            printf("Enter new admin password: ");
            if (!fgets(np, sizeof(np), stdin)) continue;
            rtrim(np); if (save_admin_password(np)) printf("Password changed.\n"); else printf("Failed to save password.\n");
        }
    } while (choice != 0);
}

void take_exam_interactive() {
    if (!load_questions(QUESTIONS_FILE)) {
        printf("No questions found. Put questions in %s\n", QUESTIONS_FILE);
        return;
    }
    if (qcount == 0) {
        printf("Question bank empty.\n"); return;
    }

    int shuffle_questions = 1;
    int shuffle_options = 1;
    int time_limit = 15;
    double mark_correct = 1.0;
    double mark_wrong = 0.0;

    char line[64];
    printf("\n--- Exam Settings ---\n");
    printf("Shuffle questions? (1=yes,0=no) [default 1]: ");
    if (!fgets(line,sizeof(line),stdin)) return;
    if (line[0] == '0') shuffle_questions = 0;
    printf("Shuffle options? (1=yes,0=no) [default 1]: ");
    if (!fgets(line,sizeof(line),stdin)) return;
    if (line[0] == '0') shuffle_options = 0;
    printf("Time per question (seconds) [default 15]: ");
    if (!fgets(line,sizeof(line),stdin)) return;
    int ttmp = atoi(line); if (ttmp > 0) time_limit = ttmp;
    printf("Mark for correct answer (float) [default 1.0]: ");
    if (!fgets(line,sizeof(line),stdin)) return;
    double dc = atof(line); if (dc != 0.0) mark_correct = dc;
    printf("Mark for wrong answer (use negative for penalty) [default 0.0]: ");
    if (!fgets(line,sizeof(line),stdin)) return;
    double dw = atof(line); mark_wrong = dw;

    int order_arr[MAX_Q];
    for (int i=0;i<qcount;i++) order_arr[i] = i;
    if (shuffle_questions) shuffle_int(order_arr, qcount);

    PresentedQ presented[MAX_Q];
    memset(presented, 0, sizeof(presented));

    printf("\nExam starting... Good luck!\n\n");
    int presented_count = 0;
    for (int pi=0; pi<qcount; ++pi) {
        int bank_idx = order_arr[pi];
        MCQ *m = &bank[bank_idx];

        int ord[4] = {0,1,2,3};
        if (shuffle_options) shuffle_int(ord, 4);

        int correct_shown = -1;
        for (int k=0;k<4;k++) {
            int src = ord[k];
            char src_letter = (src==0)?'A':(src==1)?'B':(src==2)?'C':'D';
            if (src_letter == m->ans) correct_shown = k;
        }

        presented[presented_count].bank_index = bank_idx;
        for (int k=0;k<4;k++) presented[presented_count].order[k] = ord[k];
        presented[presented_count].correct_shown = correct_shown;
        presented[presented_count].user_choice = '-';
        presented[presented_count].mark_awarded = 0.0;

        printf("Q%d: %s\n", presented_count+1, m->q);
        for (int k=0;k<4;k++) {
            int src = ord[k];
            const char *txt = (src==0)?m->A:(src==1)?m->B:(src==2)?m->C:m->D;
            printf("%c. %s\n", 'A'+k, txt);
        }
        printf("Answer (A-D). You have %d seconds: ", time_limit);
        fflush(stdout);

        char ans = read_answer_timed(time_limit);
        if (ans == '-') {
            printf("\nTime up! (no answer)\n");
            presented[presented_count].user_choice = '-';
            presented[presented_count].mark_awarded = 0.0;
        } else {
            presented[presented_count].user_choice = ans;
            int chosen_idx = ans - 'A';
            if (chosen_idx >=0 && chosen_idx <=3) {
                if (chosen_idx == correct_shown) {
                    presented[presented_count].mark_awarded = mark_correct;
                    printf("Correct!\n");
                } else {
                    presented[presented_count].mark_awarded = mark_wrong;
                    printf("Wrong. Correct was: %c\n", 'A' + correct_shown);
                }
            } else {
                presented[presented_count].user_choice = '-';
                presented[presented_count].mark_awarded = 0.0;
            }
        }
        printf("\n");
        presented_count++;
        sleep_ms(200); 
    }

    double total = 0.0, max_possible = 0.0;
    for (int i=0;i<presented_count;i++) {
        total += presented[i].mark_awarded;
        max_possible += mark_correct;
    }

    printf("\n========== EXAM SUMMARY ==========\n");
    for (int i=0;i<presented_count;i++) {
        PresentedQ *p = &presented[i];
        MCQ *m = &bank[p->bank_index];

        char your = p->user_choice ? p->user_choice : '-';
        const char *yourtxt = "(none)";
        if (your >= 'A' && your <= 'D') {
            int idx = your - 'A';
            int src = p->order[idx];
            yourtxt = (src==0)?m->A:(src==1)?m->B:(src==2)?m->C:m->D;
        }
        int src_correct = p->order[p->correct_shown];
        const char *correcttxt = (src_correct==0)?m->A:(src_correct==1)?m->B:(src_correct==2)?m->C:m->D;
        char correct_letter = 'A' + p->correct_shown;
        printf("Q%d: Your: %c (%s) | Correct: %c (%s) | Mark: %.2f\n",
               i+1,
               (your=='-'?'-':your),
               yourtxt,
               correct_letter,
               correcttxt,
               p->mark_awarded);
    }
    printf("-----------------------------------\n");
    printf("Total: %.2f / %.2f\n", total, max_possible);
    printf("===================================\n");

    printf("Save results to file? (y/n): ");
    char sline[8];
    if (!fgets(sline, sizeof(sline), stdin)) return;
    if (sline[0]=='y' || sline[0]=='Y') {
        char ts[64]; timestamp_str(ts, sizeof(ts));
        char fname[256]; snprintf(fname, sizeof(fname), "results_%s.txt", ts);
        FILE *f = fopen(fname, "w");
        if (!f) { printf("Failed to create file.\n"); return; }
        fprintf(f, "Exam Results - %s\n\n", ts);
        fprintf(f, "Time per q: %d  Shuffle options: %s\n\n", time_limit, shuffle_options ? "Yes":"No");
        for (int i=0;i<presented_count;i++) {
            PresentedQ *p = &presented[i];
            MCQ *m = &bank[p->bank_index];
            char your = p->user_choice ? p->user_choice : '-';
            const char *yourtxt = "(none)";
            if (your >= 'A' && your <= 'D') {
                int idx = your - 'A';
                int src = p->order[idx];
                yourtxt = (src==0)?m->A:(src==1)?m->B:(src==2)?m->C:m->D;
            }
            int src_correct = p->order[p->correct_shown];
            const char *correcttxt = (src_correct==0)?m->A:(src_correct==1)?m->B:(src_correct==2)?m->C:m->D;
            char correct_letter = 'A' + p->correct_shown;
            fprintf(f, "Q%d: %s\nYour: %c (%s)\nCorrect: %c (%s)\nMark: %.2f\n\n",
                    i+1, m->q,
                    (your=='-'?'-':your), yourtxt,
                    correct_letter, correcttxt, p->mark_awarded);
        }
        fprintf(f, "Total: %.2f / %.2f\n", total, max_possible);
        fclose(f);
        printf("Saved to %s\n", fname);
    }
}

int main(void) {
    srand((unsigned int)time(NULL));
    int choice = -1;
    do {
        printf("\n===== ONLINE EXAM SYSTEM =====\n");
        printf("1. Take Exam\n2. Admin Panel\n0. Exit\nChoice: ");
        if (scanf("%d", &choice) != 1) { while (getchar()!= '\n'); break; }
        while (getchar() != '\n');
        if (choice == 1) take_exam_interactive();
        else if (choice == 2) admin_menu();
        else if (choice == 0) { printf("Goodbye.\n"); }
        else printf("Invalid choice.\n");
    } while (choice != 0);
    return 0;
}