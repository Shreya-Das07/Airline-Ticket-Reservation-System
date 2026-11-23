#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
enum {
    MAX_USERS = 200,
    MAX_BOOKINGS = 3000,
    MAX_FLIGHTS = 100,
    MAX_PASSENGERS = 6,
    SEATS_PER_DEMO_FLIGHT = 24,
    AIRPORT_COUNT = 8,
    UI_WIDTH = 72
};

static const char USER_FILE[] = "users_v2.dat";
static const char BOOKING_FILE[] = "bookings_v2.dat";
static const char FLIGHT_FILE[] = "custom_flights_v2.dat";
static const char ADMIN_USER[] = "admin";
static const char ADMIN_PASS[] = "admin123";
static const double SERVICE_RATE = 0.05;
static const double CHILD_RATE = 0.75;

typedef struct {
    char name[60];
    char username[30];
    char password[30];
    char phone[20];
} User;

typedef struct {
    char name[50];
    int age;
    char type[10];
    int seatNo;
    double fare;
} Passenger;

typedef struct {
    long bookingId;
    char username[30];
    char flightNo[20];
    char fromCode[8];
    char fromCity[60];
    char toCode[8];
    char toCity[60];
    char travelDate[11];
    char departure[6];
    char arrival[6];
    Passenger passengers[MAX_PASSENGERS];
    int passengerCount;
    double subTotal;
    double serviceCharge;
    double totalFare;
    char paymentMethod[24];
    char paymentStatus[24];
    int active;
    char bookingDate[11];
} Booking;

typedef struct {
    char flightNo[20];
    int fromIndex;
    int toIndex;
    char departure[6];
    char arrival[6];
    int totalSeats;
    double baseFare;
    int active;
} Flight;

typedef struct {
    char name[60];
    char code[8];
} Airport;

static const Airport airports[AIRPORT_COUNT] = {
    {"Dhaka - Hazrat Shahjalal International Airport", "DAC"},
    {"Chattogram - Shah Amanat International Airport", "CGP"},
    {"Sylhet - Osmani International Airport", "ZYL"},
    {"Cox's Bazar Airport", "CXB"},
    {"Rajshahi - Shah Makhdum Airport", "RJH"},
    {"Jashore Airport", "JSR"},
    {"Saidpur Airport", "SPD"},
    {"Barishal Airport", "BZL"}
};

static const char *demoDeparture[3] = {"08:00", "13:30", "18:00"};
static const char *demoArrival[3]   = {"09:00", "14:30", "19:00"};

static User users[MAX_USERS];
static Booking bookings[MAX_BOOKINGS];
static Flight customFlights[MAX_FLIGHTS];
static int userCount = 0;
static int bookingCount = 0;
static int customFlightCount = 0;

static void removeNewline(char text[])
{
    size_t length = strlen(text);
    if (length > 0 && text[length - 1] == '\n') text[length - 1] = '\0';
}

static void readText(const char *message, char text[], size_t size)
{
    int ch;
    for (;;) {
        printf("%s", message);
        if (fgets(text, (int)size, stdin) == NULL) {
            printf("\nInput ended. Closing the program.\n");
            exit(0);
        }
        if (strchr(text, '\n') == NULL) {
            while ((ch = getchar()) != '\n' && ch != EOF) { }
        }
        removeNewline(text);
        if (text[0] != '\0') return;
        printf("This field cannot be empty. Please try again.\n");
    }
}

static int readNumber(const char *message, int minimum, int maximum)
{
    char line[80];
    char *end;
    long value;
    for (;;) {
        printf("%s", message);
        if (fgets(line, sizeof(line), stdin) == NULL) {
            printf("\nInput ended. Closing the program.\n");
            exit(0);
        }
        removeNewline(line);
        value = strtol(line, &end, 10);
        while (isspace((unsigned char)*end)) end++;
        if (line[0] != '\0' && *end == '\0' &&
            value >= minimum && value <= maximum) return (int)value;
        printf("Please enter a number from %d to %d.\n", minimum, maximum);
    }
}

static double readMoney(const char *message, double minimum, double maximum)
{
    char line[80];
    char *end;
    double value;
    for (;;) {
        printf("%s", message);
        if (fgets(line, sizeof(line), stdin) == NULL) {
            printf("\nInput ended. Closing the program.\n");
            exit(0);
        }
        removeNewline(line);
        value = strtod(line, &end);
        while (isspace((unsigned char)*end)) end++;
        if (line[0] != '\0' && *end == '\0' && value >= minimum && value <= maximum)
            return value;
        printf("Enter an amount between %.0f and %.0f BDT.\n", minimum, maximum);
    }
}

static int yesNo(const char *message)
{
    char answer[20];
    for (;;) {
        readText(message, answer, sizeof(answer));
        if (answer[0] == 'Y' || answer[0] == 'y') return 1;
        if (answer[0] == 'N' || answer[0] == 'n') return 0;
        printf("Enter Y or N.\n");
    }
}

