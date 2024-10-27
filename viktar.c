//Daniel Huynh, October 24th, 2024
//CS333 Jesse Chaney Lab 3. This program reads and writes archive files using getopt and read() write() functions
//


#include <openssl/md5.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "viktar.h"
#include <fcntl.h>  // Required for open()

//Define Macros
#define BUF_SIZE 1024
#define MIN_SHIFT 0
#define MAX_SHIFT 95
#define BASE 32

void shortTOC(const char * filename);
//Main
int main(int argc, char *argv[]) {
	//Define Variables
	int opt = 0;            
	char * filename = NULL;
	int iarch = STDIN_FILENO;
	char buf[100] = {0};
	viktar_header_t md; 

	//Handle Commands
	while ((opt = getopt(argc, argv, OPTIONS)) != -1) {
		switch (opt) {
			case 'f': // Specify File
				if(optarg){
					filename = optarg;
				}else{
					printf("read/write stdio\n");
				}
				break;
			case 'x': // Extract Members
				if(!filename){
					printf("extract all files\n");
				}
				break;
			case 'c': // Create File
				if(!filename){
					printf("write to stdout\n");
				}
				break;
			case 't': // Short Table Of Contents
				if(filename != NULL){
					printf("inside t case\n");
					iarch = open(filename, O_RDONLY);
					if(iarch == -1){
						perror("Error opening file\n");
						exit(EXIT_FAILURE);
					}
					read(iarch, buf, strlen(VIKTAR_TAG));
					if(strncmp(buf, VIKTAR_TAG, strlen(VIKTAR_TAG)) != 0){
						perror("Not a valid viktar file\n");
						exit(EXIT_FAILURE);
					}

					printf("Contents of the viktar file : \"%s\"\n", filename != NULL ? filename : "stdin");
					while (read(iarch, &md, sizeof(viktar_header_t)) > 0){
						//print
						memset(buf, 0, 100);
						strncpy(buf, md.viktar_name, VIKTAR_MAX_FILE_NAME_LEN);
						printf("\ntfilename: %s\n", buf);
						lseek(iarch, md.st_size + sizeof(viktar_footer_t), SEEK_CUR);
					}
					if(filename != NULL){
						close(iarch);
					}
				}
				printf("filename null");

				break;
			case 'T': // Long Table Of Contents
				if(!filename){
					printf("read from stdin\n");
				}
				break;
			case 'V': // Validate Content
				if(!filename){
					printf("read from stdin\n");
				}
				break;
			case 'v': // Verbose Processing
				printf("verbose processing\n");
				break;
			case 'h': // Help Option
				printf("\nUsage: ./viktar (-e | -d) (-s shift) (-h) (< textfile.txt | Keyboard input)\n");
				printf(" -x: Extract Members\n");
				printf(" -c: Create File\n");
				printf(" -t: Short Table Of Contents\n");
				printf(" -T: Short Table Of Contents\n");
				printf(" -f: Specify File\n");
				printf(" -V: Validate Content\n");
				printf(" -v: Verbose Processing\n");
				printf(" -h: Help Message\n");
				exit(EXIT_SUCCESS);
				break;
			default:
				printf("Error processing an option\n");
				exit(EXIT_FAILURE);
				break;
		}

	}
	// Program Rest Of Code // Table of contents tomorrow
	return 1;

}
