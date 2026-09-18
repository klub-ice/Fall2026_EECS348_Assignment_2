/*
EECS 348 Assignment 2
Author: Zoey Spies
KUID: 3136594
Creation Date: 9/17/2026
Revision date: 9/17/2026
Purpose: Sort emails in order of priorty with the ability to call the most important emails off the heap as needed
Collaborators: Copilot, Claude, EECS 348 notes and slides, EECS 388 notes and slides
*/

#include <stdio.h> // imports the stdio header
#include <stdlib.h> // imports the stdlib header
#include <string.h> // bring in the string header

/* ------------------------------------------------------------------ */
/* Data structures                                                     */
/* ------------------------------------------------------------------ */

#define MAX_LINE 512 // declares a MAX_LINE var and initializes to 512
#define MAX_FIELD 256 // declares a MAX_FIELD var and initializes to 256
#define INITIAL_CAPACITY 8 // declares a INITIAL_CAPACITY var and initializes to 8

/* One email in the inbox. */
typedef struct { // defines a struct called Email
    char category[32];      /* Sender category string, e.g. "Boss" */
    char subject[MAX_FIELD];/* Subject line (may contain spaces) */
    char date[16];           /* Date as given, MM-DD-YYYY */
    int  priority;           /* Numeric rank derived from category */
    long dateValue;           /* Date encoded as YYYYMMDD for comparison */
} Email; // defines a struct called Email

/* List-based (dynamic array) MaxHeap of Email structs. */
typedef struct { // defines a struct called MaxHeap
    Email *data;    /* Underlying resizable array */
    int size;       /* Number of elements currently stored */
    int capacity;   /* Allocated capacity of data[] */
} MaxHeap; // defines a struct called MaxHeap

/* ------------------------------------------------------------------ */
/* Helper: map a sender category string to a numeric priority rank.    */
/* Higher number = read first.                                         */
/* ------------------------------------------------------------------ */
int categoryPriority(const char *category) { // maps a sender category string to a numeric priority rank
    if (strcmp(category, "Boss") == 0) return 5; // Highest priority
    if (strcmp(category, "Subordinate") == 0) return 4; // Second highest priority
    if (strcmp(category, "Peer") == 0) return 3; // Third highest priority
    if (strcmp(category, "ImportantPerson") == 0) return 2; // Fourth highest priority
    if (strcmp(category, "OtherPerson") == 0) return 1; // Fifth highest priority
    return 0; /* Unknown category: lowest priority */
}

/* ------------------------------------------------------------------ */
/* Helper: convert "MM-DD-YYYY" into a comparable integer YYYYMMDD.    */
/* ------------------------------------------------------------------ */
long dateToValue(const char *date) {
    int month, day, year; // declares month, day, year as integers
    /* Parse the three numeric fields separated by '-' */
    if (sscanf(date, "%d-%d-%d", &month, &day, &year) != 3) {
        return 0; /* Malformed date: treat as earliest possible */
    }
    /* Combine into a single comparable number: YYYYMMDD */
    return ((long)year * 10000L) + (month * 100L) + day; // returns the date as a long integer in the format YYYYMMDD
}

/* ------------------------------------------------------------------ */
/* Comparison: returns 1 if email a has strictly higher read-priority  */
/* than email b, else 0. Used to decide heap ordering.                 */
/* ------------------------------------------------------------------ */
int isHigherPriority(const Email *a, const Email *b) {
    if (a->priority != b->priority) { // if the priority of a and b are not equal
        return a->priority > b->priority;   /* Category rank wins first */
    }
    return a->dateValue > b->dateValue;      /* Tie: newer date wins */
}

/* ------------------------------------------------------------------ */
/* MaxHeap: initialization                                             */
/* ------------------------------------------------------------------ */
void heapInit(MaxHeap *h) {
    h->capacity = INITIAL_CAPACITY; // sets the initial capacity of the heap to INITIAL_CAPACITY
    h->size = 0; // sets the initial size of the heap to 0
    h->data = (Email *)malloc(sizeof(Email) * h->capacity); // allocates memory for the heap data array
    if (h->data == NULL) { // checks if the memory allocation failed
        fprintf(stderr, "Fatal: memory allocation failed.\n"); // prints an error message to stderr
        exit(1); // exits the program
    }
}

