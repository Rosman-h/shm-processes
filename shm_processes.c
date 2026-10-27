   #include <stdio.h>
   #include <stdlib.h>
   #include <unistd.h>
   #include <time.h>
   #include <sys/types.h>
   #include <sys/ipc.h>
   #include <sys/shm.h>
   #include <sys/wait.h>

   #define LOOPS 25

   void ParentProcess(volatile int *shm);
   void ChildProcess(volatile int *shm);

   int main(void)
   {
       int shmid;
       volatile int *ShmPTR;   /* [0] = BankAccount, [1] = Turn */
       pid_t pid;

       /* shared memory for two ints */
       shmid = shmget(IPC_PRIVATE, 2 * sizeof(int), IPC_CREAT | 0666);
       if (shmid < 0)
       {
           perror("shmget");
           exit(1);
       }

       ShmPTR = (volatile int *)shmat(shmid, NULL, 0);
       if (ShmPTR == (volatile int *)-1)
       {
           perror("shmat");
           exit(1);
       }

       ShmPTR[0] = 0;   /* BankAccount */
       ShmPTR[1] = 0;   /* Turn */

       srandom(time(NULL));

       pid = fork();
       if (pid < 0)
       {
           perror("fork");
           exit(1);
       }

       if (pid == 0)
       {
           ChildProcess(ShmPTR);
           shmdt((void *)ShmPTR);
           exit(0);
       }

       ParentProcess(ShmPTR);
       wait(NULL);

       shmdt((void *)ShmPTR);
       shmctl(shmid, IPC_RMID, NULL);
       return 0;
   }

   /* Dear Old Dad */
   void ParentProcess(volatile int *shm)
   {
       for (int i = 0; i < LOOPS; i++)
       {
           sleep(random() % 6);               /* 0 - 5 seconds */

           int account = shm[0];

           while (shm[1] != 0)
               ;                              /* strict alternation: wait for my turn */

           account = shm[0];                  /* re-read so we never use a stale balance */

           if (account <= 100)
           {
               int balance = random() % 100 + 1;
               if (balance % 2 == 0)
               {
                   account += balance;
                   printf("Dear old Dad: Deposits $%d / Balance = $%d\n", balance, account);
               }
               else
               {
                   printf("Dear old Dad: Doesn't have any money to give\n");
               }
               shm[0] = account;
           }
           else
           {
               printf("Dear old Dad: Thinks Student has enough Cash ($%d)\n", account);
           }
           fflush(stdout);

           shm[1] = 1;                        /* Student's turn */
       }
   }

   /* Poor Student */
   void ChildProcess(volatile int *shm)
   {
       srandom(time(NULL) ^ getpid());        /* different numbers than the parent */

       for (int i = 0; i < LOOPS; i++)
       {
           sleep(random() % 6);               /* 0 - 5 seconds */

           int account = shm[0];

           while (shm[1] != 1)
               ;                              /* strict alternation: wait for my turn */

           account = shm[0];                  /* re-read so we never use a stale balance */

           int balance = random() % 50 + 1;
           printf("Poor Student needs $%d\n", balance);

           if (balance <= account)
           {
               account -= balance;
               printf("Poor Student: Withdraws $%d / Balance = $%d\n", balance, account);
           }
           else
           {
               printf("Poor Student: Not Enough Cash ($%d)\n", account);
           }
           fflush(stdout);

           shm[0] = account;
           shm[1] = 0;                        /* Dad's turn */
       }
   }