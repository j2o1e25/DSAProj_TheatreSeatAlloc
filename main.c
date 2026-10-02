#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TOTAL_SEATS 10

typedef struct Seat {
    int seatNo;
    int booked;
    char customerName[50];
    struct Seat *next;
} Seat;

Seat *createSeat(int seatNo) {
    Seat *newSeat = (Seat *)malloc(sizeof(Seat));
    if (newSeat == NULL) {
        printf("Memory allocation failed.\n");
        exit(1);
    }

    newSeat->seatNo = seatNo;
    newSeat->booked = 0;
    strcpy(newSeat->customerName, "Available");
    newSeat->next = NULL;
    return newSeat;
}

void insertSeat(Seat **head, int seatNo) {
    Seat *newSeat = createSeat(seatNo);
    if (*head == NULL) {
        *head = newSeat;
        return;
    }

    Seat *temp = *head;
    while (temp->next != NULL) {
        temp = temp->next;
    }
    temp->next = newSeat;
}

Seat *findSeat(Seat *head, int seatNo) {
    while (head != NULL) {
        if (head->seatNo == seatNo) {
            return head;
        }
        head = head->next;
    }
    return NULL;
}

void reserveSeat(Seat *head, int seatNo, const char *name) {
    Seat *seat = findSeat(head, seatNo);
    if (seat == NULL) {
        printf("Seat %d does not exist.\n", seatNo);
        return;
    }

    if (seat->booked) {
        printf("Seat %d is already allocated to %s.\n", seatNo, seat->customerName);
        return;
    }

    seat->booked = 1;
    strncpy(seat->customerName, name, sizeof(seat->customerName) - 1);
    seat->customerName[sizeof(seat->customerName) - 1] = '\0';
    printf("Seat %d booked successfully for %s.\n", seatNo, seat->customerName);
}

void cancelSeat(Seat *head, int seatNo) {
    Seat *seat = findSeat(head, seatNo);
    if (seat == NULL) {
        printf("Seat %d does not exist.\n", seatNo);
        return;
    }

    if (!seat->booked) {
        printf("Seat %d is already available.\n", seatNo);
        return;
    }

    seat->booked = 0;
    strcpy(seat->customerName, "Available");
    printf("Reservation for seat %d cancelled.\n", seatNo);
}

void displaySeats(Seat *head) {
    printf("\nTheatre Seat Layout:\n");
    printf("--------------------\n");

    while (head != NULL) {
        printf("Seat %d: %s\n",
               head->seatNo,
               head->booked ? head->customerName : "Available");
        head = head->next;
    }
    printf("--------------------\n");
}

void displayAvailableSeats(Seat *head) {
    int found = 0;
    printf("\nAvailable Seats: ");

    while (head != NULL) {
        if (!head->booked) {
            printf("%d ", head->seatNo);
            found = 1;
        }
        head = head->next;
    }

    if (!found) {
        printf("None");
    }
    printf("\n");
}

void freeSeats(Seat *head) {
    Seat *temp;
    while (head != NULL) {
        temp = head;
        head = head->next;
        free(temp);
    }
}

int main(void) {
    Seat *head = NULL;
    int choice, seatNo;
    char name[50];

    for (int i = 1; i <= TOTAL_SEATS; i++) {
        insertSeat(&head, i);
    }

    while (1) {
        printf("\nTheatre Seat Allocation System\n");
        printf("1. Display all seats\n");
        printf("2. Allocate a seat\n");
        printf("3. Cancel a reservation\n");
        printf("4. Display available seats\n");
        printf("5. Exit\n");
        printf("Enter your choice: ");
        scanf("%d", &choice);
        getchar();

        switch (choice) {
            case 1:
                displaySeats(head);
                break;

            case 2:
                printf("Enter seat number to allocate (1-%d): ", TOTAL_SEATS);
                scanf("%d", &seatNo);
                getchar();
                printf("Enter customer name: ");
                fgets(name, sizeof(name), stdin);
                name[strcspn(name, "\n")] = '\0';

                if (seatNo < 1 || seatNo > TOTAL_SEATS) {
                    printf("Invalid seat number. Please choose between 1 and %d.\n", TOTAL_SEATS);
                } else {
                    reserveSeat(head, seatNo, name);
                }
                break;

            case 3:
                printf("Enter seat number to cancel (1-%d): ", TOTAL_SEATS);
                scanf("%d", &seatNo);
                getchar();
                if (seatNo < 1 || seatNo > TOTAL_SEATS) {
                    printf("Invalid seat number. Please choose between 1 and %d.\n", TOTAL_SEATS);
                } else {
                    cancelSeat(head, seatNo);
                }
                break;

            case 4:
                displayAvailableSeats(head);
                break;

            case 5:
                freeSeats(head);
                printf("Program terminated.\n");
                return 0;

            default:
                printf("Invalid choice. Please select a valid option.\n");
                break;
        }
    }

    return 0;
}

