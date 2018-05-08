#include "server.h"

pthread_t *threads;
int fd_req, num_room_seats;
struct client_request buffer;
sem_t requestOnWait;
pthread_mutex_t buffer_lock = PTHREAD_MUTEX_INITIALIZER;
struct Seat *seats;
int availableSeats, num_ticket_offices;
bool closeTicketOffices = false;
pthread_mutex_t *seatMutexes;

void notProcessingReqTermHandler(int signo)
{
    if (signo != SIGTERM)
        return;

    int officeNum = getTicketOfficeNum(pthread_self());

    int fd_slog = open("slog.txt", O_WRONLY | O_APPEND | O_CREAT, DEFAULT_PERMISSION);
    char *ticketOfficeNum = (char *)malloc(sizeof(char) * (WIDTH_TICKET_OFFICE + 1 + 1)); //1-> '-' + 1-> null terminator
    sprintf(ticketOfficeNum, ticketOfficeNumFormat, officeNum);
    write(fd_slog, ticketOfficeNum, sizeof(char) * (WIDTH_TICKET_OFFICE + 1+1));
    write(fd_slog, "CLOSED\n", sizeof("CLOSED\n"));

    close(fd_slog);
    exit(1);
}

void processingReqTermHandler(int signo)
{
    if (signo != SIGTERM)
        return;

    closeTicketOffices = true;
}

void alarmHandler(int signo)
{
    if (signo != SIGALRM)
        return;
    close(fd_req);      //close the fifo
    unlink("requests"); //delete the fifo

    //Sending terminating signal to threads
    for (int i = 0; i < sizeof(threads) / sizeof(pthread_t); i++)
    {
        pthread_kill(threads[i], SIGTERM);
    }
    for (int i = 0; i < sizeof(threads) / sizeof(pthread_t); i++)
    {
        pthread_join(threads[i], NULL);
    }

    int fd_slog = open("slog.txt", O_WRONLY | O_APPEND | O_CREAT, DEFAULT_PERMISSION);
    write(fd_slog, "SERVER CLOSED\n", sizeof("SERVER CLOSED\n"));
    close(fd_slog);
    exit(0);
}

int isSeatFree(struct Seat *seats, int seatNum)
{
    pthread_mutex_lock(&seatMutexes[seatNum - 1]);
    int freeSeat = (seats[seatNum - 1].client == 0);
    DELAY();
    pthread_mutex_unlock(&seatMutexes[seatNum - 1]);
    return freeSeat;
}

void bookSeat(struct Seat *seats, int seatNum, int clientId)
{
    pthread_mutex_lock(&seatMutexes[seatNum - 1]);
    availableSeats--;
    seats[seatNum - 1].client = clientId;
    DELAY();
    pthread_mutex_unlock(&seatMutexes[seatNum - 1]);
}

void freeSeat(struct Seat *seats, int seatNum)
{
    pthread_mutex_lock(&seatMutexes[seatNum - 1]);
    availableSeats++;
    seats[seatNum - 1].client = 0;
    DELAY();
    pthread_mutex_unlock(&seatMutexes[seatNum - 1]);
}

void *ticketOffice(void *arg)
{
    signal(SIGTERM, notProcessingReqTermHandler);
    struct client_request cr;
    int officeNum = *((int *)arg);
    int fd_slog = open("slog.txt", O_WRONLY | O_APPEND | O_CREAT, DEFAULT_PERMISSION);
    char *ticketOfficeNum = (char *)malloc(sizeof(char) * (WIDTH_TICKET_OFFICE + 1 + 1)); //1-> '-' + 1-> null terminator
    sprintf(ticketOfficeNum, ticketOfficeNumFormat, officeNum);

    //Writting open state to file
    write(fd_slog, ticketOfficeNum, sizeof(char) * (WIDTH_TICKET_OFFICE + 1 + 1));
    write(fd_slog, "OPEN\n", sizeof(char) * strlen("OPEN\n"));

    while (!closeTicketOffices)
    {
        //Locking buffer to wait for request
        pthread_mutex_lock(&buffer_lock);
        sem_wait(&requestOnWait);

        processRequest(fd_slog, cr);
        signal(SIGTERM, notProcessingReqTermHandler);
    }

    //Writting closed state to file
    write(fd_slog, ticketOfficeNum, sizeof(char) * (WIDTH_TICKET_OFFICE + 1 + 1));
    write(fd_slog, "CLOSED\n", sizeof("CLOSED\n"));
    close(fd_slog);

    exit(1);
}