static void clearScreen(void)
{
    printf("\033[2J\033[H");
    fflush(stdout);
}

static void boxBorder(void)
{
    int i;
    printf("  +");
    for (i = 0; i < UI_WIDTH - 2; i++) putchar('-');
    printf("+\n");
}

static void boxText(const char *text)
{
    int length = (int)strlen(text);
    int left, right;
    if (length > UI_WIDTH - 4) length = UI_WIDTH - 4;
    left = (UI_WIDTH - 2 - length) / 2;
    right = UI_WIDTH - 2 - length - left;
    printf("  |%*s%.*s%*s|\n", left, "", length, text, right, "");
}

static void header(const char *title)
{
    clearScreen();
    boxBorder();
    boxText(title);
    boxBorder();
    putchar('\n');
}

static void welcomeScreen(void)
{
    clearScreen();
    boxBorder();
    boxText("AIRLINE RESERVATION");
    boxText("Plan your journey with ease");
    boxBorder();
    putchar('\n');
    puts("  Demonstration system | C Programming Project");
    puts("  Flight schedules and payment options are simulated.\n");
    puts("  1. User section");
    puts("  2. Admin section");
    puts("  3. Exit\n");
}

static void waitForEnter(void)
{
    char line[8];
    printf("\nPress Enter to continue...");
    fgets(line, sizeof(line), stdin);
}

static int leapYear(int year)
{
    return (year % 400 == 0) || (year % 4 == 0 && year % 100 != 0);
}

static int validDate(const char *date)
{
    int year, month, day;
    int monthDays[12] = {31,28,31,30,31,30,31,31,30,31,30,31};
    if (strlen(date) != 10 || date[4] != '-' || date[7] != '-') return 0;
    if (sscanf(date, "%4d-%2d-%2d", &year, &month, &day) != 3) return 0;
    if (year < 2026 || month < 1 || month > 12) return 0;
    if (leapYear(year)) monthDays[1] = 29;
    return day >= 1 && day <= monthDays[month - 1];
}

static int isFutureDate(const char *date)
{
    int year, month, day;
    int todayYear, todayMonth, todayDay;
    time_t now = time(NULL);
    struct tm *today = localtime(&now);
    if (!today) return 0;
    sscanf(date, "%4d-%2d-%2d", &year, &month, &day);
    todayYear = today->tm_year + 1900;
    todayMonth = today->tm_mon + 1;
    todayDay = today->tm_mday;
    if (year != todayYear) return year > todayYear;
    if (month != todayMonth) return month > todayMonth;
    return day > todayDay;
}

static void readTravelDate(char date[])
{
    for (;;) {
        readText("Travel date (YYYY-MM-DD): ", date, 11);
        if (!validDate(date)) {
            printf("Invalid date. Example: 2027-01-20\n");
        } else if (!isFutureDate(date)) {
            printf("Please choose a future date.\n");
        } else {
            return;
        }
    }
}

static int validTime(const char *text)
{
    int hour, minute;
    if (strlen(text) != 5 || text[2] != ':') return 0;
    if (sscanf(text, "%2d:%2d", &hour, &minute) != 2) return 0;
    return hour >= 0 && hour <= 23 && minute >= 0 && minute <= 59;
}

static int loadFile(const char *fileName, void *data, size_t itemSize, int maxItems)
{
    FILE *file = fopen(fileName, "rb");
    int count = 0;
    if (!file) return 0;
    while (count < maxItems && fread((char *)data + count * itemSize, itemSize, 1, file) == 1)
        count++;
    fclose(file);
    return count;
}

static int saveFile(const char *fileName, const void *data, size_t itemSize, int count)
{
    FILE *file = fopen(fileName, "wb");
    if (!file) {
        printf("Could not save %s. Check folder permission.\n", fileName);
        return 0;
    }
    if (fwrite(data, itemSize, (size_t)count, file) != (size_t)count) {
        fclose(file);
        printf("File writing error: %s\n", fileName);
        return 0;
    }
    fclose(file);
    return 1;
}

static void loadData(void)
{
    userCount = loadFile(USER_FILE, users, sizeof(User), MAX_USERS);
    bookingCount = loadFile(BOOKING_FILE, bookings, sizeof(Booking), MAX_BOOKINGS);
    customFlightCount = loadFile(FLIGHT_FILE, customFlights, sizeof(Flight), MAX_FLIGHTS);
}

static void showAirports(void)
{
    int i;
    printf("\nAvailable airport choices (demonstration project):\n");
    for (i = 0; i < AIRPORT_COUNT; i++)
        printf("  %d. %-48s (%s)\n", i + 1, airports[i].name, airports[i].code);
}

static int chooseAirport(const char *message)
{
    showAirports();
    return readNumber(message, 1, AIRPORT_COUNT) - 1;
}

