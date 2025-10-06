#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <syslog.h>


int main(int argc, char *argv[]) {
    
    openlog(NULL, LOG_PERROR|LOG_PID, LOG_USER);
    

    // TODO: write to syslog
    if (argc < 3) {
        syslog(LOG_ERR, "Usage: %s <string>\n", argv[0]);
        return 1;
    }
    
    const char* write_file = argv[1];
    const char* write_str = argv[2];
    
    FILE* fd = fopen(write_file, "w");
    
    if (!fd) {
        syslog(LOG_ERR, "Error: cannot open the file '%s': = %s\n", write_file, strerror(errno));
        return 1;
    }
    
    if (fputs(write_str, fd) == EOF) {
        syslog(LOG_ERR, "Error: cannot write the file '%s': = %s\n", write_file, strerror(errno));
        return 1;
    }
    
    if (fclose(fd) == EOF) {
        syslog(LOG_ERR, "Error: cannot close the file '%s': = %s\n", write_file, strerror(errno));
        return 1;
    }
    
    syslog(LOG_DEBUG, "Writing %s to %s", write_str, write_file);
    
    closelog();
    
    return 0;
}