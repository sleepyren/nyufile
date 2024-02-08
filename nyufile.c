/*
Written by Renaldo Hyacinthe 
for Dr. Yang Tang's Operating Systems course
at NYU

https://download.microsoft.com/download/1/6/1/161ba512-40e2-4cc9-843a-923143f3456c/fatgen103.doc
This is Microsoft FAT32 Spec Document. All FAT32 fields are referenced from there.
*/

#include <unistd.h>
#include <sys/fcntl.h>
#include "procedure.h"



void invalidUsage();
void optionExecute(unsigned char optionConfiguration, int fd, char *recoveredFileName);


void optionExecute(unsigned char optionConfiguration, int fd, char *recoveredFileName)
{
 //printf("optionConfiguration = 0x%02x\n", optionConfiguration);
if (optionConfiguration == 0x1) printSystemInfo(fd);
if (optionConfiguration == 0x2) printRootDirectory(fd);
if (optionConfiguration == 0x4) recoverSmallFile(fd, recoveredFileName);
}

void invalidUsage()
{
    fprintf(stderr, "Usage: ./nyufile disk <options>\n"
  "-i                     Print the file system information.\n"
  "-l                     List the root directory.\n"
  "-r filename [-s sha1]  Recover a contiguous file.\n"
  "-R filename -s sha1    Recover a possibly non-contiguous file.\n");
        exit(1);
}

void printSystemInfo(int fd)
{
    //unsigned char *map = (unsigned char*) mmap(NULL, 90, PROT_READ | PROT_WRITE, MAP_SHARED, fd,0);
    BootEntry *map = (BootEntry *) mmap(NULL, 90, PROT_READ | PROT_WRITE, MAP_SHARED, fd,0);
    /*
    for (int i = 0; i < 90; i++)
        printf("0x%02x\n",  map[i]);
    */

   //YOU DO NOT NEED TO SHIFT BITS ANYMORE
   //WHEN THE COMPILER SEES A STRUCT 
   /*
    unsigned short bytesPerSector = map[11] |  map[12] << 8;
    unsigned char sectorsPerCluster = map[13];
    unsigned char numberOfFats = map[16];
    unsigned short reservedSectorCount = map[14] | map[15] << 8;
   */


printf("File System Information\nNumber of FATs: 0x%02x\nBytes per Sector: \
    0x%02x\nSectors per Cluster: 0x%02x\nNumber of Reserved Sectors: 0x%02x\n",\
     map->BPB_NumFATs, map->BPB_BytsPerSec, map->BPB_SecPerClus, map->BPB_RsvdSecCnt);
    munmap(map, 90);

}


int main(int argc, int **argv) {


    //getopt usage: https://www.gnu.org/software/libc/manual/html_node/Using-Getopt.html
    extern char *optarg;
    extern int optind; //This variable is set by getopt to the index of the next element of the argv array to be processed. 
    extern int opterr;
    opterr = 0; //setting the err value to 0 because i do not want the getopt automated err message
    optind = 2; //i want to skip the diskname in the getopt process
    int option;

    char *filePath;
    char *sha1Hash;

    unsigned char optionConfiguration = 0x0;
    //bit 0 ->  -i Print the file system information.
    //bit 1 -> -l  List the root directory
    //bit 2 -> -r OPTIONAL FIELD[-s sha1] filename 
    //Recover a contiguous file.
    //bit 3 -> -R filename -s sha1    Recover a possibly non-contiguous file.
    //bit 4 -> -s as in sha1

    int fd = open( (char *) argv[1], O_RDWR);
    if (fd < 0) {
        fprintf(stderr,"This is not a valid disk.\n");
        exit(1);
                }
    


    while ((option = getopt(argc, (char * const*) argv, "ilr:R:s:")) != -1)
    {
        if ((char) option == 'i')
            optionConfiguration = optionConfiguration | 0x1;
        

        else if ((char) option == 'l')
            optionConfiguration = optionConfiguration | 0x2;

        
        else if ((char) option == 'r')
            {optionConfiguration = optionConfiguration | 0x4;
            filePath = optarg;
            }

        else if ((char) option == 'R')
            {optionConfiguration = optionConfiguration | 0x8;
            filePath = optarg;
            }
        
        else if ((char) option == 's')
        {optionConfiguration = optionConfiguration | 0x10;
        sha1Hash = optarg;
        }

        else
        invalidUsage();

    }

    /*
    if (optionConfiguration != 0x1 && optionConfiguration != 0x2 && optionConfiguration != 0x4 && optionConfiguration\
    != 0x14 && optionConfiguration != 0x18) invalidUsage();
    else
    optionExecute(optionConfiguration, fd, NULL);
    */

    if (optionConfiguration == 0x1 || optionConfiguration == 0x2) optionExecute(optionConfiguration, fd, NULL);
    else if (optionConfiguration == 0x4) optionExecute(optionConfiguration, fd, filePath);
    else if (optionConfiguration == 0x14 || optionConfiguration == 0x18) printf("not implemented yet");
    else invalidUsage();


    close(fd);
    return 0;
}

// Other functions (if any)
