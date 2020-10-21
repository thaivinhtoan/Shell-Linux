#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>  
#include <fcntl.h>   

int main()
{
	int fd, randNum;

	printf("BEGIN: Random Number Generator device\n");

	// Open the device with READ ONLY access
	fd = open("/dev/RNGChar", O_RDONLY); 
	if (fd < 0){
		perror("Failed to open the device");
		return errno;
	}

	// Read from RNG device
	printf("Reading from RNG device\n");
	read(fd, &randNum, sizeof(randNum));
	
	printf("Random number generated = %d\n", randNum);
	printf("END\n");
	return 0;       	
}