static int findUser(const char *username)
{
    int i;
    for (i = 0; i < userCount; i++)
        if (strcmp(users[i].username, username) == 0) return i;
    return -1;
}

static int findBooking(long bookingId)
{
    int i;
    for (i = 0; i < bookingCount; i++)
        if (bookings[i].bookingId == bookingId) return i;
    return -1;
}

static int findCustomFlight(const char *flightNo)
{
    int i;
    for (i = 0; i < customFlightCount; i++)
        if (strcmp(customFlights[i].flightNo, flightNo) == 0) return i;
    return -1;
}

static long nextBookingId(void)
{
    long highest = 3000;
    int i;
    for (i = 0; i < bookingCount; i++)
        if (bookings[i].bookingId > highest) highest = bookings[i].bookingId;
    return highest + 1;
}

static int seatIsBooked(const char *flightNo, const char *date, int seatNo)
{
    int i, j;
    for (i = 0; i < bookingCount; i++) {
        if (bookings[i].active == 1 && strcmp(bookings[i].flightNo, flightNo) == 0 &&
            strcmp(bookings[i].travelDate, date) == 0) {
            for (j = 0; j < bookings[i].passengerCount; j++)
                if (bookings[i].passengers[j].seatNo == seatNo) return 1;
        }
    }
    return 0;
}

static int seatsAvailable(const Flight *flight, const char *date)
{
    int seat, count = 0;
    for (seat = 1; seat <= flight->totalSeats; seat++)
        if (!seatIsBooked(flight->flightNo, date, seat)) count++;
    return count;
}

static int flightHasBookings(const char *flightNo)
{
    int i;
    for (i = 0; i < bookingCount; i++)
        if (bookings[i].active == 1 && strcmp(bookings[i].flightNo, flightNo) == 0) return 1;
    return 0;
}

static double sampleFare(int fromIndex, int toIndex, int slot)
{
    int difference = fromIndex - toIndex;
    if (difference < 0) difference = -difference;
    return 3200.0 + (difference * 350.0) + (slot * 250.0);
}

static void makeSampleFlight(Flight *flight, int fromIndex, int toIndex, int slot)
{
    flight->flightNo[0] = 'D';
    flight->flightNo[1] = 'M';
    flight->flightNo[2] = (char)('0' + fromIndex + 1);
    flight->flightNo[3] = (char)('0' + toIndex + 1);
    flight->flightNo[4] = (char)('0' + slot + 1);
    flight->flightNo[5] = '\0';
    flight->fromIndex = fromIndex;
    flight->toIndex = toIndex;
    strcpy(flight->departure, demoDeparture[slot]);
    strcpy(flight->arrival, demoArrival[slot]);
    flight->totalSeats = SEATS_PER_DEMO_FLIGHT;
    flight->baseFare = sampleFare(fromIndex, toIndex, slot);
    flight->active = 1;
}

static void showFlightOption(int number, const Flight *flight, const char *date)
{
    char line[100];
    boxBorder();
    snprintf(line, sizeof(line), "FLIGHT OPTION %d  |  %s", number, flight->flightNo);
    boxText(line);
    snprintf(line, sizeof(line), "%s  >>>  %s",
             airports[flight->fromIndex].code, airports[flight->toIndex].code);
    boxText(line);
    snprintf(line, sizeof(line), "Date: %s   Time: %s - %s",
             date, flight->departure, flight->arrival);
    boxText(line);
    snprintf(line, sizeof(line), "Seats: %d / %d available",
             seatsAvailable(flight, date), flight->totalSeats);
    boxText(line);
    snprintf(line, sizeof(line), "Adult fare: %.0f BDT", flight->baseFare);
    boxText(line);
    boxBorder();
}

static int collectFlights(int fromIndex, int toIndex, const char *date,
                          Flight options[], int maxOptions)
{
    int i, count = 0, slot;
    Flight sample;
    for (slot = 0; slot < 3 && count < maxOptions; slot++) {
        makeSampleFlight(&sample, fromIndex, toIndex, slot);
        if (seatsAvailable(&sample, date) > 0) options[count++] = sample;
    }
    for (i = 0; i < customFlightCount && count < maxOptions; i++) {
        if (customFlights[i].active == 1 && customFlights[i].fromIndex == fromIndex &&
            customFlights[i].toIndex == toIndex && seatsAvailable(&customFlights[i], date) > 0)
            options[count++] = customFlights[i];
    }
    return count;
}

