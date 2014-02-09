#include <iostream>
#include "test2.cpp"
#include "test3.cpp"
class B: public A<int>, public C<float>{
	public:
		B(): A<int>(), C<float>() {}
};

int main() {
	A<int> x;
	B y;
	return 0;
}
