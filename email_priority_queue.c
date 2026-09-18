/*
EECS 348 Assignment 2
Author: Zoey Spies
KUID: 3136594
Creation Date: 9/17/2026
Revision date: 9/17/2026
Purpose: Sort emails in order of priorty with the ability to call the most important emails off the heap as needed
Collaborators: Copilot, Claude, EECS 348 notes and slides, EECS 388 notes and slides
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ------------------------------------------------------------------ */
/* Data structures                                                     */
/* ------------------------------------------------------------------ */

#define MAX_LINE 512
#define MAX_FIELD 256
#define INITIAL_CAPACITY 8

/* One email in the inbox. */
typedef struct {
    char category[32];      /* Sender category string, e.g. "Boss" */
    char subject[MAX_FIELD];/* Subject line (may contain spaces) */
    char date[16];           /* Date as given, MM-DD-YYYY */
    int  priority;           /* Numeric rank derived from category */
    long dateValue;           /* Date encoded as YYYYMMDD for comparison */
} Email;

/* List-based (dynamic array) MaxHeap of Email structs. */
typedef struct {
    Email *data;    /* Underlying resizable array */
    int size;       /* Number of elements currently stored */
    int capacity;   /* Allocated capacity of data[] */
} MaxHeap;

/* ------------------------------------------------------------------ */
/* Helper: map a sender category string to a numeric priority rank.    */
/* Higher number = read first.                                         */
/* ------------------------------------------------------------------ */
int categoryPriority(const char *category) {
    if (strcmp(category, "Boss") == 0) return 5;
    if (strcmp(category, "Subordinate") == 0) return 4;
    if (strcmp(category, "Peer") == 0) return 3;
    if (strcmp(category, "ImportantPerson") == 0) return 2;
    if (strcmp(category, "OtherPerson") == 0) return 1;
    return 0; /* Unknown category: lowest priority */
}

/* ------------------------------------------------------------------ */
/* Helper: convert "MM-DD-YYYY" into a comparable integer YYYYMMDD.    */
/* ------------------------------------------------------------------ */
long dateToValue(const char *date) {
    int month, day, year;
    /* Parse the three numeric fields separated by '-' */
    if (sscanf(date, "%d-%d-%d", &month, &day, &year) != 3) {
        return 0; /* Malformed date: treat as earliest possible */
    }
    /* Combine into a single comparable number: YYYYMMDD */
    return ((long)year * 10000L) + (month * 100L) + day;
}

/* ------------------------------------------------------------------ */
/* Comparison: returns 1 if email a has strictly higher read-priority  */
/* than email b, else 0. Used to decide heap ordering.                 */
/* ------------------------------------------------------------------ */
int isHigherPriority(const Email *a, const Email *b) {
    if (a->priority != b->priority) {
        return a->priority > b->priority;   /* Category rank wins first */
    }
    return a->dateValue > b->dateValue;      /* Tie: newer date wins */
}

/* ------------------------------------------------------------------ */
/* MaxHeap: initialization                                             */
/* ------------------------------------------------------------------ */
void heapInit(MaxHeap *h) {
    h->capacity = INITIAL_CAPACITY;
    h->size = 0;
    h->data = (Email *)malloc(sizeof(Email) * h->capacity);
    if (h->data == NULL) {
        fprintf(stderr, "Fatal: memory allocation failed.\n");
        exit(1);
    }
}

/* ------------------------------------------------------------------ */
/* MaxHeap: grow the underlying array when it becomes full.            */
/* ------------------------------------------------------------------ */
void heapGrow(MaxHeap *h) {
    h->capacity *= 2;
    Email *newData = (Email *)realloc(h->data, sizeof(Email) * h->capacity);
    if (newData == NULL) {
        fprintf(stderr, "Fatal: memory reallocation failed.\n");
        exit(1);
    }
    h->data = newData;
}