static int findAndShowFlights(int *fromIndex, int *toIndex, char date[],
                              Flight options[], int maxOptions)
{
    int count, i;
    header("FIND A FLIGHT");
    *fromIndex = chooseAirport("Select FROM airport number: ");
    *toIndex = chooseAirport("Select TO airport number: ");
    if (*fromIndex == *toIndex) {
        printf("From and To airports must be different.\n");
        return 0;
    }
    readTravelDate(date);
    count = collectFlights(*fromIndex, *toIndex, date, options, maxOptions);
    printf("\nAvailable demonstration flights for %s:\n", date);
    if (count == 0) printf("\nNo flights found for the selected route and date.\n");
    for (i = 0; i < count; i++) showFlightOption(i + 1, &options[i], date);
    return count;
}

static void searchFlights(void)
{
    Flight options[120];
    char date[11];
    int fromIndex, toIndex;
    findAndShowFlights(&fromIndex, &toIndex, date, options, 120);
}

static void registerUser(void)
{
    User user;
    char confirm[30];
    header("NEW USER REGISTRATION");
    if (userCount >= MAX_USERS) {
        printf("User storage is full.\n");
        return;
    }
    readText("Full name: ", user.name, sizeof(user.name));
    readText("Create username (no spaces): ", user.username, sizeof(user.username));
    if (strpbrk(user.username, " \t") != NULL || findUser(user.username) >= 0 ||
        strcmp(user.username, ADMIN_USER) == 0) {
        printf("Username is not available. Try another username.\n");
        return;
    }
    readText("Create password (minimum 4 characters): ", user.password, sizeof(user.password));
    if (strlen(user.password) < 4) {
        printf("Password is too short. Registration cancelled.\n");
        return;
    }
    readText("Confirm password: ", confirm, sizeof(confirm));
    if (strcmp(user.password, confirm) != 0) {
        printf("Passwords do not match. Registration cancelled.\n");
        return;
    }
    readText("Contact phone number: ", user.phone, sizeof(user.phone));
    users[userCount++] = user;
    if (!saveFile(USER_FILE, users, sizeof(User), userCount)) {
        userCount--;
        return;
    }
    printf("Registration completed. Now you can log in.\n");
}

static int userLogin(char username[])
{
    char password[30];
    int index;
    header("USER LOGIN");
    readText("Username: ", username, 30);
    readText("Password: ", password, sizeof(password));
    index = findUser(username);
    if (index < 0 || strcmp(users[index].password, password) != 0) {
        printf("Incorrect username or password.\n");
        return 0;
    }
    printf("Welcome, %s. Login successful.\n", users[index].name);
    return 1;
}

static void printCancelledCard(const Booking *booking)
{
    char line[100];
    boxBorder();
    boxText("BOOKING CANCELLED");
    snprintf(line, sizeof(line), "Booking ID: %ld", booking->bookingId);
    boxText(line);
    boxText("YOUR BOOKING IS CANCELLED");
    boxText("This ticket is no longer active or valid.");
    boxBorder();
}

static void printTicket(const Booking *booking)
{
    int i;
    char line[120];
    if (!booking->active) {
        printCancelledCard(booking);
        return;
    }

    boxBorder();
    boxText("E-TICKET  |  BOOKING CONFIRMED");
    boxBorder();
    snprintf(line, sizeof(line), "BOOKING ID: %ld", booking->bookingId);
    boxText(line);
    snprintf(line, sizeof(line), "FLIGHT: %s", booking->flightNo);
    boxText(line);
    snprintf(line, sizeof(line), "FROM: %.48s (%s)", booking->fromCity, booking->fromCode);
    boxText(line);
    snprintf(line, sizeof(line), "TO: %.48s (%s)", booking->toCity, booking->toCode);
    boxText(line);
    snprintf(line, sizeof(line), "DATE: %s  |  DEPARTURE: %s  |  ARRIVAL: %s",
             booking->travelDate, booking->departure, booking->arrival);
    boxText(line);
    boxBorder();

    for (i = 0; i < booking->passengerCount; i++) {
        const Passenger *passenger = &booking->passengers[i];
        snprintf(line, sizeof(line), "%-6s %-18.18s AGE %2d  SEAT %02d  %.2f BDT",
                 passenger->type, passenger->name, passenger->age,
                 passenger->seatNo, passenger->fare);
        boxText(line);
    }

    boxBorder();
    snprintf(line, sizeof(line), "SUBTOTAL: %.2f BDT", booking->subTotal);
    boxText(line);
    snprintf(line, sizeof(line), "SERVICE CHARGE: %.2f BDT", booking->serviceCharge);
    boxText(line);
    snprintf(line, sizeof(line), "TOTAL: %.2f BDT", booking->totalFare);
    boxText(line);
    snprintf(line, sizeof(line), "PAYMENT: %s", booking->paymentMethod);
    boxText(line);
    snprintf(line, sizeof(line), "PAYMENT STATUS: %s", booking->paymentStatus);
    boxText(line);
    boxText("STATUS: CONFIRMED");
    boxBorder();
    puts("  Keep the Booking ID for your records.");
}

