#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h> 
#include <stdint.h>

// cross platform VT sequence setup
#ifdef _WIN32
	#include <windows.h> // VT Sequences for TUI
	// global terminal state 
	HANDLE h_out, h_in;
	DWORD dw_original_out_mode = 0;
	DWORD dw_original_in_mode = 0;
#else
	#include <termios.h>
	#include <unistd.h>
	struct termios orig_termios;
#endif

#define BUFF_BYTES 1024
#define BUFF_BYTES_HEX 16 // formats better in the terminal
#define EDIT_BUFF_BYTES 256

char EDIT_INPUT[2] = { '\0', '\0'};

void print_binary (const size_t bytes_to_read, unsigned char *binary_data);
void read_file_binary (FILE* fp);
void print_hex (const size_t bytes_to_read, unsigned char* binary_data, size_t mem_off, int interactive, size_t camera_x, size_t camera_y, size_t local_row, int is_editing);
void read_file_hex (FILE* fp);
void hexedit_file (FILE* fp);
void cli_syntax (void);
int init_terminal (void);
void restore_terminal_mode (void);
int read_input_bytes (char* buffer, size_t count);
void wipe_edit_input (void);
uint8_t hex_to_dec_val (char chr);

int main (int argc, char *argv[]) {
	if (init_terminal() == -1) {
		printf("Error: Could not initialize terminal\n");
		exit(EXIT_FAILURE);
	}

	// disable VT sequencing when exiting the program
	atexit(restore_terminal_mode);

	if (argc < 2 || argc > 3) {
		cli_syntax();
	}
	
	int mode = 3; // default is edit

	if (argc == 3) {
		if (strcmp(argv[1], "binary") == 0) {
			mode = 0;
		}
		else if (strcmp(argv[1], "hex") == 0) {
			mode = 1;
		}
		else if (strcmp(argv[1], "edit") == 0) {
			mode = 3;
		}
		else {
			cli_syntax();
		}
	}

	char* file_mode = (mode == 3) ? "rb+" : "rb";
	FILE* fp = fopen(argv[argc-1], file_mode); // argc-1 means the last CLI arg is the file name
	if (fp == NULL) {
		printf("Failed to open file. Did you spell it correctly? (Extensions must be included)\n");
		exit(EXIT_FAILURE);
	}

	if (mode == 0) {
		read_file_binary(fp);
	}
	else if (mode == 1) {
		read_file_hex(fp);
	}
	else {
		//printf("\x1b[31mEdit Mode it is!\x1b[0m\n");
		hexedit_file(fp);
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

void print_hex (const size_t bytes_to_read, unsigned char* binary_data, size_t mem_off, int interactive, size_t camera_x, size_t camera_y, size_t local_row, int is_editing) {
	char buff_output[(BUFF_BYTES_HEX * 3) + 1 + 19]; // one hex is equal to 4 bits so * 2 for the whole byte + (*1) for the whitespace and +1 for the NULL terminator
	// ^^ + 19 because of the hidden VT string characters
	int buff_index = 0;

	char ascii_buff[BUFF_BYTES_HEX + 1 + 3]; // each char + NULL terminator + 3 whitespaces to format better (3 because there is a \0 from output_buffer)
	ascii_buff[0] = ' ';
	ascii_buff[1] = ' ';
	ascii_buff[2] = ' ';
	int ascii_buff_index = 3; // hard coding spaces like this may not look pretty but its actually very fast and lightweight

	// 32bit max so this tool technically doesnt support hexediting and hexdumping files larger than 4GB as of now, easy fix tho maybe ill add a flag in the future
	printf("%08zX    ", mem_off);

	for (size_t i = 0; i < bytes_to_read; i++) {
		// overwriting the null terminator by adding len to buff_index, also sprintf is secure in this case so no need to use snprintf
		unsigned int len;
		if (interactive && camera_x == i && camera_y == local_row) {
			if (is_editing) {
				len = sprintf(&buff_output[buff_index], "\x1b[7;41m%02X\x1b[0m ", binary_data[i]);
			}
			else {
				len = sprintf(&buff_output[buff_index], "\x1b[7m%02X\x1b[27m ", binary_data[i]);
			}
		}
		else {
			len = sprintf(&buff_output[buff_index], "%02X ", binary_data[i]); // this adds a null terminator
		}
		
		buff_index += len;

		ascii_buff[ascii_buff_index++] = isprint(binary_data[i]) ? binary_data[i] : '.';
	}

	// padding the empty space so the ASCII column doesnt crash and shift into the middle if hex bytes are not aligned perfectly, formatting purposes basically
	for (size_t i = bytes_to_read; i < BUFF_BYTES_HEX; i++) {
		unsigned int len = sprintf(&buff_output[buff_index], "   ");
		buff_index += len;
	}

	// sprintf automatically adds a '\0' so we dont need to add it here..
	fputs(buff_output, stdout);

	ascii_buff[ascii_buff_index] = '\0';
	fputs(ascii_buff, stdout);

	printf("\n");
}

void read_file_hex (FILE* fp) {
	size_t fread_count, current_mem_offset = 0;

	unsigned char buff[BUFF_BYTES_HEX];
	while ((fread_count = fread(buff, sizeof(char), sizeof(buff), fp)) > 0) {
		print_hex(fread_count, buff, current_mem_offset, 0, 0, 0, 0, 0);
		current_mem_offset += fread_count; // appending the number of bytes we've read so we track the exact value and avoid fread() hiccups when it returns short reads (i was tracking 16 byte chunks earlier lol)
	}
}

void hexedit_file (FILE* fp) {
	fseek(fp, 0, SEEK_END);
	size_t file_size = (size_t) ftell(fp); // fixed the long and size_t type mismatch
	fseek(fp, 0, SEEK_SET);

	size_t viewport_offset = 0, camera_x = 0, camera_y = 0;
	unsigned char buff[EDIT_BUFF_BYTES]; // 16 bytes 16 columns
	int is_editing = 0;

	// decided to just hide the cursor && enter alternate buffer mode
	printf("\x1b[?1049h\x1b[?25l");

	while (1) {
		// This means user has commited the edit, time to execute
		if (is_editing && EDIT_INPUT[0] != '\0' && EDIT_INPUT[1] != '\0') {
			// get the raw byte
			uint8_t byte = (hex_to_dec_val(EDIT_INPUT[0]) << 4) | hex_to_dec_val(EDIT_INPUT[1]);

			// move the file pointer to the target byte
			size_t target = viewport_offset + (camera_y * 16) + camera_x;
			fseek(fp, target, SEEK_SET);

			// overwrite the byte and flush to disk
			fputc(byte, fp);
			fflush(fp);

			wipe_edit_input();

			if (target + 1 < file_size) {
				if (camera_x < 15) { // move to the next hex digits
					camera_x += 1;
				}
				else if (camera_y < 15) { // move one line down
					camera_x = 0;
					camera_y += 1;
				}
				else if (viewport_offset + EDIT_BUFF_BYTES < file_size) { // scroll down if hitting bottom right
					camera_x = 0;
					viewport_offset += BUFF_BYTES_HEX; 
				}
			}
		}

		printf("\x1b[2J\x1b[H");
		fseek(fp, viewport_offset, SEEK_SET);
		fread(buff, sizeof(char), sizeof(buff), fp);
		size_t local_row = 0;

		// loop through the array 16 bytes at a time
		for (size_t i = 0; i < sizeof(buff); i += BUFF_BYTES_HEX) {
			size_t current_file_offset = viewport_offset + i;

			// stop if we reach the end of the file
			if (current_file_offset >= file_size) break;

			// check if we have less than 16 bytes left
			size_t chunk_left = BUFF_BYTES_HEX;
			if (current_file_offset + chunk_left > file_size) {
				chunk_left = file_size - current_file_offset;
			}

			print_hex(chunk_left, &buff[i], current_file_offset, 1, camera_x, camera_y, local_row++, is_editing);
		}

		if (is_editing) {
			fputs("Overwrite with: ", stdout);

			if (EDIT_INPUT[0] != '\0') {
				putchar(toupper(EDIT_INPUT[0]));
			}

			if (EDIT_INPUT[1] != '\0') {
				putchar(toupper(EDIT_INPUT[1]));
			}

			fflush(stdout); // need to flush because stdous is line buffered in posix systems
		}

		char user_input[8];
		int bytes_read = read_input_bytes(user_input, 1);
		if (bytes_read == 0) continue;

		if (user_input[0] == '\x1b') {

			bytes_read = read_input_bytes(&user_input[1], 2);

			if (user_input[1] == '[') {

				// keep track so we can use it to stop navigating into empty memory past the EOF
				size_t absolute_camera_pos = viewport_offset + (camera_y * BUFF_BYTES_HEX) + camera_x;

				switch (user_input[2]) {
				case 'A': // up arrowkey
					if (camera_y > 0) {
						camera_y -= 1;
						wipe_edit_input();
					}
					else if (viewport_offset >= 16) {
						viewport_offset -= 16;
						wipe_edit_input();
					}

					break;

				case 'B': // down arrowkey
					if (camera_y < 15 && absolute_camera_pos + 16 < file_size) {
						camera_y += 1;
						wipe_edit_input();
					}
					else if (viewport_offset + 16 < file_size) {
						viewport_offset += 16;
						wipe_edit_input();
					}

					break;

				case 'C': 
					if (absolute_camera_pos + 1 >= file_size) {
						break; 
					}

					if (camera_x < 15) {
						camera_x += 1;
						wipe_edit_input();
					}
					else if (camera_y < 15) {
						camera_x = 0;
						camera_y += 1;
						wipe_edit_input();
					}

					break;

				case 'D':
					if (camera_x > 0) {
						camera_x -= 1;
						wipe_edit_input();
					}
					else if (camera_y > 0) {
						camera_x = 15;
						camera_y -= 1;
						wipe_edit_input();
					}

					break;

				case '5': // page up
					read_input_bytes(&user_input[3], 1); // eat the trailing ~
					if (viewport_offset >= EDIT_BUFF_BYTES) {
						viewport_offset -= EDIT_BUFF_BYTES;
						wipe_edit_input();
					}
					else {
						viewport_offset = 0;
						wipe_edit_input();
					}

					break;

				case '6': // page down
					read_input_bytes(&user_input[3], 1); // same reason 
					if (viewport_offset + EDIT_BUFF_BYTES < file_size) {
						viewport_offset += EDIT_BUFF_BYTES;
						wipe_edit_input();
					}

					break;
				}
			}
		}
		else if (is_editing && isxdigit(user_input[0])) {
			if (EDIT_INPUT[0] == '\0') {
				EDIT_INPUT[0] = user_input[0];
			}
			else if (EDIT_INPUT[1] == '\0') {
				EDIT_INPUT[1] = user_input[0];
			}
		}
		else if (user_input[0] == 'q' || user_input[0] == 'Q') {
			// printf("\x1b[2J\x1b[H"); // clr screen rq // clear screen no longer needed cuz we're leaving the alternate buffer when exiting
			exit(EXIT_SUCCESS);
		}
		else if (user_input[0] == '\r' || user_input[0] == '\n') { // \n for Unix enter key handling
			is_editing = !(is_editing); // toggle edit mode
			wipe_edit_input();
		}
	}
}

void cli_syntax (void) {
	printf(
		"Invalid option -- hexdump [opt: binary | hex | edit] filename.extension\n"
	);
	exit(EXIT_FAILURE);
}

int init_terminal (void) {
#ifdef _WIN32
	h_out = GetStdHandle(STD_OUTPUT_HANDLE);
	if (h_out == INVALID_HANDLE_VALUE) return -1;

	h_in = GetStdHandle(STD_INPUT_HANDLE);
	if (h_in == INVALID_HANDLE_VALUE) return -1;
	if (!GetConsoleMode(h_out, &dw_original_out_mode)) return -1;

	DWORD dw_requested_out_modes = ENABLE_VIRTUAL_TERMINAL_PROCESSING | DISABLE_NEWLINE_AUTO_RETURN;
	DWORD dw_requested_in_modes = ENABLE_VIRTUAL_TERMINAL_INPUT;

	DWORD dw_out_mode = dw_original_out_mode | dw_requested_out_modes;
	if (!SetConsoleMode(h_out, dw_out_mode)) {
		// this means we failed to set both modes so we try to step down mode gracefully
		dw_requested_out_modes = ENABLE_VIRTUAL_TERMINAL_PROCESSING;
		dw_out_mode = dw_original_out_mode | dw_requested_out_modes;

		// atp we failed to set any VT mode so cant do anything here
		if (!SetConsoleMode(h_out, dw_out_mode)) return -1;	
	}

	DWORD dw_in_mode = dw_original_in_mode | dw_requested_in_modes;

	// same story here, cant do anything more
	if (!SetConsoleMode(h_in, dw_in_mode)) return -1;

	return 0;
#else
	if (tcgetattr(STDIN_FILENO, &orig_termios) == -1) return -1;

	struct termios raw = orig_termios;
	
	// turn off echoing and canonical mode
	raw.c_lflag &= ~(ECHO | ICANON);
	
	// read returns as soon as 1 byte is pressed 
	raw.c_cc[VMIN] = 1;
	raw.c_cc[VTIME] = 0;

	if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) return -1;
	return 0;
#endif
}

void restore_terminal_mode (void) {
#ifdef _WIN32
	SetConsoleMode(h_out, dw_original_out_mode);
	SetConsoleMode(h_in, dw_original_in_mode);
	
#else
	tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
#endif
	printf("\x1b[?1049l\x1b[?25h"); // unhide the cursor && leave the alternate buffer terminal mode
	fflush(stdout); // needed on posix to make sure string prints before exiting
}

int read_input_bytes (char *buffer, size_t count) {
#ifdef _WIN32
	DWORD bytes_read;
	int success = ReadFile(h_in, buffer, (DWORD) count, &bytes_read, NULL);
	if (success == 0 || bytes_read == 0) return 0;
	return (int) bytes_read;
#else
	ssize_t bytes_read = read(STDIN_FILENO, buffer, count);
	if (bytes_read <= 0) return 0;
	return (int)bytes_read;
#endif
}

void wipe_edit_input (void) {
	EDIT_INPUT[0] = '\0';
	EDIT_INPUT[1] = '\0';
}

uint8_t hex_to_dec_val (char chr) {
	if (chr >= '0' && chr <= '9') return chr - '0';
	if (chr >= 'A' && chr <= 'F') return chr - 'A' + 10;
	if (chr >= 'a' && chr <= 'f') return chr - 'a' + 10;

	return 0;
}