/* ------------------------------------------------------------------ */
/* MaxHeap: swap two elements in the array by index.                   */
/* ------------------------------------------------------------------ */
void heapSwap(MaxHeap *h, int i, int j) {
    Email temp = h->data[i];
    h->data[i] = h->data[j];
    h->data[j] = temp;
}

/* ------------------------------------------------------------------ */
/* MaxHeap: restore heap property upward from index i (used after      */
/* inserting a new element at the end of the array).                   */
/* ------------------------------------------------------------------ */
void siftUp(MaxHeap *h, int i) {
    while (i > 0) {
        int parent = (i - 1) / 2;
        /* Stop once the parent already has higher-or-equal priority */
        if (!isHigherPriority(&h->data[i], &h->data[parent])) {
            break;
        }
        heapSwap(h, i, parent);
        i = parent;
    }
}

/* ------------------------------------------------------------------ */
/* MaxHeap: restore heap property downward from index i (used after    */
/* moving the last element to the root during extraction).             */
/* ------------------------------------------------------------------ */
void siftDown(MaxHeap *h, int i) {
    while (1) {
        int left = 2 * i + 1;
        int right = 2 * i + 2;
        int largest = i;

        /* Find which of node i, its left child, and right child should
           be at the top of this subtree. */
        if (left < h->size && isHigherPriority(&h->data[left], &h->data[largest])) {
            largest = left;
        }
        if (right < h->size && isHigherPriority(&h->data[right], &h->data[largest])) {
            largest = right;
        }
        if (largest == i) {
            break; /* Heap property satisfied; done */
        }
        heapSwap(h, i, largest);
        i = largest;
    }
}

/* ------------------------------------------------------------------ */
/* MaxHeap: insert a new email, growing the array if necessary.        */
/* ------------------------------------------------------------------ */
void heapInsert(MaxHeap *h, Email e) {
    if (h->size == h->capacity) {
        heapGrow(h);
    }
    h->data[h->size] = e;   /* Place new element at the end */
    siftUp(h, h->size);      /* Restore heap property */
    h->size++;
}

/* ------------------------------------------------------------------ */
/* MaxHeap: return a pointer to the highest-priority email without     */
/* removing it. Returns NULL if the heap is empty.                     */
/* ------------------------------------------------------------------ */
Email *heapPeek(MaxHeap *h) {
    if (h->size == 0) {
        return NULL;
    }
    return &h->data[0];
}

/* ------------------------------------------------------------------ */
/* MaxHeap: remove and discard the highest-priority email.             */
/* Returns 1 on success, 0 if the heap was already empty.              */
/* ------------------------------------------------------------------ */
int heapExtractMax(MaxHeap *h) {
    if (h->size == 0) {
        return 0;
    }
    h->size--;
    h->data[0] = h->data[h->size]; /* Move last element to root */
    if (h->size > 0) {
        siftDown(h, 0);              /* Restore heap property */
    }
    return 1;
}

/* ------------------------------------------------------------------ */
/* MaxHeap: free all allocated memory.                                  */
/* ------------------------------------------------------------------ */
void heapFree(MaxHeap *h) {
    free(h->data);
    h->data = NULL;
    h->size = 0;
    h->capacity = 0;
}

/* ------------------------------------------------------------------ */
/* Parsing helper: split "category,subject,date" into three fields.    */
/* Subject may contain spaces but not commas, so a simple split on the */
/* first two commas is sufficient. Returns 1 on success, 0 on failure. */
/* ------------------------------------------------------------------ */
int parseEmailFields(char *rest, char *category, char *subject, char *date) {
    char *firstComma = strchr(rest, ',');
    if (firstComma == NULL) return 0;
    char *secondComma = strchr(firstComma + 1, ',');
    if (secondComma == NULL) return 0;

    /* Copy category: from start of rest up to firstComma */
    int catLen = (int)(firstComma - rest);
    strncpy(category, rest, catLen);
    category[catLen] = '\0';

    /* Copy subject: between the two commas */
    int subLen = (int)(secondComma - firstComma - 1);
    strncpy(subject, firstComma + 1, subLen);
    subject[subLen] = '\0';

    /* Copy date: everything after the second comma */
    strcpy(date, secondComma + 1);

    /* Trim a trailing newline/carriage return from the date field */
    size_t len = strlen(date);
    while (len > 0 && (date[len - 1] == '\n' || date[len - 1] == '\r')) {
        date[--len] = '\0';
    }
    return 1;
}