static void showSeatMap(const Flight *flight, const char *date)
{
    int seat;
    printf("\nSeat map: [XX] means booked; number means available\n");
    for (seat = 1; seat <= flight->totalSeats; seat++) {
        if (seatIsBooked(flight->flightNo, date, seat)) printf("[XX] ");
        else printf("[%02d] ", seat);
        if (seat % 6 == 0) printf("\n");
    }
    if (flight->totalSeats % 6 != 0) printf("\n");
}

static void bookFlight(const char *username)
{
    Flight options[120], selectedFlight;
    Booking booking;
    char date[11];
    int fromIndex, toIndex, flightCountFound, option, adults, children;
    int totalPassengers, i, j, seat, duplicate;
    int userIndex = findUser(username);

    flightCountFound = findAndShowFlights(&fromIndex, &toIndex, date, options, 120);
    if (flightCountFound == 0) return;
    option = readNumber("Choose flight option number: ", 1, flightCountFound) - 1;
    selectedFlight = options[option];

    printf("\nOne registered account can book tickets for family members too.\n");
    printf("Family members do NOT need separate login accounts.\n");
    adults = readNumber("Number of adults (age 12 or above): ", 1, MAX_PASSENGERS);
    children = readNumber("Number of children (age 2 to 11): ", 0, MAX_PASSENGERS - adults);
    totalPassengers = adults + children;

    if (totalPassengers > seatsAvailable(&selectedFlight, date)) {
        printf("Not enough seats. Only %d seat(s) are available.\n",
               seatsAvailable(&selectedFlight, date));
        return;
    }
    if (bookingCount >= MAX_BOOKINGS) {
        printf("Booking storage is full.\n");
        return;
    }

    memset(&booking, 0, sizeof(booking));
    booking.bookingId = nextBookingId();
    strcpy(booking.username, username);
    strcpy(booking.flightNo, selectedFlight.flightNo);
    strcpy(booking.fromCode, airports[fromIndex].code);
    strcpy(booking.fromCity, airports[fromIndex].name);
    strcpy(booking.toCode, airports[toIndex].code);
    strcpy(booking.toCity, airports[toIndex].name);
    strcpy(booking.travelDate, date);
    strcpy(booking.departure, selectedFlight.departure);
    strcpy(booking.arrival, selectedFlight.arrival);
    booking.passengerCount = totalPassengers;
    booking.active = 1;

    printf("\nEnter passenger details.\n");
    printf("Adult passengers are entered first, then children.\n");
    for (i = 0; i < totalPassengers; i++) {
        Passenger *passenger = &booking.passengers[i];
        printf("\nPassenger %d of %d\n", i + 1, totalPassengers);
        readText("Passenger full name: ", passenger->name, sizeof(passenger->name));
        if (i < adults) {
            strcpy(passenger->type, "Adult");
            passenger->age = readNumber("Age (12-100): ", 12, 100);
            passenger->fare = selectedFlight.baseFare;
        } else {
            strcpy(passenger->type, "Child");
            passenger->age = readNumber("Age (2-11): ", 2, 11);
            passenger->fare = selectedFlight.baseFare * CHILD_RATE;
        }
        showSeatMap(&selectedFlight, date);
        for (;;) {
            seat = readNumber("Choose seat number: ", 1, selectedFlight.totalSeats);
            duplicate = seatIsBooked(selectedFlight.flightNo, date, seat);
            for (j = 0; j < i; j++)
                if (booking.passengers[j].seatNo == seat) duplicate = 1;
            if (!duplicate) break;
            printf("That seat is already taken. Please choose another seat.\n");
        }
        passenger->seatNo = seat;
        booking.subTotal += passenger->fare;
    }

    booking.serviceCharge = booking.subTotal * SERVICE_RATE;
    booking.totalFare = booking.subTotal + booking.serviceCharge;
    printf("\n---------------------- FARE SUMMARY ----------------------\n");
    printf("Adult fare       : %.2f BDT each\n", selectedFlight.baseFare);
    printf("Child fare       : %.2f BDT each (75%% of sample adult fare)\n",
           selectedFlight.baseFare * CHILD_RATE);
    printf("Passengers       : %d adult(s), %d child(ren)\n", adults, children);
    printf("Subtotal         : %.2f BDT\n", booking.subTotal);
    printf("Service charge   : %.2f BDT (5%%)\n", booking.serviceCharge);
    printf("Total             : %.2f BDT\n", booking.totalFare);
    printf("----------------------------------------------------------\n");

    printf("\nChoose a payment method (DEMO ONLY):\n");
    printf("1. Pay at airport counter\n");
    printf("2. bKash (simulated)\n");
    printf("3. Nagad (simulated)\n");
    printf("4. Card (simulated)\n");
    switch (readNumber("Payment choice: ", 1, 4)) {
        case 1:
            strcpy(booking.paymentMethod, "Airport counter");
            strcpy(booking.paymentStatus, "PAY AT COUNTER");
            break;
        case 2:
            strcpy(booking.paymentMethod, "bKash Demo");
            strcpy(booking.paymentStatus, "SIMULATED ONLY");
            break;
        case 3:
            strcpy(booking.paymentMethod, "Nagad Demo");
            strcpy(booking.paymentStatus, "SIMULATED ONLY");
            break;
        default:
            strcpy(booking.paymentMethod, "Card Demo");
            strcpy(booking.paymentStatus, "SIMULATED ONLY");
            break;
    }
    printf("Note: This project does not collect payment PIN, OTP or card details.\n");
    if (!yesNo("Confirm this family booking? (Y/N): ")) {
        printf("Booking cancelled before saving.\n");
        return;
    }

    if (userIndex >= 0) {
        time_t now = time(NULL);
        struct tm *today = localtime(&now);
        if (today) strftime(booking.bookingDate, sizeof(booking.bookingDate), "%Y-%m-%d", today);
    }
    bookings[bookingCount++] = booking;
    if (!saveFile(BOOKING_FILE, bookings, sizeof(Booking), bookingCount)) {
        bookingCount--;
        printf("Booking could not be saved.\n");
        return;
    }
    printf("\nBooking confirmed. Your e-ticket is below.\n");
    printTicket(&booking);
}

