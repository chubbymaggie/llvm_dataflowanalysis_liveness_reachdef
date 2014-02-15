#include <stdio.h>
#include <stdlib.h>

int compare (int a, int b) {
  if (a > b) {
    return a;
  }
  else {
    return b;
  }
}

int main () {
  int a = 2;
  int b = 1;
  int result;

  result = compare(a, b);

  return 0;
}
