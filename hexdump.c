#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BUFF_BYTES 1024
#define BUFF_BYTES_HEX 16 // formats better in the terminal

void print_binary (const size_t bytes_to_read, unsigned char *binary_data);
void read_file_binary (FILE* fp);
void print_hex (const size_t bytes_to_read, unsigned char* binary_data, size_t mem_off);
void read_file_hex (FILE* fp);
void cli_syntax (void);

int main (int argc, char *argv[]) {
	if (argc < 2 || argc > 3) {
		cli_syntax();
	}
	
	int mode = 0;

	if (argc == 3) {
		if (strcmp(argv[1], "binary") == 0) {
			mode = 0;
		}
		else if (strcmp(argv[1], "hex") == 0) {
			mode = 1;
		}
		else {
			cli_syntax();
		}
	}

	FILE* fp = fopen(argv[argc-1], "rb"); // argc-1 means the last CLI arg is the file name
	if (fp == NULL) {
		printf("Failed to open file. Did you spell it correctly? (Extensions must be included)\n");
		exit(EXIT_FAILURE);
	}

	if (mode == 0) {
		read_file_binary(fp);
	}
	else
	{
		read_file_hex(fp);
	}
	
	fclose(fp);
	return 0;
}

void print_binary (const size_t bytes_to_read, unsigned char *binary_data) {
	char buff_output[(BUFF_BYTES * 9) + 1]; // each char has 8 bits, +1 is for the NULL terminator and the +1 to x8 for the whitespace lol
	int buff_index = 0;
	for (size_t i = 0; i < bytes_to_read; i++) { // // using 'count' so u dont read more than the count size and read garbage data
		// loop through the 8 bits of each byte
		for (int j = 7; j > -1; j--) {
			// shift each bit starting from the leftmost bit, then isolate only the right most bit and discard anything to the left of it
			// basically if ((binary_data[i] >> j) & 1) which is 1 if its add '1' otherwise add '0' to the buffer
			buff_output[buff_index++] = ((binary_data[i] >> j) & 1) ? '1' : '0';	
		}
		buff_output[buff_index++] = ' ';
	}

	buff_output[buff_index] = '\0';
	fputs(buff_output, stdout); // alot faster than printf cuz of no format specifiers
}

void read_file_binary (FILE* fp) {
	size_t fread_count = 0;

	unsigned char buff[BUFF_BYTES]; // unsigned char data type to hold binary cuz its guaranteed to be 1 byte (8 bits) long on all standard systems
	while ((fread_count = fread(buff, sizeof(char), sizeof(buff), fp)) > 0) { // sizeof(char) is one but for the sake of readability ive included it
		print_binary(fread_count, buff);
	}
}

void print_hex (const size_t bytes_to_read, unsigned char* binary_data, size_t mem_off) {
	char buff_output[(BUFF_BYTES_HEX * 3) + 1]; // one hex is equal to 4 bits so * 2 for the whole byte + (*1) for the whitespace and +1 for the NULL terminator
	int buff_index = 0;

	// 32bit max so this tool technically doesnt support hexediting and hexdumping files larger than 4GB as of now, easy fix tho maybe ill add a flag in the future
	printf("%08zX    ", mem_off);
	for (size_t i = 0; i < bytes_to_read; i++) {
		// overwriting the null terminator by adding len to buff_index, also sprintf is secure in this case so no need to use snprintf
		unsigned int len = sprintf(&buff_output[buff_index], "%02X ", binary_data[i]); // this adds a null terminator
		buff_index += len;
	}

	// sprintf automatically adds a '\0' so we dont need to add it here..
	fputs(buff_output, stdout);
	printf("\n");
}

void read_file_hex (FILE* fp) {
	size_t fread_count, current_mem_offset = 0;

	unsigned char buff[BUFF_BYTES_HEX];
	while ((fread_count = fread(buff, sizeof(char), sizeof(buff), fp)) > 0) {
		print_hex(fread_count, buff, current_mem_offset);
		current_mem_offset += fread_count; // appending the number of bytes we've read so we track the exact value and avoid fread() hiccups when it returns short reads (i was tracking 16 byte chunks earlier lol)
	}
}

void cli_syntax (void) {
	printf(
		"Invalid option -- hexdump [opt: binary | hex] filename.extension\n"
	);
	exit(EXIT_FAILURE);
}