static void showBookingSummary(const Booking *booking)
{
    printf("ID:%ld | Flight:%s | %s to %s | %s | %d passenger(s) | %.2f BDT | %s\n",
           booking->bookingId, booking->flightNo, booking->fromCode, booking->toCode,
           booking->travelDate, booking->passengerCount, booking->totalFare,
           booking->active ? "CONFIRMED" : "CANCELLED");
}

static void viewMyBookings(const char *username)
{
    int i, activeCount = 0, cancelledCount = 0;
    long id;

    header("MY BOOKINGS");
    puts("  CONFIRMED BOOKINGS\n");
    for (i = 0; i < bookingCount; i++) {
        if (strcmp(bookings[i].username, username) == 0 && bookings[i].active) {
            showBookingSummary(&bookings[i]);
            activeCount++;
        }
    }
    if (activeCount == 0) puts("  You have no active bookings.");

    puts("\n  CANCELLED BOOKINGS\n");
    for (i = 0; i < bookingCount; i++) {
        if (strcmp(bookings[i].username, username) == 0 && !bookings[i].active) {
            printCancelledCard(&bookings[i]);
            cancelledCount++;
        }
    }
    if (cancelledCount == 0) puts("  No cancelled bookings.");

    if (activeCount == 0) return;
    if (yesNo("\nView a confirmed e-ticket? (Y/N): ")) {
        id = (long)readNumber("Enter Booking ID: ", 1, 2147483647);
        i = findBooking(id);
        if (i < 0 || strcmp(bookings[i].username, username) != 0) {
            puts("Booking ID not found in your account.");
        } else if (!bookings[i].active) {
            puts("YOUR BOOKING IS CANCELLED. This ticket is no longer active.");
        } else {
            printTicket(&bookings[i]);
        }
    }
}

static void cancelBookingById(long id)
{
    int index = findBooking(id);
    if (index < 0) {
        puts("Booking ID not found.");
        return;
    }
    if (!bookings[index].active) {
        puts("This booking is already cancelled.");
        printCancelledCard(&bookings[index]);
        return;
    }

    bookings[index].active = 0;
    if (!saveFile(BOOKING_FILE, bookings, sizeof(Booking), bookingCount)) {
        bookings[index].active = 1;
        puts("Could not save cancellation.");
        return;
    }

    puts("\nYour booking has been cancelled successfully.");
    puts("The reserved seats are available again.");
    printCancelledCard(&bookings[index]);
}

static void cancelMyBooking(const char *username)
{
    int i, found = 0;
    long id;
    header("CANCEL BOOKING");
    for (i = 0; i < bookingCount; i++) {
        if (strcmp(bookings[i].username, username) == 0 && bookings[i].active) {
            showBookingSummary(&bookings[i]);
            found++;
        }
    }
    if (!found) {
        printf("You have no confirmed booking to cancel.\n");
        return;
    }
    id = (long)readNumber("Enter Booking ID to cancel: ", 1, 2147483647);
    i = findBooking(id);
    if (i < 0 || strcmp(bookings[i].username, username) != 0) {
        printf("This Booking ID does not belong to your account.\n");
        return;
    }
    if (yesNo("Are you sure? (Y/N): ")) cancelBookingById(id);
}

