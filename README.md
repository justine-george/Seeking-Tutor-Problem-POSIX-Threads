# Multi-threaded POSIX Simulation of a Mentoring Center with Priority Scheduling (aka Seeking-Tutor-Problem)

This repository contains an implementation that models and synchronizes the activities of a coordinator, tutors, and students in a mentoring center using POSIX threads, mutex locks, and semaphores, as per the original problem described below.

## Problem Description

In the computer science department's mentoring center (csmc), there are coordinators, tutors, and students. The center is equipped with a waiting area with several chairs and a separate tutoring area. The coordinators and tutors manage the flow of students based on their need for assistance and priority.

Students seeking help from tutors arrive at the center and wait in the available chairs. If no chairs are free, they return later. The coordinator organizes the students based on their priority, which depends on the frequency of their visits: first-time visitors have the highest priority. Students with the same priority level are queued based on their arrival time. Tutors, after being notified by the coordinator, assist the students with the highest priority.

## Implementation Details

The implementation uses:

- POSIX threads to simulate the concurrent activities of coordinators, tutors, and students
- Mutex locks to manage access to shared resources like chairs
- Semaphores to synchronize the interaction between students, tutors, and the coordinator

## Compilation Format

Compile the program using the following command:

```bash
gcc csmc.c -o csmc -Wall -Werror -pthread -std=gnu99
```

## Running the Program

To execute the program, use the following format:

```bash
./csmc #students #tutors #chairs #help
```

## Sample Usage

### Input

```bash
./csmc 2 2 2 1
```

### Output

````
S: Student 1 takes a seat. Empty chairs = 1.  
S: Student 2 takes a seat. Empty chairs = 0.  
C: Student 1 with priority 0 added to the queue. Waiting students now = 1. Total requests = 1  
T: Student 1 tutored by Tutor 1. Students tutored now = 0. Total sessions tutored = 1  
S: Student 1 received help from Tutor 1.  
C: Student 2 with priority 0 added to the queue. Waiting students now = 1. Total requests = 2  
T: Student 2 tutored by Tutor 2. Students tutored now = 0. Total sessions tutored = 2  
S: Student 2 received help from Tutor 2.
````
