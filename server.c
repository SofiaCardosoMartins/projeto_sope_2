#include "server.h"

pthread_t *threads;
int fd_req, num_room_seats;
struct client_request buffer;
sem_t requestOnWait;
pthread_mutex_t buffer_lock = PTHREAD_MUTEX_INITIALIZER;
struct Seat *seats;
int availableSeats;
bool * processingRequestsTicketOffices;

void termHandler(int signo)
{
    if (signo != SIGTERM)
        return;

    
    //Handling signal so that if the ticketOffice terminates attending the last client
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
}

int isSeatFree(struct Seat *seats, int seatNum)
{
    int freeSeat = (seats[seatNum - 1].client == 0);
    DELAY();
    return freeSeat;
}

void bookSeat(struct Seat *seats, int seatNum, int clientId)
{
    availableSeats--;
    seats[seatNum - 1].client = clientId;
    DELAY();
}

void freeSeat(struct Seat *seats, int seatNum)
{
    availableSeats++;
    seats[seatNum - 1].client = 0;
    DELAY();
}

void *ticketOffice(void *arg)
{
    signal(SIGTERM, termHandler);
    struct client_request cr;
    int officeNum = *((int *) arg);
    int fd_slog = open("slog.txt", O_WRONLY | O_APPEND | O_CREAT, DEFAULT_PERMISSION);
    char * ticketOfficeNum = (char* ) malloc(sizeof(char)*(WIDTH_TICKET_OFFICE+1+1)); //1-> '-' + 1-> null terminator
    sprintf(ticketOfficeNum, ticketOfficeNumFormat, officeNum);
   
   //Writting open state to file
    write(fd_slog, ticketOfficeNum, sizeof(char)*(WIDTH_TICKET_OFFICE+1+1));
    wrIte(fd_slog, "OPEN\n", sizeof("OPEN\n"));


    while (true)
    {
        //Locking buffer to wait for request
        pthread_mutex_lock(&buffer_lock);
        sem_wait(&requestOnWait);

        //ticket office just started processing a request
        processingRequestsTicketOffices[officeNum-1] = true;

        cr.client_pid = buffer.client_pid;
        cr.num_wanted_seats = buffer.num_wanted_seats;
        intdup(buffer.preferences, cr.preferences, sizeof(buffer.preferences));
        pthread_mutex_unlock(&buffer_lock);
        char *answerFifoName = (char *)(malloc(sizeof(char) * (WIDTH_PID + 3 + 1))); // 3-> ans + 1 -> null terminator
        int fd_ans;

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

        //Attending request
        struct Seat reservedSeats[cr.num_wanted_seats];
        int numReservedSeats = 0;

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
                freeSeat(seats, &reservedSeats[i]);
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
        sprintf(answerFifoName, answerFifoFormat, cr.client_pid);
        fd_ans = open(answerFifoName, O_WRONLY);
        if (fd_ans == -1)
        { //timeout do client
            for (int i = 0; i < numReservedSeats; i++)
            {
                freeSeat(seats, &reservedSeats[i]);
            }
            numReservedSeats = 0;
        }
        else
            write(fd_ans, &ans, sizeof(ans));


        //ticket office just ended processing a request
        processingRequestsTicketOffices[officeNum-1] = false;
    }
}

int main(int argc, char *argv[])
{
    if (argc != 4)
        printf("Invalid argumments\n Usage: server <num_room_seats> <num_ticket_offices> <open_time> \n");

    num_room_seats = atoi(argv[0]);
    int num_ticket_offices = atoi(argv[1]);
    int open_time = atoi(argv[2]);
    threads = (pthread_t *)malloc(sizeof(pthread_t) * num_ticket_offices);
    availableSeats = num_room_seats;
    int ticketOfficeNums[num_ticket_offices];
    processingRequestsTicketOffices = (bool*) malloc(sizeof(bool)*num_ticket_offices);
    memset(processingRequestsTicketOffices, false, sizeof(processingRequestsTicketOffices));

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
        ticketOfficeNums[i] = i +1 ;
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