static void userDashboard(const char *username)
{
    int choice;
    do {
        header("USER DASHBOARD");
        printf("Logged in as: %s\n\n", username);
        printf("1. Search flights\n");
        printf("2. Book flight (single or family)\n");
        printf("3. View my bookings / e-ticket\n");
        printf("4. Cancel my booking\n");
        printf("5. Logout\n");
        choice = readNumber("Choose option: ", 1, 5);
        switch (choice) {
            case 1: searchFlights(); waitForEnter(); break;
            case 2: bookFlight(username); waitForEnter(); break;
            case 3: viewMyBookings(username); waitForEnter(); break;
            case 4: cancelMyBooking(username); waitForEnter(); break;
            case 5: printf("Logged out.\n"); break;
        }
    } while (choice != 5);
}

static void userSection(void)
{
    int choice;
    char username[30];
    do {
        header("USER SECTION");
        printf("1. Register new account\n");
        printf("2. Login\n");
        printf("3. Back to main menu\n");
        choice = readNumber("Choose option: ", 1, 3);
        if (choice == 1) { registerUser(); waitForEnter(); }
        else if (choice == 2) {
            if (userLogin(username)) userDashboard(username);
            waitForEnter();
        }
    } while (choice != 3);
}

static int adminLogin(void)
{
    char username[30], password[30];
    header("ADMIN LOGIN");
    readText("Admin username: ", username, sizeof(username));
    readText("Admin password: ", password, sizeof(password));
    if (strcmp(username, ADMIN_USER) == 0 && strcmp(password, ADMIN_PASS) == 0) {
        printf("Admin login successful.\n");
        return 1;
    }
    printf("Incorrect admin login.\n");
    return 0;
}

static void listCustomFlights(void)
{
    int i, shown = 0;
    printf("\nAdmin-added flight templates:\n");
    printf("Flight       From   To     Departure  Arrival  Seats  Fare(BDT)  Status\n");
    printf("--------------------------------------------------------------------------\n");
    for (i = 0; i < customFlightCount; i++) {
        printf("%-12s %-6s %-6s %-10s %-8s %-6d %-10.0f %s\n",
               customFlights[i].flightNo,
               airports[customFlights[i].fromIndex].code,
               airports[customFlights[i].toIndex].code,
               customFlights[i].departure, customFlights[i].arrival,
               customFlights[i].totalSeats, customFlights[i].baseFare,
               customFlights[i].active ? "ACTIVE" : "INACTIVE");
        shown++;
    }
    if (!shown) printf("No custom flight templates have been added.\n");
}

static void addCustomFlight(void)
{
    Flight flight;
    header("ADD ADMIN FLIGHT TEMPLATE");
    if (customFlightCount >= MAX_FLIGHTS) {
        printf("Flight list is full.\n");
        return;
    }
    readText("Flight number (example DEMO501): ", flight.flightNo, sizeof(flight.flightNo));
    if (findCustomFlight(flight.flightNo) >= 0) {
        printf("That flight number already exists.\n");
        return;
    }
    flight.fromIndex = chooseAirport("Select FROM airport number: ");
    flight.toIndex = chooseAirport("Select TO airport number: ");
    if (flight.fromIndex == flight.toIndex) {
        printf("From and To must be different.\n");
        return;
    }
    readText("Departure time (HH:MM): ", flight.departure, sizeof(flight.departure));
    readText("Arrival time (HH:MM): ", flight.arrival, sizeof(flight.arrival));
    if (!validTime(flight.departure) || !validTime(flight.arrival)) {
        printf("Time format is invalid. Use 24-hour HH:MM.\n");
        return;
    }
    flight.totalSeats = readNumber("Total seats (1-200): ", 1, 200);
    flight.baseFare = readMoney("Adult base fare in BDT: ", 100, 100000);
    flight.active = 1;
    customFlights[customFlightCount++] = flight;
    if (!saveFile(FLIGHT_FILE, customFlights, sizeof(Flight), customFlightCount)) {
        customFlightCount--;
        return;
    }
    printf("Custom flight template saved. It will appear for this route on selected future dates.\n");
}