/* ------------------------------------------------------------------ */
/* MaxHeap: grow the underlying array when it becomes full.            */
/* ------------------------------------------------------------------ */
void heapGrow(MaxHeap *h) { // grows the underlying array when it becomes full
    h->capacity *= 2; // doubles the capacity of the heap
    Email *newData = (Email *)realloc(h->data, sizeof(Email) * h->capacity); // reallocates memory for the heap data array with the new capacity
    if (newData == NULL) { // checks if the memory reallocation failed
        fprintf(stderr, "Fatal: memory reallocation failed.\n"); // prints an error message to stderr
        exit(1); // exits the program
    }
    h->data = newData; // sets the heap data array to the new location
}

/* ------------------------------------------------------------------ */
/* MaxHeap: swap two elements in the array by index.                   */
/* ------------------------------------------------------------------ */
void heapSwap(MaxHeap *h, int i, int j) {
    Email temp = h->data[i]; // stores the value of h->data[i] in a temporary variable
    h->data[i] = h->data[j]; // sets the value of h->data[i] to the value of h->data[j]
    h->data[j] = temp; // sets the value of h->data[j] to the value of the temporary variable
}

/* ------------------------------------------------------------------ */
/* MaxHeap: restore heap property upward from index i (used after      */
/* inserting a new element at the end of the array).                   */
/* ------------------------------------------------------------------ */
void siftUp(MaxHeap *h, int i) { // restores the heap property upward from index i
    while (i > 0) { // while the index is greater than 0
        int parent = (i - 1) / 2; // calculates the index of the parent node
        /* Stop once the parent already has higher-or-equal priority */
        if (!isHigherPriority(&h->data[i], &h->data[parent])) { // if the priority of the current node is not higher than the priority of the parent node
            break; // breaks out of the loop
        }
        heapSwap(h, i, parent); // swaps the current node with the parent node
        i = parent; // sets the index to the parent node
    }
}

