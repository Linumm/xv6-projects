// Test for checking CFS scheduling stats

#include "types.h"
#include "stat.h"
#include "user.h"

#define N 10
#define SUM 1000000

void
cfs_test()
{
  int n, pid;

  printf(1, "CFS test with %d\n", N);

  for(n=0; n<N; n++){
	pid = fork();
	if(pid < 0){
	  printf(1, "fork error!\n");
	  break;
	}
	if(pid == 0){
	  int j = 0;
	  for(; j<SUM; j++){}
	  exit();
	}
  }

  if(n == N){
	sleep(10000);
	exit();
  }
}


int
main(void)
{
  cfs_test();
  exit();
}
