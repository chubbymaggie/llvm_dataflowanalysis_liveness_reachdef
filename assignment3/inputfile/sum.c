int sum (int a, int b) {
	//int i, c;
	
	int i;
	int res = 1;
	//a = a + 1;
	for (i = a; i < b; i++) {
		//c = a + 1;
		res *= i;
	}
	//return res + c;
	return res;
}
