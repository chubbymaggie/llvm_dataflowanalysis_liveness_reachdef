int sum (int a, int b) {
int i;
int res = 1;
for (i = a; i < b; i++) {
res *= i;
}
return res;
}

int sum1 (int a, int b, int c) {
    int d,e;
    if (c > 0) {
        d = a;
        e = b;
    } else {
        d = b;
        e = a;
    }
    return d + e;
}

int sum2 (int a, int b) {
  int i;
  int res = 1;

  for (i = a; i < b; i++) {
    res *= i;
    if (res % 3 == 0) {
      res = -1;
    }
  }

  return res;
}

int sum3 (int b, int d) {
int a = b + d;
 	int c;
if (a < 0)
  c = b + 1; //path1
else
  c = d + 1;  //path2
return c; //this will be ret = phi(c_path1, c_path2)
}

int sum4(int a, int b) {
int c = a - b;
switch (a) {
case 0: c = a + b; break;
case 1: c = a * b; break;
default: c = a - b; break; 
}
if (c == a) {
return 1;
} else {
return 2;
}
}

int sum5(int a, int b) {
int c;
for (int i = a; i < b + a; i ++) {
for (int j = b; j < a + b; j ++) {
c = i + j;
}
}
int result = c + a;
return result;
}