/* ------------------------------------------------------------------ */
/* Command handling                                                     */
/* ------------------------------------------------------------------ */

/* Handle a line beginning with "EMAIL ": build an Email and insert it. */
void handleEmailCommand(MaxHeap *h, char *line) {
    char category[32], subject[MAX_FIELD], date[16];
    char *rest = line + 6; /* Skip past "EMAIL " */

    if (!parseEmailFields(rest, category, subject, date)) {
        fprintf(stderr, "Warning: malformed EMAIL line ignored: %s\n", line);
        return;
    }

    Email e;
    strncpy(e.category, category, sizeof(e.category) - 1);
    e.category[sizeof(e.category) - 1] = '\0';
    strncpy(e.subject, subject, sizeof(e.subject) - 1);
    e.subject[sizeof(e.subject) - 1] = '\0';
    strncpy(e.date, date, sizeof(e.date) - 1);
    e.date[sizeof(e.date) - 1] = '\0';
    e.priority = categoryPriority(e.category);
    e.dateValue = dateToValue(e.date);

    heapInsert(h, e);
}

/* Handle the "NEXT" command: display the top email without removing it. */
void handleNextCommand(MaxHeap *h) {
    Email *top = heapPeek(h);
    if (top == NULL) {
        printf("No emails to read.\n");
        return;
    }
    printf("Next email:\n");
    printf("Sender: %s\n", top->category);
    printf("Subject: %s\n", top->subject);
    printf("Date: %s\n", top->date);
}

/* Handle the "READ" command: remove the top email (no display). */
void handleReadCommand(MaxHeap *h) {
    if (!heapExtractMax(h)) {
        printf("No emails to read.\n");
    }
}

/* Handle the "COUNT" command: print how many unread emails remain. */
void handleCountCommand(MaxHeap *h) {
    printf("There are %d emails to read.\n", h->size);
}

/* ------------------------------------------------------------------ */
/* Main: read commands from a file (argv[1]) or stdin, dispatch each.  */
/* ------------------------------------------------------------------ */
int main(int argc, char *argv[]) {
    FILE *input = stdin;

    if (argc > 1) {
        input = fopen(argv[1], "r");
        if (input == NULL) {
            fprintf(stderr, "Error: could not open file %s\n", argv[1]);
            return 1;
        }
    }

    MaxHeap heap;
    heapInit(&heap);

    char line[MAX_LINE];
    while (fgets(line, sizeof(line), input) != NULL) {
        /* Strip trailing newline/carriage return for clean comparisons */
        size_t len = strlen(line);
        while (len > 0 && (line[len - 1] == '\n' || line[len - 1] == '\r')) {
            line[--len] = '\0';
        }
        if (len == 0) continue; /* Skip blank lines */

        if (strncmp(line, "EMAIL ", 6) == 0) {
            handleEmailCommand(&heap, line);
        } else if (strcmp(line, "NEXT") == 0) {
            handleNextCommand(&heap);
        } else if (strcmp(line, "READ") == 0) {
            handleReadCommand(&heap);
        } else if (strcmp(line, "COUNT") == 0) {
            handleCountCommand(&heap);
        } else {
            fprintf(stderr, "Warning: unrecognized command ignored: %s\n", line);
        }
    }

    if (input != stdin) {
        fclose(input);
    }
    heapFree(&heap);
    return 0;
}
