// Test that fork fails gracefully.
// Tiny executable so that the limit can be filling the proc table.

#include "types.h"
#include "stat.h"
#include "user.h"

#define N_CHILD 5
#define N_WORK 1000000

void
work(int child_id)
{
  volatile int i;
  volatile int sum = 0;

  for(i = 0; i < N_WORK; i++)
    sum += i;

  printf(1, "work(%d) done with sum = %d\n", child_id, sum);
}


void
cfstest(void)
{
  int i;
  int start_ticks, end_ticks;
  int pid[N_CHILD];
  int runtime[N_CHILD] = {0};

  printf(1, "CFS scheduler test started\n");
  
  for(i = 0; i < N_CHILD; i++) {
    pid[i] = fork();
    if (pid[i] < 0) {
      printf(1, "fork failed: %dth iter\n", i);
      exit();
    }
    else if (pid[i] == 0) {
      // Child
      start_ticks = uptime();
      work(i);
      end_ticks = uptime();
      runtime[i] = end_ticks - start_ticks;
      printf(1, "Child %d consumed: %d\n", i, runtime[i]);
      exit();
    }

    // Parent
    for(i = 0; i < N_CHILD; i++) {
      if(wait() < 0) {
        printf(1, "Wait failed for child %d\n", i);
        exit();
      }
    }
  }

  // Summary
  printf(1, "[Summary]\n");
  for(i = 0; i < N_CHILD; i++)
    printf(1, "Child %d runtime: %d\n", i, runtime[i]);

  printf(1, "CFS scheduler test OK\n");
}


int
main()
{
  cfstest();
  exit();
}