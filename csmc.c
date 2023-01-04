// Seeking Tutor Problem
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>
#include <semaphore.h>
#include "common_threads.h"

void* studentThread(void *);
void* coordinatorThread(void *);
void* tutorThread(void *);
int validateInput(int, int, int, int);
 
struct Student {
    int id; // numbered from 1 to studentCount
    int helpReceived; // initially 0
    int tokenNo; // increment this when entering waiting room
} *sArr;

struct StdValid {
    struct Student s;
    int valid; // 1 if valid, 0 if invalid
    int tId; // tutorId of the tutor that tutors the student
} **queue; // 2D array of StdValid, helpCount rows and studentCount columns

struct Tutor {
    int id; // numbered from 1,2,.. to tutorCount
} *tArr;

pthread_mutex_t chairLock, waitingStdCountLock, currStdLock;
pthread_mutex_t tGlbVarLock, queueLock;
sem_t studentWaiting;
sem_t *tutorAgreed, *tutoringStart, *tutoringEnd; // list of semaphores
sem_t placedOnQueueStudent, placedOnQueueTutor;

// global variables related to tutors
struct TutorGlobalVars {
    int activeTutors;
    int totalTutoredCount; 
} tGlbVar;

struct Student currStd;
int studentCount;
int tutorCount;
int freeChairCount;
int helpCount;
int globalArrivalToken = 0;
int waitingStdCount = 0;
int totalReqCount = 0;

// Main procedure
int main(int argc, char *argv[]) {                    
    if (argc != 5) {
	fprintf(stderr, "usage: csmc #students #tutors #chairs #help\n");
	exit(1);
    }

    studentCount = atoi(argv[1]);
    tutorCount = atoi(argv[2]);
    freeChairCount = atoi(argv[3]);
    helpCount = atoi(argv[4]);

    // validate the arguments received
    if (validateInput(studentCount, tutorCount, freeChairCount, helpCount) == 1)
        exit(1);


    pthread_t *student, *tutor, coordinator;

    // time as seed for the random number generator
    srand(time(NULL));

    // set up tutor specific global variables
    tGlbVar.activeTutors = 0;
    tGlbVar.totalTutoredCount = 0;

    // global variable currStd for the coordinator to know the student and place it in the queue
    currStd.id = -1;
    currStd.helpReceived = -1;
    currStd.tokenNo = -1; 

    // allocate memory for the MLFQ struct StdValid queue[helpCount][studentCount]
    queue = malloc(sizeof(struct StdValid*) * helpCount);
    int i,j;
    for (i = 0; i < helpCount; i++) {
        queue[i] = malloc(sizeof(struct StdValid) * studentCount);
	for (j = 0; j < studentCount; j++) {
	    queue[i][j].valid = 0; // all values are invalid by default on the queue.
	}
    }

    // initialize the mutex locks
    // lock that needs to be acquired before updating freeChairCount
    Pthread_mutex_init(&chairLock, NULL);
    // lock that needs to be acquired before updating tGlbVar
    Pthread_mutex_init(&tGlbVarLock, NULL);
    // lock that needs to be acquired before updating MLFQ **Queue 
    Pthread_mutex_init(&queueLock, NULL);
    // lock that needs to be acquired before updating waitingStudentCount 
    Pthread_mutex_init(&waitingStdCountLock, NULL);
    // lock that needs to be acquired before updating current student global variable
    Pthread_mutex_init(&currStdLock, NULL);

    // initialize the semaphores
    // signal sent by the student to the coordinator
    sem_init(&studentWaiting, 0, 0);
    // signal sent by the coordinator to the student
    sem_init(&placedOnQueueStudent, 0, 0);
    // signal sent by the coordinator to the tutor
    sem_init(&placedOnQueueTutor, 0, 0);
    
    // tutor signals the student with id sId using tutorAgreed[sId-1] that the tutor is ready
    tutorAgreed = malloc(sizeof(sem_t) * studentCount);
    for (i = 0; i < studentCount; i++) { // list of semaphore of size studentCount
        sem_init(&tutorAgreed[i], 0, 0); // initialize with value 0
    }
    // student with id sId signals the tutor using tutoringStart[sId-1] to start tutoring 
    tutoringStart = malloc(sizeof(sem_t) * studentCount);
    for (i = 0; i < studentCount; i++) { // list of semaphore of size studentCount
        sem_init(&tutoringStart[i], 0, 0); // initialize with value 0
    }
    // tutor signals the student with id sId using tutoringEnd[sId-1] that tutoring session is done  
    tutoringEnd = malloc(sizeof(sem_t) * studentCount);
    for (i = 0; i < studentCount; i++) { // list of semaphore of size studentCount
        sem_init(&tutoringEnd[i], 0, 0); // initialize with value 0
    }
    
    // start studentCount number of student threads
    student = malloc(sizeof(pthread_t) * studentCount);
    sArr = malloc(sizeof(struct Student) * studentCount);
    for (i = 0; i < studentCount; i++) {
	sArr[i].id = i+1;
	sArr[i].helpReceived = 0;
	sArr[i].tokenNo = -1; // default value
	Pthread_create(&student[i], NULL, studentThread, &sArr[i]);
    }

    //start a single coordinator thread
    Pthread_create(&coordinator, NULL, coordinatorThread, NULL);

    // start tutorCount number of tutor threads
    tutor = malloc(sizeof(pthread_t) * tutorCount);
    tArr = malloc(sizeof(struct Tutor) * tutorCount);
    for (i = 0; i < tutorCount; i++) {
	tArr[i].id = i+1;
	Pthread_create(&tutor[i], NULL, tutorThread, &tArr[i]);
    }

    // join waits for the threads to finish executing
    for (i = 0; i < studentCount; i++) {
    	Pthread_join(student[i], NULL); 
    }
    Pthread_join(coordinator, NULL);
    for (i = 0; i < tutorCount; i++) {
    	Pthread_join(tutor[i], NULL); 
    }
    return 0;
}

