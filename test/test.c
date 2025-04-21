#include <stdio.h>

void pmap_test(int n); // test/pmap_test.c
void buffer_test();

int main(int argc, char *argv[])
{
	pmap_test(3);
	buffer_test();
	printf("done!\n");
}