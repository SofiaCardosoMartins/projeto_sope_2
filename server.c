#include "server.h"
#include "macros.h"

struct ticket_office *threads;
int fd_req, num_room_seats;
struct client_request buffer;
sem_t requestOnWait;
pthread_mutex_t buffer_lock = PTHREAD_MUTEX_INITIALIZER;
struct Seat *seats;
int availableSeats, num_ticket_offices;
bool closeTicketOffices = false;
pthread_mutex_t *seatMutexes;
pthread_mutex_t writeSlogMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t writeSbookMutex = PTHREAD_MUTEX_INITIALIZER;

//////////////
sem_t empty;
sem_t full;

void termHandler(int signo)
{
    if (signo != SIGTERM)
        return;

    int officeNum = getTicketOfficeNum(pthread_self());
    closeTicketOffices = true;

    if (threads[officeNum - 1].processingRequest)
        return;
    else
    {
        int fd_slog = open("slog.txt", O_WRONLY | O_APPEND | O_CREAT, DEFAULT_PERMISSION);
        char *ticketOfficeNum = (char *)malloc(sizeof(char) * (WIDTH_TICKET_OFFICE + 1 + 1)); //1-> '-' + 1-> null terminator
        sprintf(ticketOfficeNum, ticketOfficeNumFormat, officeNum);
        pthread_mutex_lock(&writeSlogMutex);
        write(fd_slog, ticketOfficeNum, strlen(ticketOfficeNum));
        write(fd_slog, "CLOSED\n", strlen("CLOSED\n"));
        pthread_mutex_unlock(&writeSlogMutex);

        close(fd_slog);
        pthread_exit(NULL);
    }
}

void alarmHandler(int signo)
{

    if (signo != SIGALRM)
        return;
    close(fd_req);      //close the fifo
    unlink("requests"); //delete the fifo

    //Sending terminating signal to threads
    for (int i = 0; i < num_ticket_offices; i++)
        pthread_kill(threads[i].tid, SIGTERM);

    for (int i = 0; i < num_ticket_offices; i++)
        pthread_join(threads[i].tid, NULL);

    int fd_slog = open("slog.txt", O_WRONLY | O_APPEND | O_CREAT, DEFAULT_PERMISSION);
    pthread_mutex_lock(&writeSlogMutex);
    write(fd_slog, "SERVER CLOSED\n", strlen("SERVER CLOSED\n"));
    pthread_mutex_unlock(&writeSlogMutex);
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
    signal(SIGTERM, termHandler);
    struct client_request *cr = (struct client_request *)malloc(sizeof(struct client_request));
    int *officeNum = (int *)arg;
    int fd_slog = open("slog.txt", O_WRONLY | O_APPEND | O_CREAT, DEFAULT_PERMISSION);

    char *ticketOfficeNum = (char *)(malloc(sizeof(char) * (WIDTH_TICKET_OFFICE + 1 + 1))); //1-> '-' + 1-> null terminator
    sprintf(ticketOfficeNum, ticketOfficeNumFormat, *officeNum);

    //Writting open state to file
    pthread_mutex_lock(&writeSlogMutex);
    write(fd_slog, ticketOfficeNum, strlen(ticketOfficeNum));
    write(fd_slog, "OPEN\n", strlen("OPEN\n"));
    pthread_mutex_unlock(&writeSlogMutex);

    while (!closeTicketOffices)
    {
        //Locking buffer to wait for request
        pthread_mutex_lock(&buffer_lock);
        sem_wait(&full);

        threads[*officeNum - 1].processingRequest = true;
        processRequest(fd_slog, cr);
        threads[*officeNum - 1].processingRequest = false;
    }

    //Writting closed state to file
    pthread_mutex_lock(&writeSlogMutex);
    write(fd_slog, ticketOfficeNum, strlen(ticketOfficeNum));
    write(fd_slog, "CLOSED\n", strlen("CLOSED\n"));
    pthread_mutex_unlock(&writeSlogMutex);
    close(fd_slog);
    pthread_exit(NULL);
}

