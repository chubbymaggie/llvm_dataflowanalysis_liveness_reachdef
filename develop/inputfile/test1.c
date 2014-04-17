int sum (int a, int b, int c) {
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
