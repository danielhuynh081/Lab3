//Daniel Huynh, October 24th, 2024
//CS333 Jesse Chaney Lab 3. This program reads and writes archive files using getopt and read() write() functions
//

#include <md5.h>

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

void validateFile(const char * filename);
void extractFiles(const char* filename);
void print_mode(mode_t mode);
void shortTOC(const char * filename, viktar_header_t md, char buf[BUF_SIZE]);
void longTOC( char * filename, viktar_header_t md, char buf[BUF_SIZE], size_t buf_size);
void print_timespec(struct timespec ts);
void createFile(char * filename, char ** files);
//Main
int main(int argc, char *argv[]) {
	//Define Variables
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
				extractFiles(filename);
				break;
			case 'c': // Create File
				create = 1;
				break;
			case 't': // Short Table Of Contents
				sTOC =1;
				break;
			case 'T': // Long Table Of Contents
				longTOC(filename, md, buf, sizeof(buf));
				break;
			case 'V': // Validate Content
			validateFile(filename);
				break;
			case 'v': // Verbose Processing
				break;
			case 'h': // Help Option
				printf("help text\n");
				printf("\t./viktar\n");
				printf("\tOptions: xcTf:Vhv\n");
				printf("\t\t-x\t\textract file/files from archive\n");
				printf("\t\t-c\t\tcreate an archive file\n");
				printf("\t\t-t\t\tdisplay a short table of contents of the archive file\n");
				printf("\t\t-T\t\tdisplay a long table of contents of the archive file\n");
				printf("\t\tOnly one of the xctTV can be specified");
				printf("\n\t\t-f filename\tuse filename as the archive file\n");
				printf("\t\t-V\t\tvalidate the MD5 vaues in the viktar file\n");
				printf("\t\t-v\t\tgive verbose diagnostic messages\n");
				printf("\t\t-h\t\tdisplay this AMAZING help message\n");
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

void validateFile(const char * filename){
	//get md5 value we stored


}

void extractFiles(const char * filename){

	int iarch = open(filename, O_RDONLY);
	char *data_buf = NULL;
	ssize_t bytes_read = {'\0'};
	FILE *outfile;
		viktar_footer_t footer;
	viktar_header_t md;
	if(!filename){
		printf("stdin\n");
		exit(EXIT_FAILURE);
	}
	if (iarch == -1) {
		perror("Error opening viktar file");
		exit(EXIT_FAILURE);
	}
	while(read(iarch, &md, sizeof(viktar_header_t)) > 0){
		data_buf = malloc(md.st_size);
		if (!data_buf) {
			perror("Error allocating memory for file data");
			exit(EXIT_FAILURE);
		}

		// Read the file data into the buffer
		bytes_read = read(iarch, data_buf, md.st_size);
		if (bytes_read != md.st_size) {
			perror("Error reading file data");
			free(data_buf);
			exit(EXIT_FAILURE);
		}

		// Create a new file to write the extracted data
		outfile = fopen(md.viktar_name, "wb");
		if (!outfile) {
			perror("Error creating output file");
			free(data_buf);
			exit(EXIT_FAILURE);
		}

		// Write the data to the new file
		fwrite(data_buf, 1, md.st_size, outfile);

		// Clean up
		fclose(outfile);
		free(data_buf);

		// Optionally, you might want to seek past the footer here if it exists
		// Assuming footer is defined as following:
		read(iarch, &footer, sizeof(viktar_footer_t));
	}

	close(iarch);



}
void longTOC( char * filename, viktar_header_t md, char buf[BUF_SIZE], size_t buf_size){
	int iarch = STDIN_FILENO;
	viktar_footer_t footer;  // Added footer variable

	if (filename != NULL) {
		iarch = open(filename, O_RDONLY);
		if (iarch == -1) {
			perror("Error opening file\n");
			exit(EXIT_FAILURE);
		}

		read(iarch, buf, strlen(VIKTAR_TAG));
		if (strncmp(buf, VIKTAR_TAG, strlen(VIKTAR_TAG)) != 0) {
			perror("Not a valid viktar file\n");
			exit(EXIT_FAILURE);
		}

		printf("Contents of the viktar file: \"%s\"\n", filename != NULL ? filename : "stdin");

		while (read(iarch, &md, sizeof(viktar_header_t)) > 0) {
			struct passwd *pw = getpwuid(md.st_uid);
			struct group *grp = getgrgid(md.st_gid);

			memset(buf, 0, 100);
			strncpy(buf, md.viktar_name, VIKTAR_MAX_FILE_NAME_LEN);
			printf("\tfile name: %s\n", buf);
			print_mode(md.st_mode);
			printf("\t\tuser: \t\t%s\n", pw->pw_name);
			printf("\t\tgroup: \t\t%s\n", grp->gr_name);
			snprintf(buf, buf_size, "%lld", (long long)md.st_size);
			printf("\t\tsize: \t\t%s\n", buf);
			printf("\t\tatime: ");
			print_timespec(md.st_mtim);
			printf("\t\tmtime: ");
			print_timespec(md.st_atim);

			// Move file pointer past file content
			if (lseek(iarch, md.st_size, SEEK_CUR) == -1) {
				perror("Error seeking past file content\n");
				exit(EXIT_FAILURE);
			}

			// Read and print footer MD5 checksums
			if (read(iarch, &footer, sizeof(viktar_footer_t)) != sizeof(viktar_footer_t)) {
				perror("Error reading footer\n");
				exit(EXIT_FAILURE);
			}

			// Debug: Print raw footer data in hex
			printf("Debug: Raw footer data:\n");
			for (size_t i = 0; i < sizeof(viktar_footer_t); i++) {
				printf("%02x ", ((unsigned char*)&footer)[i]);
			}
			printf("\n");

			printf("\t\tmd5 sum header: ");
			for (int i = 0; i < MD5_DIGEST_LENGTH; i++) printf("%02x", footer.md5sum_header[i]);
			printf("\n\t\tmd5 sum data:   ");
			for (int i = 0; i < MD5_DIGEST_LENGTH; i++) printf("%02x", footer.md5sum_data[i]);
			printf("\n\n");
		}
		close(iarch);
	} else {
		perror("Filename null\n");
		exit(EXIT_FAILURE);
	}
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


		printf(" Contents of the viktar file: %s\n", filename != NULL ? filename : "stdin");
		while (read(iarch, &md, sizeof(viktar_header_t)) > 0){
			//print
			memset(buf, 0, 100);
			strncpy(buf, md.viktar_name, VIKTAR_MAX_FILE_NAME_LEN);
			printf("\n\tfile name: %s\n", buf);
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
	viktar_footer_t footer;
	struct stat hold;
	char buffer[BUF_SIZE];
	ssize_t bytes_read;
	mode_t old_umask = umask(0);
	MD5_CTX context_header;
	MD5_CTX context_data;
	MD5Init(&context_header);
	MD5Init(&context_data);

	if(filename){
		//initial permissions
		//open file descriptor after
		//assign persmission using umask
		oarch = open(filename, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
		if (oarch == -1) {
			printf("open fail");
			exit(EXIT_FAILURE);
		}
	}	
	else{
		//check verbose flag, if verbose flag is not null do something
		return;
	}
	//write the viktar tag into the output file
	write(oarch, VIKTAR_TAG, strlen(VIKTAR_TAG));
	for(int i= 0; files[i] != NULL; ++i){
		//iterate the files to put into the viktar and handle metadata
		//		buffer[BUF_SIZE] = {0};
		memset(&header, 0, sizeof(viktar_header_t));
		strncpy(header.viktar_name, files[i], VIKTAR_MAX_FILE_NAME_LEN);
		printf("file name: %s", files[i]);

		//use stat function
		if(stat(files[i], &hold) == -1){
			perror("Meta data failure\n");
			close(oarch);
			exit(EXIT_FAILURE);
		}

		//check if its successsful, exit if fails
		header.st_mode = hold.st_mode;
		header.st_size = hold.st_size;
		header.st_uid = hold.st_uid;
		header.st_gid = hold.st_gid;
		header.st_atim = hold.st_atim;
		header.st_mtim = hold.st_mtim;
		//write the header into the archive

		MD5Update(&context_header, (unsigned char*)&header, sizeof(viktar_header_t));
		MD5Final(footer.md5sum_header, &context_header);

		write(oarch, &header, sizeof(viktar_header_t));

		// Open the file for reading content
		input_fd = open(files[i], O_RDONLY);
		if (input_fd == -1) {
			perror("Failed to open input file");
			close(oarch);
			return;
		}
		//md5


		while ((bytes_read = read(input_fd, buffer, sizeof(buffer))) > 0) {
			//update md5
			MD5Update(&context_data, (const uint8_t *)buffer, bytes_read);
			if (write(oarch, buffer, bytes_read) != bytes_read) {
				perror("Error writing file content to archive");
				close(input_fd);
				close(oarch);
				exit(EXIT_FAILURE);
			}
		}
		//Finalize md5
		MD5Final(footer.md5sum_data, &context_data);

		close(input_fd);
		write(oarch, &footer, sizeof(viktar_footer_t));
	}
	close(oarch);
	umask(old_umask);
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