void processRequest(int fd_slog, struct client_request *cr)
{
    cr->client_pid = buffer.client_pid;
    cr->num_wanted_seats = buffer.num_wanted_seats;
    intdup(buffer.preferences, cr->preferences, sizeof(buffer.preferences));

    printf("---> thread %d recebeu pid %d\n", getTicketOfficeNum(pthread_self()), cr->client_pid);
    sem_post(&empty);

    pthread_mutex_unlock(&buffer_lock);
    char *answerFifoName = (char *)(malloc(sizeof(char) * (WIDTH_PID + 3 + 1))); // 3-> ans + 1 -> null terminator
    int fd_ans;
    int numReservedSeats = 0;
    struct Seat reservedSeats[cr->num_wanted_seats];

    //Validating request
    int numPreferences = 0, i = 0;
    bool validPreferences = true;

    while (1)
    {
        if (cr->preferences[i] != 0)
        {
            numPreferences++;
            validPreferences &= (cr->preferences[i] >= 1 && cr->preferences[i] <= num_room_seats);
        }
        else
            break;
        i++;
    }

    bool validNumWantedSeats = (cr->num_wanted_seats >= 1) && (cr->num_wanted_seats <= MAX_CLI_SEATS);
    bool validNumPreferences = (numPreferences >= cr->num_wanted_seats) && (numPreferences <= MAX_CLI_SEATS);
    bool fullRoom = (availableSeats == 0);

    struct server_answer ans;
    if (!validNumWantedSeats)
        ans.state = -1;
    else if (!validNumPreferences)
        ans.state = -2;
    else if (!validPreferences)
        ans.state = -3;
    else if (cr->client_pid <= 0)
        ans.state = -4;
    else if (fullRoom)
        ans.state = -6;
    else
    {
        //Attending request
        for (int i = 0; i < numPreferences; i++)
        {
            if (isSeatFree(seats, cr->preferences[i]))
            {
                bookSeat(seats, cr->preferences[i], cr->client_pid);
                reservedSeats[numReservedSeats] = seats[cr->preferences[i] - 1];
                numReservedSeats++;
                if (numReservedSeats == cr->num_wanted_seats)
                    break;
            }
        }
        if (numReservedSeats != cr->num_wanted_seats)
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
            memset(ans.seats, 0, sizeof(ans.seats));
            ans.seats[0] = numReservedSeats;
            printf("numero de lugares reservados: %d\n", numReservedSeats);
            for (int i = 1; i <= numReservedSeats; i++)
            {
                ans.seats[i] = reservedSeats[i - 1].num;
            }
        }
    }
    sprintf(answerFifoName, answerFifoFormat, cr->client_pid);
    fd_ans = open(answerFifoName, O_WRONLY | O_NONBLOCK);
    if (fd_ans == -1) //timeout of client
    {
        printf("sou a thread %d n consegui escrever pid %d\n", getTicketOfficeNum(pthread_self()), cr->client_pid);
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

void sigint_handler(int errno)
{
    close(fd_req);      //close the fifo
    unlink("requests"); //delete the fifo
    exit(0);
}

int main(int argc, char *argv[])
{
    //delete any previous files
    unlink("./slog.txt");
    unlink("./sbook.txt");
    unlink("./clog.txt");
    unlink("./cbook.txt");

    signal(SIGINT, sigint_handler);
    sem_init(&empty, SHARED, 1);
    sem_init(&full, SHARED, 0);

    if (argc != 4)
    {
        printf("--> Invalid argumments\n  --> Usage: server <num_room_seats> <num_ticket_offices> <open_time> \n");
        exit(1);
    }
    num_room_seats = atoi(argv[1]);
    if(num_room_seats > MAX_ROOM_SEATS)
    {
        printf("--> The number of seats in the room cannot be higher than %d\n",MAX_ROOM_SEATS);
        exit(1);
    }

    num_ticket_offices = atoi(argv[2]);
    int open_time = atoi(argv[3]);
    threads = (struct ticket_office *)malloc(sizeof(struct ticket_office) * num_ticket_offices);
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

    for (int i = 0; i < num_ticket_offices; i++)
    {
        ticketOfficeNums[i] = i + 1;
        pthread_create(&threads[i].tid, NULL, ticketOffice, &ticketOfficeNums[i]);
    }

    signal(SIGALRM, alarmHandler);
    alarm(open_time);

    int sval = 0;

    mkfifo("requests", 0660);
    fd_req = open("requests", O_RDONLY);

    //Initializing semaphore
    sem_init(&requestOnWait, SHARED, 0);

    while (true)
    {
        sem_wait(&empty);
        sem_getvalue(&empty, &sval);

        if (read(fd_req, &buffer, sizeof(buffer)) != 0)
        {
            printf("---> server: recebi pid %d\n", buffer.client_pid);
            sem_post(&full);
        }
        else
            sem_post(&empty);
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
        if (threads[i].tid == tid)
            return i + 1;
    }
    return 0;
}

void writeRequestAnswer(int fd_slog, char *ticketOfficeNum, struct client_request *cr, struct server_answer sa)
{
    pthread_mutex_lock(&writeSlogMutex);
    pthread_mutex_lock(&writeSbookMutex);
    int fd_sbook = open("sbook.txt", O_WRONLY | O_APPEND | O_CREAT, DEFAULT_PERMISSION);

    //ticket office num
    write(fd_slog, ticketOfficeNum, strlen(ticketOfficeNum));

    //client pid
    char *client_pid = (char *)malloc(sizeof(char) * (WIDTH_PID + 1));
    sprintf(client_pid, pidFormat, cr->client_pid);
    write(fd_slog, client_pid, strlen(client_pid));
    write(fd_slog, "-", strlen("-"));

    //num tickets wanted
    char *numTicketsWanted = malloc(sizeof(char) * (WIDTH_TICKETS_WANTED + 1 + 1 + 1)); //1-> ' ', 1-> ':' , 1-> null terminator
    sprintf(numTicketsWanted, numTicketsWantedFormat, cr->num_wanted_seats);
    write(fd_slog, numTicketsWanted, strlen(numTicketsWanted));

    //tickets wanted
    char *ticketWanted = malloc(sizeof(char) * (WIDTH_SEAT + 1 + 1)); //1-> ':' , 1-> null terminator
    for (int i = 0; i < MAX_CLI_SEATS; i++)
    {
        if (cr->preferences[i] != 0)
            sprintf(ticketWanted, ticketsFormat, cr->preferences[i]);
        else
            ticketWanted = getSpaceString(WIDTH_SEAT + 2); // 1-> space

        write(fd_slog, ticketWanted, strlen(ticketWanted));
        ticketWanted[0] = '\0';
    }

    //request/answer divisor
    write(fd_slog, "- ", strlen("- "));

    if (sa.state < 0) //request was not successful
    {
        switch (sa.state)
        {
        case -1:
            write(fd_slog, "MAX", strlen("MAX"));
            break;
        case -2:
            write(fd_slog, "NST", strlen("NST"));
            break;
        case -3:
            write(fd_slog, "IID", strlen("IID"));
            break;
        case -4:
            write(fd_slog, "ERR", strlen("ERR"));
            break;
        case -5:
            write(fd_slog, "NAV", strlen("NAV"));
            break;
        case -6:
            write(fd_slog, "FUL", strlen("FUL"));
            break;
        default:
            break;
        }
    }
    else //sucessful reservation
    {
        char *ticketReserved = malloc(sizeof(char) * (WIDTH_SEAT + 1 + 1)); //1-> ' ' , 1-> null terminator
        for (int i = 0; i < cr->num_wanted_seats; i++)
        {
            if (sa.seats[i] != 0)
                sprintf(ticketReserved, ticketsFormat, sa.seats[i]);
            else
                break;

            write(fd_slog, ticketReserved, strlen(ticketReserved));
            write(fd_sbook, ticketReserved, strlen(ticketReserved));
            write(fd_sbook, "\n", strlen("\n"));
            ticketReserved[0] = '\0';
        }
    }
    write(fd_slog, "\n", strlen("\n"));
    close(fd_sbook);
    pthread_mutex_unlock(&writeSbookMutex);
    pthread_mutex_unlock(&writeSlogMutex);
}

char *getSpaceString(int n)
{
    char *spaceString = (char *)malloc(sizeof(char) * (1 + n));
    for (int i = 0; i < n; i++)
    {
        sprintf(spaceString, " ");
    }
    return spaceString;
}