void processRequest(int fd_slog, struct client_request cr)
{
    signal(SIGTERM, processingReqTermHandler);
    cr.client_pid = buffer.client_pid;
    cr.num_wanted_seats = buffer.num_wanted_seats;
    intdup(buffer.preferences, cr.preferences, sizeof(buffer.preferences));
    pthread_mutex_unlock(&buffer_lock);
    char *answerFifoName = (char *)(malloc(sizeof(char) * (WIDTH_PID + 3 + 1))); // 3-> ans + 1 -> null terminator
    int fd_ans;
    int numReservedSeats = 0;
    struct Seat reservedSeats[cr.num_wanted_seats];

    //Validating request
    int numPreferences = 0, i = 0;
    bool validPreferences = true;

    while (1)
    {
        if (cr.preferences[i] != 0)
        {
            numPreferences++;
            validPreferences &= (cr.preferences[i] >= 1 && cr.preferences[i] <= num_room_seats);
        }
        else
            break;
        i++;
    }

    bool validNumWantedSeats = (cr.num_wanted_seats >= 1) && (cr.num_wanted_seats <= MAX_CLI_SEATS);
    bool validNumPreferences = (numPreferences >= cr.num_wanted_seats) && (numPreferences <= MAX_CLI_SEATS);
    bool fullRoom = (availableSeats == 0);

    struct server_answer ans;
    if (!validNumWantedSeats)
        ans.state = -1;
    else if (!validNumPreferences)
        ans.state = -2;
    else if (!validPreferences)
        ans.state = -3;
    else if (cr.client_pid <= 0)
        ans.state = -4;
    else if (fullRoom)
        ans.state = -6;
    else
    {
        //Attending request
        for (int i = 0; i < numPreferences; i++)
        {
            if (isSeatFree(seats, cr.preferences[i]))
            {
                bookSeat(seats, cr.preferences[i], cr.client_pid);
                reservedSeats[numReservedSeats] = seats[cr.preferences[i] - 1];
                numReservedSeats++;
                if (numReservedSeats == cr.num_wanted_seats)
                    break;
            }
        }
        if (numReservedSeats != cr.num_wanted_seats)
        {
            ans.state = -5;
            for (int i = 0; i < numReservedSeats; i++)
            {
                freeSeat(seats, (reservedSeats[i]).num);
            }
            numReservedSeats = 0;
        }
        else
        {
            ans.state = 0;
            ans.seats = (int *)malloc(sizeof(int) * (numReservedSeats + 1)); //the number of reserved seats + the seats
            ans.seats[0] = numReservedSeats;
            for (int i = 1; i <= numReservedSeats; i++)
            {
                ans.seats[i] = reservedSeats[i - 1].num;
            }
        }
    }
    sprintf(answerFifoName, answerFifoFormat, cr.client_pid);
    fd_ans = open(answerFifoName, O_WRONLY);
    if (fd_ans == -1)
    { //timeout do client
        for (int i = 0; i < numReservedSeats; i++)
        {
            freeSeat(seats, reservedSeats[i].num);
        }
        numReservedSeats = 0;
    }
    else
    {
        write(fd_ans, &ans, sizeof(ans));

        int officeNum = getTicketOfficeNum(pthread_self());
        char *ticketOfficeNum = (char *)malloc(sizeof(char) * (WIDTH_TICKET_OFFICE + 1 + 1)); //1-> '-' + 1-> null terminator
        sprintf(ticketOfficeNum, ticketOfficeNumFormat, officeNum);

        writeRequestAnswer(fd_slog, ticketOfficeNum, cr, ans);
    }
}

int main(int argc, char *argv[])
{
    if (argc != 4)
        printf("Invalid argumments\n Usage: server <num_room_seats> <num_ticket_offices> <open_time> \n");

    num_room_seats = atoi(argv[0]);
    num_ticket_offices = atoi(argv[1]);
    int open_time = atoi(argv[2]);
    threads = (pthread_t *)malloc(sizeof(pthread_t) * num_ticket_offices);
    availableSeats = num_room_seats;
    int ticketOfficeNums[num_ticket_offices];
    seatMutexes = (pthread_mutex_t *)malloc(sizeof(pthread_mutex_t) * num_room_seats);
    for (int i = 0; i < num_room_seats; i++)
    {
        pthread_mutex_init(&seatMutexes[i], NULL);
    }

    //initialize the seats
    seats = (struct Seat *)malloc(sizeof(struct Seat) * num_room_seats);
    struct Seat defaultSeat;
    defaultSeat.client = 0;
    defaultSeat.num = 1;
    for (int i = 0; i <= num_room_seats; i++)
    {
        seats[i] = defaultSeat;
        defaultSeat.num += 1;
    }

    int sval;

    mkfifo("requests", 0660);
    fd_req = open("requests", O_RDONLY);

    //Initializing semaphore
    sem_init(&requestOnWait, SHARED, 0);

    for (int i = 0; i < num_ticket_offices; i++)
    {
        ticketOfficeNums[i] = i + 1;
        pthread_create(&threads[i], NULL, ticketOffice, &ticketOfficeNums[i]);
    }

    alarm(open_time);
    signal(SIGALRM, alarmHandler);

    while (true)
    {

        sem_getvalue(&requestOnWait, &sval);
        if (sval == 0)
        {
            read(fd_req, &buffer, sizeof(buffer));
            sem_post(&requestOnWait);
        }
    }
    return 0;
}