static void updateCustomFlight(void)
{
    char flightNo[20];
    int index, newSeats, highestSeat = 0, i, j;
    Flight oldFlight, changedFlight;
    header("UPDATE ADMIN FLIGHT TEMPLATE");
    listCustomFlights();
    readText("Flight number to update: ", flightNo, sizeof(flightNo));
    index = findCustomFlight(flightNo);
    if (index < 0 || !customFlights[index].active) {
        printf("Active custom flight not found.\n");
        return;
    }
    oldFlight = customFlights[index];
    changedFlight = oldFlight;
    changedFlight.fromIndex = chooseAirport("New FROM airport number: ");
    changedFlight.toIndex = chooseAirport("New TO airport number: ");
    if (changedFlight.fromIndex == changedFlight.toIndex) {
        printf("Invalid route. Update cancelled.\n");
        return;
    }
    readText("New departure time (HH:MM): ", changedFlight.departure, sizeof(changedFlight.departure));
    readText("New arrival time (HH:MM): ", changedFlight.arrival, sizeof(changedFlight.arrival));
    if (!validTime(changedFlight.departure) || !validTime(changedFlight.arrival)) {
        printf("Invalid time. Update cancelled.\n");
        return;
    }
    for (i = 0; i < bookingCount; i++) {
        if (bookings[i].active && strcmp(bookings[i].flightNo, flightNo) == 0) {
            for (j = 0; j < bookings[i].passengerCount; j++)
                if (bookings[i].passengers[j].seatNo > highestSeat)
                    highestSeat = bookings[i].passengers[j].seatNo;
        }
    }
    newSeats = readNumber("New total seats (1-200): ", 1, 200);
    if (newSeats < highestSeat) {
        printf("Cannot reduce seats below the highest already-booked seat number (%d).\n", highestSeat);
        return;
    }
    changedFlight.totalSeats = newSeats;
    changedFlight.baseFare = readMoney("New adult base fare in BDT: ", 100, 100000);
    customFlights[index] = changedFlight;
    if (!saveFile(FLIGHT_FILE, customFlights, sizeof(Flight), customFlightCount)) {
        customFlights[index] = oldFlight;
        return;
    }
    printf("Custom flight template updated.\n");
}

static void deactivateCustomFlight(void)
{
    char flightNo[20];
    int index;
    header("DEACTIVATE ADMIN FLIGHT TEMPLATE");
    listCustomFlights();
    readText("Flight number to deactivate: ", flightNo, sizeof(flightNo));
    index = findCustomFlight(flightNo);
    if (index < 0 || !customFlights[index].active) {
        printf("Active custom flight not found.\n");
        return;
    }
    if (flightHasBookings(flightNo)) {
        printf("This flight has confirmed bookings. Cancel them before deactivation.\n");
        return;
    }
    if (yesNo("Deactivate this flight template? (Y/N): ")) {
        customFlights[index].active = 0;
        if (!saveFile(FLIGHT_FILE, customFlights, sizeof(Flight), customFlightCount)) {
            customFlights[index].active = 1;
            return;
        }
        printf("Flight template deactivated.\n");
    }
}

static void manageFlights(void)
{
    int choice;
    do {
        header("FLIGHT MANAGEMENT");
        printf("1. View admin-added flight templates\n");
        printf("2. Add flight template\n");
        printf("3. Update flight template\n");
        printf("4. Deactivate flight template\n");
        printf("5. Back\n");
        choice = readNumber("Choose option: ", 1, 5);
        switch (choice) {
            case 1: listCustomFlights(); waitForEnter(); break;
            case 2: addCustomFlight(); waitForEnter(); break;
            case 3: updateCustomFlight(); waitForEnter(); break;
            case 4: deactivateCustomFlight(); waitForEnter(); break;
            case 5: break;
        }
    } while (choice != 5);
}

static void listAllBookings(void)
{
    int i;
    header("ALL BOOKING RECORDS");
    if (bookingCount == 0) {
        printf("No booking records found.\n");
        return;
    }
    for (i = 0; i < bookingCount; i++) showBookingSummary(&bookings[i]);
}

static void adminCancelBooking(void)
{
    long id;
    listAllBookings();
    if (bookingCount == 0) return;
    id = (long)readNumber("Booking ID to cancel: ", 1, 2147483647);
    if (yesNo("Confirm cancellation? (Y/N): ")) cancelBookingById(id);
}

static void manageBookings(void)
{
    int choice;
    do {
        header("BOOKING MANAGEMENT");
        printf("1. View all bookings\n");
        printf("2. Cancel a booking\n");
        printf("3. Back\n");
        choice = readNumber("Choose option: ", 1, 3);
        if (choice == 1) { listAllBookings(); waitForEnter(); }
        else if (choice == 2) { adminCancelBooking(); waitForEnter(); }
    } while (choice != 3);
}

static void adminDashboard(void)
{
    int choice;
    do {
        header("ADMIN DASHBOARD");
        printf("1. Manage flights\n");
        printf("2. Manage booking records\n");
        printf("3. Logout\n");
        choice = readNumber("Choose option: ", 1, 3);
        if (choice == 1) manageFlights();
        else if (choice == 2) manageBookings();
    } while (choice != 3);
}

static void adminSection(void)
{
    if (adminLogin()) adminDashboard();
}

int main(void)
{
    int choice;
    int firstScreen = 1;

    loadData();
    do {
        if (firstScreen) welcomeScreen();
        else header("HOME");
        if (!firstScreen) {
            puts("  1. User section");
            puts("  2. Admin section");
            puts("  3. Exit\n");
        }
        firstScreen = 0;
        choice = readNumber("  Choose option: ", 1, 3);
        if (choice == 1) userSection();
        else if (choice == 2) adminSection();
        else {
            header("GOODBYE");
            boxText("Thank you for using the reservation system.");
            boxBorder();
        }
    } while (choice != 3);

    return 0;
}
