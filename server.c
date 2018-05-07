#include "server.h"

pthread_t *threads;
int fd_req, num_room_seats;
struct client_request buffer;
sem_t requestOnWait;
pthread_mutex_t buffer_lock = PTHREAD_MUTEX_INITIALIZER;
struct Seat *seats;

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
    seats[seatNum - 1].client = clientId;
    DELAY();
}

void freeSeat(struct Seat *seats, int seatNum)
{
    seats[seatNum - 1].client = 0;
    DELAY();
}

void *ticketOffice(void *arg)
{
    signal(SIGTERM, termHandler);
    struct client_request cr;

    while (true)
    {
        //Locking buffer to wait for request
        pthread_mutex_lock(&buffer_lock);
        sem_wait(&requestOnWait);
        cr.client_pid = buffer.client_pid;
        cr.num_wanted_seats = buffer.num_wanted_seats;
        intdup(buffer.preferences, cr.preferences, sizeof(buffer.preferences));
        pthread_mutex_unlock(&buffer_lock);

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

        struct server_answer ans;
        if (!validNumWantedSeats)
            ans.state = -1;
        else if (!validNumPreferences)
            ans.state = -2;
        else if (!validPreferences)
            ans.state = -3;
        else if (cr.client_pid <= 0)
            ans.state = -4;
        //missing the case of full room

        //Attending request
        struct Seat *reservedSeats[cr.num_wanted_seats];
        int numReservedSeats = 0;

        for (int i = 0; i < numPreferences; i++)
        {
            if (isSeatFree(seats, cr.preferences[i]))
            {
                bookSeat(seats, cr.preferences[i], cr.client_pid);
                reservedSeats[numReservedSeats] = cr.preferences[i];
                numReservedSeats++;
                if (numReservedSeats == cr.num_wanted_seats)
                break;
            }
        }
        if (numReservedSeats != cr.num_wanted_seats){
            ans.state = -5;
            for (int i = 0 ; i < numReservedSeats; i++){
                freeSeat(seats, reservedSeats[i]);
            }
        }
        else { 
            ans.state = 0;
            ans.seats = (struct Seat *) malloc(sizeof(struct Seat) * (numReservedSeats+1)); //the number of reserved seats + the seats
            ans.seats[0] = numReservedSeats;
            for(int i = 1; i <= numReservedSeats; i++){
                ans.seats[i] = reservedSeats[i-1];
            }
        }

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
        pthread_create(&threads[i], NULL, ticketOffice, NULL);
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