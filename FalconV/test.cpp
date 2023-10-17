#include <stdio.h>
#include <stdlib.h>


struct Car {
	std::string name;
	int year;

	Car(std::string n, int y) {
		name = n;
		year = y;
	}

	virtual std::string modelMake() = 0;
};

int main() {
	printf("Hello, World!\n");
	return 0;
}