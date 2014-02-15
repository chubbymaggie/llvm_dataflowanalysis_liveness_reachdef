#include <stdlib.h>

int sum (int a, int b) {
  int i;
  int res = 1;
  /*
  int *pt = malloc(sizeof(int));
  pt[0] = 0;
*/
  for (i = a; i < b; i++) {
    res *= i;
	//pt[0] += i;
  }

  return res;
}
/*
int main() {
  int a, b, res;
  a = 1;
  b = 5;
  res = sum(a, b);
  return 0;
}
*/