void intdup(int const *src, int *dest, size_t size)
{
    memcpy(dest, src, size);
}

int getTicketOfficeNum(pthread_t tid)
{
    for (int i = 0; i < num_ticket_offices; i++)
    {
        if (threads[i] == tid)
            return i + 1;
    }
    return 0;
}

void writeRequestAnswer(int fd_slog, char *ticketOfficeNum, struct client_request cr, struct server_answer sa)
{
    int fd_sbook = open("sbook.txt", O_WRONLY | O_APPEND | O_CREAT, DEFAULT_PERMISSION);

    //ticket office num
    write(fd_slog, ticketOfficeNum, sizeof(char) * (WIDTH_TICKET_OFFICE + 1 + 1));

    //client pid
    write(fd_slog, &cr.client_pid, sizeof(int));
    write(fd_slog, "-", sizeof("-"));

    //num tickets wanted
    char *numTicketsWanted = malloc(sizeof(char) * (WIDTH_TICKETS_WANTED + 1 + 1 + 1)); //1-> ' ', 1-> ':' , 1-> null terminator
    sprintf(numTicketsWanted, numTicketsWantedFormat, cr.num_wanted_seats);
    write(fd_slog, numTicketsWanted, sizeof(char) * (WIDTH_TICKETS_WANTED + 1 + 1 + 1));

    //tickets wanted
    char *ticketWanted = malloc(sizeof(char) * (WIDTH_SEAT + 1 + 1)); //1-> ':' , 1-> null terminator
    for (int i = 0; i < MAX_CLI_SEATS; i++)
    {
        if (cr.preferences[i] != 0)
            sprintf(ticketWanted, ticketsFormat, cr.preferences[i]);
        else
            ticketWanted = getSpaceString(WIDTH_SEAT + 1); // 1-> space

        write(fd_slog, ticketWanted, (WIDTH_SEAT + 1 + 1));
        ticketWanted[0] = '\0';
    }

    //request/answer divisor
    write(fd_slog, "- ", sizeof("- "));

    if (sa.state < 0) //request was not successful
    {
        switch (sa.state)
        {
        case -1:
            write(fd_slog, "MAX", sizeof("MAX"));
            break;
        case -2:
            write(fd_slog, "NST", sizeof("NST"));
            break;
        case -3:
            write(fd_slog, "IID", sizeof("IID"));
            break;
        case -4:
            write(fd_slog, "ERR", sizeof("ERR"));
            break;
        case -5:
            write(fd_slog, "NAV", sizeof("NAV"));
            break;
        case -6:
            write(fd_slog, "FUL", sizeof("FUL"));
            break;
        default:
            break;
        }
    }

    else //sucessful was sucessful
    {
        char *ticketReserved = malloc(sizeof(char) * (WIDTH_SEAT + 1 + 1)); //1-> ' ' , 1-> null terminator
        for (int i = 0; i < cr.num_wanted_seats; i++)
        {
            if (cr.preferences[i] != 0)
                sprintf(ticketReserved, ticketsFormat, cr.preferences[i]);
            else
                break;

            write(fd_slog, ticketReserved, (WIDTH_SEAT + 1 + 1));
            write(fd_sbook, ticketReserved, (WIDTH_SEAT + 1 + 1));
            write(fd_sbook, "\n", sizeof("\n"));
            ticketReserved[0] = '\0';
        }
    }
    write(fd_slog, "\n", sizeof("\n"));
    close(fd_sbook);
}

char * getSpaceString(int n)
{
    char *spaceString = (char *)malloc(sizeof(char) * n);
    for (int i = 0; i < n; i++)
    {
        sprintf(spaceString, " ");
    }
    return spaceString;
}