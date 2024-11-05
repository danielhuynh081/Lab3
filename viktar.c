//Daniel Huynh, October 24th, 2024
//CS333 Jesse Chaney Lab 3. This program reads and writes archive files using getopt and read() write() functions
//

#include <md5.h>

#include <stdbool.h>
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


bool findFile(char ** files, char * file);
void validateFile(const char * filename);
void extractFiles(const char* filename, char ** files);
void print_mode(mode_t mode);
void shortTOC(char * filename, viktar_header_t md, char buf[BUF_SIZE], size_t buf_size);
void longTOC( char * filename, viktar_header_t md, char buf[BUF_SIZE], size_t buf_size);
void print_timespec(struct timespec *ts);
void createFile(char * filename, char ** files);
//Main
int main(int argc, char *argv[]) {
	//Define Variables
	bool Verbose = false;
	int opt = 0;            
	char * filename = NULL;
	int create =0;
	int sTOC =0;
	int lTOC =0;
	int validFunc =0;
	int extFunct =0;
	char buf[BUF_SIZE] = {0};
	viktar_header_t md; 
	//Handle Commands
	while ((opt = getopt(argc, argv, OPTIONS)) != -1) {
		switch (opt) {
			case 'f': // Specify File
				if(optarg){
					filename = optarg;
				}else{
					printf("Enter the filename:");
					if (fgets(buf, VIKTAR_MAX_FILE_NAME_LEN , stdin) == NULL) {
						perror("Error reading filename");
						exit(EXIT_FAILURE);
					}
				}

				break;
			case 'x': // Extract Members
				extFunct=1;
				break;
			case 'c': // Create File
				create = 1;
				break;
			case 't': // Short Table Of Contents
				sTOC =1;
				break;
			case 'T': // Long Table Of Contents
				lTOC =1;
				break;
			case 'V': // Validate Content
				validFunc=1;
				break;
			case 'v': // Verbose Processing
				Verbose = !Verbose;
				break;
			case 'h': // Help Option
				fprintf(stderr, "help text\n\t%s\n\tOptions: %s\n", argv[0], OPTIONS);
		//		printf("help text");
		//		printf("\n\t./viktar");
		//		printf("\n\tOptions: xcTf:Vhv");
				fprintf(stderr,"\t\t-x\t\textract file/files from archive");
				fprintf(stderr,"\n\t\t-c\t\tcreate an archive file");
				fprintf(stderr,"\n\t\t-t\t\tdisplay a short table of contents of the archive file");
				fprintf(stderr,"\n\t\t-T\t\tdisplay a long table of contents of the archive file\n");
				fprintf(stderr,"\t\tOnly one of xctTV can be specified\n");
				fprintf(stderr,"\t\t-f filename\tuse filename as the archive file");
				fprintf(stderr,"\n\t\t-V\t\tvalidate the MD5 vaues in the viktar file");
				fprintf(stderr,"\n\t\t-v\t\tgive verbose diagnostic messages");
				fprintf(stderr,"\n\t\t-h\t\tdisplay this AMAZING help message\n");
				exit(EXIT_SUCCESS);
				break;
			default:
				printf("Error processing an option\n");
					printf("Enter the filename:");
					if (fgets(buf, VIKTAR_MAX_FILE_NAME_LEN , stdin) == NULL) {
						perror("Error reading filename");
						exit(EXIT_FAILURE);
					}

				break;
		}

	}
	//create
	if(create){
		createFile(filename, &argv[optind]);
	}
	if(sTOC){
		shortTOC(filename, md, buf, sizeof(buf));
	}
	if(validFunc){
		validateFile(filename);
	}
	if(lTOC){
		longTOC(filename, md, buf, sizeof(buf));
	}
	if(extFunct){
		extractFiles(filename, &argv[optind]);
	}



	return 1;

}

