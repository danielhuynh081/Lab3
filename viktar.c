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
void shortTOC(const char * filename, viktar_header_t md, char buf[BUF_SIZE]);
void print_timespec(struct timespec ts);
void createFile(char * filename, char ** files);
//Main
int main(int argc, char *argv[]) {
	//Define Variables
	int iarch = STDIN_FILENO;
	int opt = 0;            
	int fd =0;
	char * filename = NULL;
	int create =0;
	int sTOC =0;
	char buf[BUF_SIZE] = {0};
	viktar_header_t md; 
	//Handle Commands
	while ((opt = getopt(argc, argv, OPTIONS)) != -1) {
		switch (opt) {
			case 'f': // Specify File
				if(optarg){
					filename = optarg;
				}
				break;
			case 'x': // Extract Members
				if(!filename){
					printf("extract all files\n");
				}
				break;
			case 'c': // Create File
				create = 1;
				break;
			case 't': // Short Table Of Contents
				  sTOC =1;
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
						print_mode(md.st_mode);
						printf("\t\tuser: \t\t%s\n", pw->pw_name);
						printf("\t\tgroup: \t\t%s\n", grp->gr_name);
						snprintf(buf, sizeof(buf), "%lld", (long long)md.st_size);
						printf("\t\tsize: \t\t%s\n", buf);
						printf("\t\tatime: ");
						print_timespec(md.st_mtim);
						printf("\t\tmtime: ");
						print_timespec(md.st_atim);
						printf("\n\t\twork in progress:\n");
						printf("\t\tmd5 sum header: %s\n", buf);
						printf("\t\tmd5 sum data: \t%s\n\n", buf);
						// You can also print the last access and modification times
						lseek(iarch, md.st_size + sizeof(viktar_footer_t), SEEK_CUR);
					}
						close(iarch);
				}else{
					perror("Filename null\n");
					exit(EXIT_FAILURE);
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
	//create
	if(create == 1){
		if (!filename) {
			printf("Please specify an archive filename with -f\n");
			exit(EXIT_FAILURE);
		}
		fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
		if (fd == -1) {
			perror("Error creating archive file");
			exit(EXIT_FAILURE);
		}
		createFile(filename, &argv[optind]);
		close(fd);
	}
	if(sTOC ==1){
		shortTOC(filename, md, buf);
	}
	return 1;

}

void shortTOC(const char * filename, viktar_header_t md, char buf[BUF_SIZE]){
	ssize_t bytes_read;
	int iarch = STDIN_FILENO;
	if(filename != NULL){
		iarch = open(filename, O_RDONLY);
		if(iarch == -1){
			perror("Error opening file\n");
			exit(EXIT_FAILURE);
		}
		// Read the expected number of bytes for the tag
		bytes_read = read(iarch, buf, strlen(VIKTAR_TAG));
		if (bytes_read != strlen(VIKTAR_TAG)) {
			fprintf(stderr, "Failed to read tag from file\n");
			close(iarch);
			exit(EXIT_FAILURE);
		}
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
	}else{
	printf("filename null");
	exit(EXIT_FAILURE);
	}
	exit(EXIT_SUCCESS);
}
void createFile(char * filename, char ** files){
	int oarch = STDOUT_FILENO;
	int input_fd;
	viktar_header_t header;
	struct stat hold;
	char buffer[BUF_SIZE];
        ssize_t bytes_read;

	mode_t old_umask = umask(0022);
	if(filename){
		//initial permissions
		//open file descriptor after
		//assign persmission using umask
		oarch = open(filename, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
		if (oarch == -1) {
			printf("open fail");
			exit(EXIT_FAILURE);
		}
		umask(old_umask);
	}	
	else{
		//check verbose flag, if verbose flag is not null do something
		return;
	}
	//write the header infomration into the output file
	write(oarch, VIKTAR_TAG, strlen(VIKTAR_TAG));
	for(int i= 0; files[i] != NULL; ++i){
		//iterate the files to put into the viktar and handle metadata
		printf("\nfile: %d\n", i); 
		memset(&header, 0, sizeof(viktar_header_t));
		strncpy(header.viktar_name, files[i], VIKTAR_MAX_FILE_NAME_LEN);
		//use stat function
		if(stat(files[i], &hold) == -1){
			perror("Meta data failure\n");
			close(oarch);
			exit(EXIT_FAILURE);
		}

		//check if its successsful, exit if fails
		header.st_mode = hold.st_mode;
		header.st_uid = hold.st_uid;
		header.st_gid = hold.st_gid;
		header.st_atim = hold.st_atim;
		header.st_mtim = hold.st_mtim;
		//write the header into the archive
		//handle md5 in the footer
		// Write the header to the archive
		write(oarch, &header, sizeof(viktar_header_t));
		// Open the file for reading content
		input_fd = open(files[i], O_RDONLY);
		if (input_fd == -1) {
			perror("Failed to open input file");
			close(oarch);
			return;
		}
		while ((bytes_read = read(input_fd, buffer, sizeof(buffer))) > 0) {
			if (write(oarch, buffer, bytes_read) != bytes_read) {
				perror("Error writing file content to archive");
				close(input_fd);
				close(oarch);
				exit(EXIT_FAILURE);
			}
		}
		close(input_fd);
	}
	close(oarch);
	printf("Archive file %s created successfully with metadata.\n", filename);
	//for loop iterates the files
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