// Student thread procedure
void* studentThread(void* arg) {
    struct Student *s = (struct Student*)arg;
    do {
	float sTime =  (float)(rand() % 200) / 1000; // sleep upto 2 ms while programming
	Pthread_mutex_lock(&chairLock);
	if (freeChairCount != 0) {
	    freeChairCount--; // occupy a chair
	   
	    // tokenNo assume values from 0,1, to studentCount - 1
	    s->tokenNo = globalArrivalToken % studentCount;
	    globalArrivalToken++;
	    
	    printf("S: Student %d takes a seat. Empty chairs = %d.\n", s->id, freeChairCount);
	    Pthread_mutex_unlock(&chairLock);
	
	    Pthread_mutex_lock(&waitingStdCountLock);
	    waitingStdCount++; // one more student is waiting
	    Pthread_mutex_unlock(&waitingStdCountLock);

	    Pthread_mutex_lock(&currStdLock);
	    // save current student details to the global variable currStd struct
	    currStd.id = s->id;
	    currStd.helpReceived = s->helpReceived;
	    currStd.tokenNo = s->tokenNo;
	    // wake up coordinator
	    sem_post(&studentWaiting);
	    // waits for the coordinator to place the student on the queue
	    sem_wait(&placedOnQueueStudent); 
	    // coordinator placed student on the queue, release lock
	    Pthread_mutex_unlock(&currStdLock);

	    // wait for tutor's reply
	    sem_wait(&tutorAgreed[(s->id) - 1]);

	    // get up from the chair
	    Pthread_mutex_lock(&chairLock);
	    freeChairCount++;
	    Pthread_mutex_unlock(&chairLock);
	    
	    // this student is no longer waiting
	    Pthread_mutex_lock(&waitingStdCountLock);
	    waitingStdCount--;
	    Pthread_mutex_unlock(&waitingStdCountLock);
	   
	    // one more active tutor present
	    Pthread_mutex_lock(&tGlbVarLock);
	    tGlbVar.activeTutors++;
	    Pthread_mutex_unlock(&tGlbVarLock);

	    // leave to get tutored
	    // ask tutor to start the tutoring
	    sem_post(&tutoringStart[(s->id) - 1]);
	    
	    // wait for the session to end
	    sem_wait(&tutoringEnd[(s->id) - 1]);
	   
	    // received help from tutor y
	    // increment help Received counter
	    s->helpReceived++;
	    printf("S: Student %d received help from Tutor %d.\n",
			    s->id, queue[s->helpReceived - 1][s->tokenNo].tId);
	    
	    // return back to programming
	    usleep(sTime);
	} else {
	    printf("S: Student %d found no empty chair. Will try again later.\n", s->id);
	    
	    // return back to programming
	    usleep(sTime);
	    
	    Pthread_mutex_unlock(&chairLock);
	}
    } while(s->helpReceived != helpCount);

    Pthread_mutex_lock(&tGlbVarLock);
    if (tGlbVar.totalTutoredCount >= studentCount * helpCount) {
        Pthread_mutex_unlock(&tGlbVarLock);
        exit(0);
    }
    Pthread_mutex_unlock(&tGlbVarLock);
   
    return NULL;
}