void validateFile(const char *filename) {
        uint8_t headercheck[MD5_DIGEST_LENGTH];
        uint8_t datacheck[MD5_DIGEST_LENGTH];
        ssize_t bytes_read2 =0;
        int iarch = STDIN_FILENO;
        //int iarch2 = STDIN_FILENO;
        int member =1;
        viktar_footer_t footer;  // Added footer variable
        viktar_header_t md;
        char buf[BUF_SIZE];
        unsigned char* buffer = NULL;

        MD5_CTX context_header;
        MD5_CTX context_data;
	if(!filename){
                if (fgets(buf, sizeof(buf), stdin) == NULL) {
                        perror("Error reading filename");
                        exit(EXIT_FAILURE);
                }
                buf[strcspn(buf, "\n")] = '\0';
                filename = buf;

        }

        if (filename != NULL) {
                iarch = open(filename, O_RDONLY);
                //iarch2 = open(filename, O_RDONLY);

                if (iarch == -1) {
                        perror("Error opening file\n");
                        exit(EXIT_FAILURE);
                }

                read(iarch, buf, strlen(VIKTAR_TAG));
                if (strncmp(buf, VIKTAR_TAG, strlen(VIKTAR_TAG)) != 0) {
                        perror("Not a valid viktar file\n");
			exit(EXIT_FAILURE);
		}
	}
	while (read(iarch, &md, sizeof(viktar_header_t)) > 0) {
		memset(buf, 0, 100);
		strncpy(buf, md.viktar_name, VIKTAR_MAX_FILE_NAME_LEN);
		//calculate md5
		//header
		MD5Init(&context_header);
		MD5Init(&context_data);
		MD5Update(&context_header, (unsigned char *)&md, sizeof(viktar_header_t));
		//data
		buffer = (unsigned char * ) malloc(md.st_size + sizeof(unsigned char));
		memset(buffer, 0, md.st_size);
		bytes_read2 = read(iarch, buffer, md.st_size);

		MD5Update(&context_data, buffer, bytes_read2);
		MD5Final(datacheck, &context_data);
		MD5Final(headercheck, &context_header);
		printf("Validation for data member %d:\n", member);

		// Debug: Print raw footer data in hex
		if (read(iarch, &footer, sizeof(viktar_footer_t)) != sizeof(viktar_footer_t)) {
			perror("Error reading footer\n");
			exit(EXIT_FAILURE);
		}
		if (memcmp(headercheck, footer.md5sum_header, MD5_DIGEST_LENGTH) == 0) {
			printf("\tHeader MD5 does match:\n");
			printf("\t\tfound:   ");
			for (int i = 0; i < MD5_DIGEST_LENGTH; i++) printf("%02x", headercheck[i]);
			printf("\n\t\tin file: ");
			for(int i =0; i < MD5_DIGEST_LENGTH; i++)printf("%02x", footer.md5sum_header[i]);
		}
		else {
			printf("\t*** Header MD5 does not match:\n");
			printf("\t\tfound:   ");
			for (int i = 0; i < MD5_DIGEST_LENGTH; i++) printf("%02x", headercheck[i]);
			printf("\n\t\tin file: ");
			for(int i =0; i < MD5_DIGEST_LENGTH; i++)printf("%02x", footer.md5sum_header[i]);
		}
		if (memcmp(footer.md5sum_data, datacheck, MD5_DIGEST_LENGTH) == 0) {
			printf("\n\tData MD5 does match:\n");
			printf("\t\tfound:   ");
			for (int i = 0; i < MD5_DIGEST_LENGTH; i++) printf("%02x", datacheck[i]);
			printf("\n\t\tin file: ");
			for (int i = 0; i < MD5_DIGEST_LENGTH; i++) printf("%02x", footer.md5sum_data[i]);
		} else {
			printf("\n\t*** Data MD5 does not match:\n");
			printf("\t\tfound:   ");
			for (int i = 0; i < MD5_DIGEST_LENGTH; i++) printf("%02x", datacheck[i]);
			printf("\n\t\tin file: ");
			for (int i = 0; i < MD5_DIGEST_LENGTH; i++) printf("%02x", footer.md5sum_data[i]);
		}
		if((memcmp(footer.md5sum_data, datacheck, MD5_DIGEST_LENGTH) == 0) && (memcmp(headercheck, footer.md5sum_header, MD5_DIGEST_LENGTH) == 0)){
			printf("\n\t\tValidation success:\t\t%s for member %d", filename ? filename : "stdin", member);
		}else{
			printf("\n\t*** Validation failure:\t\t%s for member %d", filename ? filename : "stdin",  member);
		}
		printf("\n");
		++member;
	}


	close(iarch);
	//close(iarch2);
	exit(EXIT_SUCCESS);


}
bool findFile(char ** files, char * file){
	for(int i =0 ; files[i] != NULL; ++i){
		if(strncmp(files[i], file, VIKTAR_MAX_FILE_NAME_LEN) == 0){
			return true;
		}
	}	
	return false;
}
void extractFiles(const char *filename, char **files) {
    int iarch = STDIN_FILENO;
    int fd = 0;
    struct timespec times[2];
    char *data_buf = NULL;
    ssize_t bytes_read = 0;
    char buf[BUF_SIZE];
    viktar_footer_t footer;
    viktar_header_t md;
    MD5_CTX context_header;
    MD5_CTX context_data;

    MD5Init(&context_header);
    MD5Init(&context_data);

    // If filename is NULL, ask for input
    if (!filename) {
        printf("Enter the filename: ");
        if (fgets(buf, sizeof(buf), stdin) == NULL) {
            perror("Error reading filename");
            exit(EXIT_FAILURE);
        }
        buf[strcspn(buf, "\n")] = '\0';  // Remove the newline character
        filename = buf;
    }

    // Open the viktar file
    iarch = open(filename, O_RDONLY);
    if (iarch == -1) {
        perror("Error opening viktar file");
        exit(EXIT_FAILURE);
    }

    // Read the tag and validate it's a viktar archive
    read(iarch, buf, strlen(VIKTAR_TAG));
    if (strncmp(buf, VIKTAR_TAG, strlen(VIKTAR_TAG)) != 0) {
        perror("Not a valid viktar file\n");
        exit(EXIT_FAILURE);
    }

    // Process each file in the archive
    while (read(iarch, &md, sizeof(viktar_header_t)) > 0) {
        data_buf = malloc(md.st_size);
        if (!data_buf) {
            perror("Error allocating memory for file data");
            exit(EXIT_FAILURE);
        }

        // Check if this file should be extracted
        if ((*files == NULL) || (findFile(files, md.viktar_name))) {
            // Open the file to extract
            fd = open(md.viktar_name, O_WRONLY | O_CREAT | O_TRUNC, md.st_mode);
            if (fd == -1) {
                perror("Error opening file for writing");
                free(data_buf);
                exit(EXIT_FAILURE);
            }

            // Read the file data into the buffer
            bytes_read = read(iarch, data_buf, md.st_size);
            if (bytes_read != md.st_size) {
                perror("Error reading file data");
                free(data_buf);
                close(fd);
                exit(EXIT_FAILURE);
            }

            // Read the footer (MD5 checksums)
            read(iarch, &footer, sizeof(viktar_footer_t));

            // Write the data to the extracted file
            write(fd, data_buf, md.st_size);

            // Set file permissions
            fchmod(fd, md.st_mode);

            // Set the access and modification times using futimens
            times[0] = md.st_atim;  // access time
            times[1] = md.st_mtim;  // modification time

            if (futimens(fd, times) == -1) {
                perror("Error setting file timestamps");
            }

            // Close the file after writing
            close(fd);
        } else {
            // Skip this file (seek to the next one)
            if (lseek(iarch, md.st_size + sizeof(viktar_footer_t), SEEK_CUR) == -1) {
                perror("Error seeking past file content\n");
                exit(EXIT_FAILURE);
            }
        }

        // Free the buffer before processing the next file
        free(data_buf);
    }

    // Close the archive file
    close(iarch);
}
void longTOC( char * filename, viktar_header_t md, char buf[BUF_SIZE], size_t buf_size){
	int iarch = STDIN_FILENO;
	viktar_footer_t footer;  // Added footer variable
	char filename2[VIKTAR_MAX_FILE_NAME_LEN];
	if (filename == NULL) {
		printf("Enter the filename:");
		if (fgets(buf, buf_size, stdin) == NULL) {
			perror("Error reading filename");
			exit(EXIT_FAILURE);
		}
		buf[strcspn(buf, "\n")] = '\0';
		filename = buf;
	}
	iarch = open(filename, O_RDONLY);
	strncpy(filename2, filename, sizeof(filename2) - 1);
	if(iarch == -1){
		perror("Error opening file\n");
		exit(EXIT_FAILURE);
	}



	read(iarch, buf, strlen(VIKTAR_TAG));
	if (strncmp(buf, VIKTAR_TAG, strlen(VIKTAR_TAG)) != 0) {
		perror("Not a valid viktar file\n");
		exit(EXIT_FAILURE);
	}

	printf("Contents of viktar file: \"%s\"\n", filename ? filename : "stdin");

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
		printf("\t\tsize: \t\t%s", buf);
		printf("\n\t\tmtime: ");
		print_timespec(&md.st_mtim);
		printf("\t\tatime: ");
		print_timespec(&md.st_atim);

		// Move file pointer past file content
		if (lseek(iarch, md.st_size, SEEK_CUR) == -1) {
			perror("Error seeking past file content\n");
			exit(EXIT_FAILURE);
		}

		// Debug: Print raw footer data in hex
		if (read(iarch, &footer, sizeof(viktar_footer_t)) != sizeof(viktar_footer_t)) {
			perror("Error reading footer\n");
			exit(EXIT_FAILURE);
		}

		printf("\t\tmd5 sum header: ");
		for(int i =0; i < MD5_DIGEST_LENGTH; ++i)printf("%02x", footer.md5sum_header[i]);
		printf("\n\t\tmd5 sum data:   ");
		for (int i = 0; i < MD5_DIGEST_LENGTH; ++i) printf("%02x", footer.md5sum_data[i]);
		printf("\n");
	}
	close(iarch);
	exit(EXIT_SUCCESS);
}

