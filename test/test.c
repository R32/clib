#include <stdio.h>

void kuai_test(int n);
void pmap_test(int n); // test/pmap_test.c
void buffer_test();
void ucs2_test();

int main(int argc, char *argv[])
{
	kuai_test(3);
	pmap_test(3);
	buffer_test();
	ucs2_test();
	printf("done!\n");
	return 0;
}
