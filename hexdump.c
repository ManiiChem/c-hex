#include <stdio.h>
#include <stdlib.h>

void print_binary (const size_t bytes_to_read, unsigned char *binary_data);

int main (int argc, char *argv[]) {
	if (argc != 2) {
		printf("Enter the name of the file you want to hexdump\n");
		exit(EXIT_FAILURE);
	}

	size_t fread_count = 0;
	FILE* fp = fopen(argv[1], "rb");

	if (fp == NULL) {
		printf("Failed to open file. Did you spell it correctly? (Extensions must be included)\n");
		exit(EXIT_FAILURE);
	}

	unsigned char buff[1024]; // unsigned char data type to hold binary cuz its guaranteed to be 1 byte (8 bits) long on all standard systems
	while ((fread_count = fread(buff, sizeof(char), sizeof(buff), fp)) > 0) { // sizeof(char) is one but for the sake of readability ive included it
		print_binary(fread_count, buff);
	}
	
	fclose(fp);
	return 0;
}

void print_binary (const size_t bytes_to_read, unsigned char *binary_data) {
	for (int i = 0; i < bytes_to_read; i++) { // // using 'count' so u dont read more than the count size and read garbage data
		// loop through the 8 bits of each byte
		for (int j = 7; j > -1; j--) {
			printf("%d", (binary_data[i] >> j) & 1); // shift each bit starting from the leftmost bit, then isolate only the right most bit and discard anything to the left of it
		}
		printf(" ");
	}
}