/* ------------------------------------------------------------------ */
/* MaxHeap: restore heap property downward from index i (used after    */
/* moving the last element to the root during extraction).             */
/* ------------------------------------------------------------------ */
void siftDown(MaxHeap *h, int i) { // restores the heap index i
    while (1) { // infinite loop
        int left = 2 * i + 1; // calculates the index of the left child node
        int right = 2 * i + 2; // calculates the index of the right child node
        int largest = i; // initializes the largest index to the current index

        /* Find which of node i, its left child, and right child should
           be at the top of this subtree. */
        if (left < h->size && isHigherPriority(&h->data[left], &h->data[largest])) { // if the left child index is less than the size of the heap and the priority of the left child is higher than the priority of the current largest node
            largest = left; // sets the largest index to the left child index
        }
        if (right < h->size && isHigherPriority(&h->data[right], &h->data[largest])) { // if the right child index is less than the size of the heap and the priority of the right child is higher than the priority of the current largest node
            largest = right;
        }
        if (largest == i) {
            break; /* Heap property satisfied; done */
        }
        heapSwap(h, i, largest); // swaps the current node with the largest node
        i = largest; // sets the index to the largest node
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
void handleEmailCommand(MaxHeap *h, char *line) { // handles a line beginning with "EMAIL " by building an Email and inserting it into the heap
    char category[32], subject[MAX_FIELD], date[16]; // declares char arrays for category, subject, and date
    char *rest = line + 6; /* Skip past "EMAIL " */

    if (!parseEmailFields(rest, category, subject, date)) { // if the parseEmailFields function returns 0, indicating that the line is malformed
        fprintf(stderr, "Warning: malformed EMAIL line ignored: %s\n", line);
        return; // returns from the function
    }

    Email e; // declares an Email variable called e
    strncpy(e.category, category, sizeof(e.category) - 1);
    e.category[sizeof(e.category) - 1] = '\0';
    strncpy(e.subject, subject, sizeof(e.subject) - 1);
    e.subject[sizeof(e.subject) - 1] = '\0';
    strncpy(e.date, date, sizeof(e.date) - 1);
    e.date[sizeof(e.date) - 1] = '\0';
    e.priority = categoryPriority(e.category);
    e.dateValue = dateToValue(e.date);

    heapInsert(h, e); // inserts the email into the heap
}

/* Handle the "NEXT" command: display the top email without removing it. */
void handleNextCommand(MaxHeap *h) { // handles the NEXT command by displaying the top email without removing it
    Email *top = heapPeek(h); // gets a pointer to the top email in the heap
    if (top == NULL) { // if the heap is empty and there are no emails to read
        printf("No emails to read.\n"); // prints a message indicating that there are no emails to read
        return; // returns from the function
    }
    printf("Next email:\n"); // prints a message indicating that the next email is being displayed
    printf("Sender: %s\n", top->category); // prints the sender of the top email
    printf("Subject: %s\n", top->subject); // prints the subject of the top email
    printf("Date: %s\n", top->date); // prints the date of the top email
}

/* Handle the "READ" command: remove the top email (no display). */
void handleReadCommand(MaxHeap *h) { // handles the READ command by removing the top email from the heap
    if (!heapExtractMax(h)) { // if the heap is empty and there are no emails to read
        printf("No emails to read.\n"); // prints a message indicating that there are no emails to read
    }
}

/* Handle the "COUNT" command: print how many unread emails remain. */
void handleCountCommand(MaxHeap *h) { // handles the COUNT command by printing the number of unread emails remaining in the heap
    printf("There are %d emails to read.\n", h->size); // prints the number of unread emails remaining in the heap
}

/* ------------------------------------------------------------------ */
/* Main: read commands from a file (argv[1]) or stdin, dispatch each.  */
/* ------------------------------------------------------------------ */
int main(int argc, char *argv[]) { // main function that reads commands from a file or stdin and dispatches them
    FILE *input = stdin; // initializes input to stdin

    if (argc > 1) { // if there is a command line argument
        input = fopen(argv[1], "r"); // opens the file specified in the command line argument for reading
        if (input == NULL) { // if the file could not be opened
            fprintf(stderr, "Error: could not open file %s\n", argv[1]); // prints an error message to stderr
            return 1; // returns 1 to indicate an error
        }
    }

    MaxHeap heap; // declares a MaxHeap variable called heap
    heapInit(&heap); // initializes the heap

    char line[MAX_LINE]; // declares a char array called line with size MAX_LINE
    while (fgets(line, sizeof(line), input) != NULL) { // reads a line from the input file or stdin
        /* Strip trailing newline/carriage return for clean comparisons */
        size_t len = strlen(line); // gets the length of the line
        while (len > 0 && (line[len - 1] == '\n' || line[len - 1] == '\r')) { // while the last character of the line is a newline or carriage return
            line[--len] = '\0'; // replace the last character with a null terminator
        }
        if (len == 0) continue; /* Skip blank lines */

        if (strncmp(line, "EMAIL ", 6) == 0) { // if the line starts with "EMAIL "
            handleEmailCommand(&heap, line); // handles the EMAIL command
        } else if (strcmp(line, "NEXT") == 0) { // if the line is "NEXT"
            handleNextCommand(&heap);
        } else if (strcmp(line, "READ") == 0) { // if the line is "READ"
            handleReadCommand(&heap);
        } else if (strcmp(line, "COUNT") == 0) { // if the line is "COUNT"
            handleCountCommand(&heap);
        } else { // if the line is unrecognized
            fprintf(stderr, "Warning: unrecognized command ignored: %s\n", line); // prints a warning message to stderr
        }
    }

    if (input != stdin) { // if the input is not stdin
        fclose(input); // closes the input file
    }
    heapFree(&heap); // frees the memory allocated for the heap
    return 0; // returns 0 to indicate successful execution
}