void shortTOC(char * filename, viktar_header_t md, char buf[BUF_SIZE], size_t buf_size){
	ssize_t bytes_read;
	int iarch = STDIN_FILENO;
	char filename2[VIKTAR_MAX_FILE_NAME_LEN];
	if (filename == NULL) {
		printf("Enter the filename:");
		if (fgets(buf, buf_size, stdin) == NULL) {
			perror("Error reading filename");
			exit(EXIT_FAILURE);
		}
		buf[strcspn(buf, "\n")] = '\0';
		filename = buf;

		iarch = open(filename, O_RDONLY);
	}
	else {
		iarch = open(filename, O_RDONLY);
	}
	if(iarch == -1){
		perror("Error opening file\n");
		exit(EXIT_FAILURE);
	}
	strncpy(filename2, filename, sizeof(filename2) - 1);
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
	printf("Contents of viktar file: \"%s\"\n", filename ? filename : "stdin");
	while (read(iarch, &md, sizeof(viktar_header_t)) > 0){
		//print
		memset(buf, 0, 100);
		strncpy(buf, md.viktar_name, VIKTAR_MAX_FILE_NAME_LEN);
		printf("\tfile name: %s\n", buf);
		lseek(iarch, md.st_size + sizeof(viktar_footer_t), SEEK_CUR);
	}
	if(filename != NULL){
		close(iarch);
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
		MD5Init(&context_header);
		MD5Init(&context_data);

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
	exit(EXIT_SUCCESS);
}

void print_mode(mode_t mode) {
	// Print file type
	printf("\t\tmode:\t\t%c", (mode & S_IFDIR) ? 'd' : '-');

	// User permissions (owner)
	printf("%c", (mode & S_IRUSR) ? 'r' : '-');
	printf("%c", (mode & S_IWUSR) ? 'w' : '-');
	printf("%c", (mode & S_IXUSR) ? 'x' : '-');

	// Group permissions
	printf("%c", (mode & S_IRGRP) ? 'r' : '-');
	printf("%c", (mode & S_IWGRP) ? 'w' : '-');
	printf("%c", (mode & S_IXGRP) ? 'x' : '-');

	// Other permissions
	printf("%c", (mode & S_IROTH) ? 'r' : '-');
	printf("%c", (mode & S_IWOTH) ? 'w' : '-');
	printf("%c", (mode & S_IXOTH) ? 'x' : '-');

	// Newline after the output
	printf("\n");
}

void print_timespec(struct timespec* ts) {
	char buffer[100];
	// Convert seconds to struct tm
	struct tm *tm_info = localtime(&(ts->tv_sec));

	strftime(buffer, sizeof(buffer), "\t\t%Y-%m-%d %H:%M:%S %Z", tm_info);

	// Print the formatted time
	printf("%s\n", buffer);
}
