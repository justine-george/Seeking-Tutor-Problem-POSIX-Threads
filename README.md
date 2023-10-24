# 🧑‍🏫🧑‍🎓🏫 Seeking-Tutor-Problem

This repo is the implementation of a solution that synchronizes the activities 
of the coordinator, tutors, and students using POSIX threads, mutex 
locks, and semaphores. Original problem described below.

The computer science department runs a mentoring center (csmc) to 
help undergraduate students with their programming assignments. 
The lab has a coordinator and several tutors to assist the students. 
The waiting area of the center has several chairs. Initially, all 
the chairs are empty. The coordinator is waiting for the students 
to arrive. The tutors are either waiting for the coordinator to 
notify that there are students waiting or they are busy tutoring. 
The tutoring area is separate from the waiting area.

A student while programming for their project, decides to go to 
csmc to get help from a tutor. After arriving at the center, the 
student sits in an empty chair in the waiting area and waits to be 
called for tutoring. If no chairs are available, the student will 
go back to programming and come back to the center later. Once a 
student arrives, the coordinator queues the student based on the 
student’s priority (details on the priority discussed below), and 
then the coordinator notifies an idle tutor. A tutor, once woken up, 
finds the student with the highest priority and begins tutoring. A 
tutor after helping a student, waits for the next student. A 
student, after receiving help from a tutor goes back to programming.

The priority of a student is based on the number of times the 
student has taken help from a tutor. A student visiting the center 
for the first time gets the highest priority. In general, a student 
visiting to take help for the ith time has a priority higher than 
the priority of the student visiting to take help for the kth time 
for any k > i. If two students have the same priority, then the 
student who came first has a higher priority.

## Compilation format
```
gcc csmc.c -o csmc -Wall -Werror -pthread -std=gnu99
```

## Input format
```
./csmc #students #tutors #chairs #help
```

## Sample input
```
./csmc 2 2 2 1
```

## Sample output
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
