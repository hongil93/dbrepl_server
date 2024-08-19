#include "main.h"

char* time_now() {
    time_t now = time(NULL);
    char* datetime = malloc(16);
    if (datetime == NULL) {
        return NULL;
    }
    strftime(datetime, 16, "%Y%m%d_%H%M", localtime(&now));
    return datetime;
}

int count_files(const char* dir_path) {
    DIR* dir;
    struct dirent* entry;
    struct stat statbuf;
	char full_path[1024];
    int file_count = 0;

    dir = opendir(dir_path);
    if (dir == NULL) {
        perror("opendir");
        return -1;
    }

    while ((entry = readdir(dir)) != NULL) {
        // "."°ú ".."Àº ¹«½Ã
        if (entry->d_name[0] == '.' && (entry->d_name[1] == '\0' || (entry->d_name[1] == '.' && entry->d_name[2] == '\0'))) {
            continue;
        }

        snprintf(full_path, sizeof(full_path), "%s/%s", dir_path, entry->d_name);

        if (stat(full_path, &statbuf) == 0 && S_ISREG(statbuf.st_mode)) {
            file_count++;
        }
    }

    closedir(dir);
    return file_count;
}

int all_request(const char* dir_path){
	DIR* dir;
	FILE* pfile = NULL;
	char line[1024];
    struct dirent* entry;
    struct stat statbuf;
	char full_path[1024];
    int req_count = 0;

    dir = opendir(dir_path);
    if (dir == NULL) {
        perror("opendir");
        return -1;
    }

	while ((entry = readdir(dir)) != NULL) {
        if (entry->d_name[0] == '.' && (entry->d_name[1] == '\0' || (entry->d_name[1] == '.' && entry->d_name[2] == '\0'))) {
            continue;
        }

        snprintf(full_path, sizeof(full_path), "%s/%s", dir_path, entry->d_name);

        if (stat(full_path, &statbuf) == 0 && S_ISREG(statbuf.st_mode)) {
			pfile = fopen(full_path, "r");
			if(pfile == NULL){
				printf("can not open file: %s \n", full_path);
				closedir(dir);
				return -1;
			}

			if (statbuf.st_size > 0) {
				while (fgets(line, sizeof(line), pfile)) {
					if (ferror(pfile)) {
                        printf("Error reading file: %s\n", full_path);
                        break;
                    }
					line[strcspn(line, "\n")] = '\0';
					req_count++;
				}
			}

			fclose(pfile);
			pfile= NULL;
        }
    }
	
	closedir(dir);
	return req_count;
}

int calculate_tps(){
	

}