// Coordinator thread procedure
void* coordinatorThread(void* arg) {
    while(1) {
       	// wait for the student's signal once they arrive
	sem_wait(&studentWaiting);

	// increment the totalReqCount
	totalReqCount++;

	// place student on the 2D array struct Student queue[helpCount][studentCount]
	// the queue positions itself have a priority assigned to them.
	// priority decreases from index 0 to studentCount left to right
	// priority decreases from index 0 to helpCount top to down
	// coordinator have the student details from the currStd global variable
	if (currStd.helpReceived < helpCount && currStd.tokenNo < studentCount) {
	    queue[currStd.helpReceived][currStd.tokenNo].s.id = currStd.id;
	    queue[currStd.helpReceived][currStd.tokenNo].s.helpReceived = currStd.helpReceived;
	    queue[currStd.helpReceived][currStd.tokenNo].s.tokenNo = currStd.tokenNo;
	    // set valid bit to 1 for tutor's reference
	    queue[currStd.helpReceived][currStd.tokenNo].valid = 1;
	}
	
	printf("C: Student %d with priority %d added to the queue. Waiting students now = %d. Total requests = %d\n",
			currStd.id, currStd.helpReceived, waitingStdCount, totalReqCount);

	// Notify the student who called coordinator. Student proceeds to wait for the tutor's reply.
	// Tutors are also notified.
	sem_post(&placedOnQueueStudent);
	sem_post(&placedOnQueueTutor);

	// loop is exited when totalReqCount is studentCount times helpCount
	if (totalReqCount >= studentCount * helpCount) {	    
	    break;
	}
    }
    return NULL;
}

// Tutor thread procedure
void* tutorThread(void* arg) {
    struct Tutor *t = (struct Tutor*)arg;
    int viSId; // viSId (a spin on vip) is the student id of the top priority student

    while(1) {    
	// wait for the coordinator's signal
	sem_wait(&placedOnQueueTutor);
	
	Pthread_mutex_lock(&queueLock);
	// find the student with the highest priority and wakes them up, tutor them.
	int i, j;
	for (i = 0; i < helpCount; i++) {
	    viSId = -1;
	    for (j = 0; j < studentCount; j++) {
	        if (queue[i][j].valid == 1) { // first entry with valid bit 1 will be the student with top priority  
		    viSId = queue[i][j].s.id; // retrieve the student id and assign to viSId
		    queue[i][j].tId = t->id; // Tutor signs on the queue entry with tutor's id
		    queue[i][j].valid = 0; // make this entry invalid
		    break;
		}
	    }
	    if (viSId != -1) { // entry found
	        break;
	    }
	}
	Pthread_mutex_unlock(&queueLock);

	// wake up the student with id viSId
	sem_post(&tutorAgreed[viSId - 1]); 

	// wait for the consent of the student with id viSId
	sem_wait(&tutoringStart[viSId - 1]);

	sleep(0.0002); // tutor away!
	
	Pthread_mutex_lock(&tGlbVarLock);
	// one less active tutor
	tGlbVar.activeTutors--;
	// increase total tutored count
	tGlbVar.totalTutoredCount++;
	Pthread_mutex_unlock(&tGlbVarLock);

	if (viSId != -1) {
	    printf("T: Student %d tutored by Tutor %d. Students tutored now = %d. Total sessions tutored = %d\n",
			viSId, t->id, tGlbVar.activeTutors, tGlbVar.totalTutoredCount);
	}

	// signal student with id viSId that the session is done.
    	sem_post(&tutoringEnd[viSId - 1]);

	// Loop exits when totalTutoredCount is studentCount times helpCount
	Pthread_mutex_lock(&tGlbVarLock);
	if (tGlbVar.totalTutoredCount >= studentCount * helpCount) {
	    Pthread_mutex_unlock(&tGlbVarLock);
	    break;
	}
	Pthread_mutex_unlock(&tGlbVarLock);
    }
    return NULL;
}

// Input argument validator procedure
// Returns 1 if error, else 0
int validateInput(int studentCount, int tutorCount, int freeChairCount, int helpCount) {
    int errFound = 0;
    if (studentCount < 1) {
	fprintf(stderr, "There should be atleast one student\n");
	errFound = 1;
    }
    if (tutorCount < 1) {
	fprintf(stderr, "There should be atleast one tutor\n");
	errFound = 1;
    }
    if (freeChairCount < 1) {
	fprintf(stderr, "There should be atleast one chair\n");
	errFound = 1;
    }
    if (helpCount < 1) {
	fprintf(stderr, "Help should be asked atleast once\n");
	errFound = 1;
    }
    return errFound;
}
