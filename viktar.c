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
#include <pwd.h>  // For getpwuid
#include <grp.h>  // For getgrgid
#include <fcntl.h>  // Required for open()
#include <sys/stat.h>
#include <time.h>

//Define Macros
#define BUF_SIZE 1024
#define MIN_SHIFT 0
#define MAX_SHIFT 95
#define BASE 32

void print_mode(mode_t mode);
void shortTOC(const char * filename);
void print_timespec(struct timespec ts);
//Main
int main(int argc, char *argv[]) {
	//Define Variables
	int opt = 0;            
	char * filename = NULL;
	int iarch = STDIN_FILENO;
	char buf[BUF_SIZE] = {0};
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
				if (!filename) {
					printf("Please specify an archive filename with -f\n");
					exit(EXIT_FAILURE);
				}
				break;
			case 't': // Short Table Of Contents
				if(filename != NULL){
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
						printf("\nfilename: %s\n", buf);
						lseek(iarch, md.st_size + sizeof(viktar_footer_t), SEEK_CUR);
					}
					if(filename != NULL){
						close(iarch);
					}
				}
				printf("filename null");

				break;
			case 'T': // Long Table Of Contents
				if(filename != NULL){
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
						struct passwd *pw = getpwuid(md.st_uid);
						struct group * grp = getgrgid(md.st_gid);

						//print
						memset(buf, 0, 100);

						strncpy(buf, md.viktar_name, VIKTAR_MAX_FILE_NAME_LEN);
						printf("\tfile name: %s\n", buf);

						//snprintf(buf, sizeof(buf), "%o", md.st_mode);
						//printf("\t\tmode: \t\t%s\n", buf);
						print_mode(md.st_mode);
						printf("\t\tuser: \t\t%s\n", pw->pw_name);
						printf("\t\tgroup: \t\t%s\n", grp->gr_name);
						snprintf(buf, sizeof(buf), "%lld", (long long)md.st_size);
						printf("\t\tsize: \t\t%s\n", buf);

						printf("\t\tatime: ");
						print_timespec(md.st_mtim);
						printf("\t\tmtime: ");
						print_timespec(md.st_atim);


						/*
						   snprintf(buf, sizeof(buf), "%d", md.st_gid);
						   printf("\t\tgroup ID: \t%s\n", buf);

						   snprintf(buf, sizeof(buf), "%d", md.st_uid);
						   printf("\t\tuser ID: \t%s\n", buf);
						   */



						printf("\n\t\twork in progress:\n");
						printf("\t\tmd5 sum header: %s\n", buf);
						printf("\t\tmd5 sum data: \t%s\n\n", buf);




						// You can also print the last access and modification times
						lseek(iarch, md.st_size + sizeof(viktar_footer_t), SEEK_CUR);
					}
					if(filename != NULL){
						close(iarch);
					}
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
void print_mode(mode_t mode) {
    char buf[11];
    buf[0] = (mode & S_IFDIR) ? 'd' : '-';

    buf[1] = (mode & S_IRUSR) ? 'r' : '-';
    buf[2] = (mode & S_IWUSR) ? 'w' : '-';
    buf[3] = (mode & S_IXUSR) ? 'x' : '-';

    buf[4] = (mode & S_IRGRP) ? 'r' : '-';
    buf[5] = (mode & S_IWGRP) ? 'w' : '-';
    buf[6] = (mode & S_IXGRP) ? 'x' : '-';

    buf[7] = (mode & S_IROTH) ? 'r' : '-';
    buf[8] = (mode & S_IWOTH) ? 'w' : '-';
    buf[9] = (mode & S_IXOTH) ? 'x' : '-';

    buf[10] = '\0';

    printf("\t\tmode: \t\t%s\n", buf);
}

void print_timespec(struct timespec ts) {
    // Convert seconds to struct tm
    struct tm *tm_info = localtime(&ts.tv_sec);
   char * timezone = tzname[tm_info->tm_isdst];

    // Buffer for formatted time
    char buffer[100];

    // Format the time
    strftime(buffer, sizeof(buffer), "\t\t%Y-%m-%d %H:%M:%S", tm_info);

    // Print the formatted time
    printf("%s %s\n", buffer, timezone);
}
