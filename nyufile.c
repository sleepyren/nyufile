/*
Written by Renaldo Hyacinthe 
for Dr. Yang Tang's Operating Systems course
at NYU

https://download.microsoft.com/download/1/6/1/161ba512-40e2-4cc9-843a-923143f3456c/fatgen103.doc
This is Microsoft FAT32 Spec Document. All FAT32 fields are referenced from there.
*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/fcntl.h>
#include <sys/mman.h>

#pragma pack(push,1)
typedef struct BootEntry {
  unsigned char  BS_jmpBoot[3];     // Assembly instruction to jump to boot code
  unsigned char  BS_OEMName[8];     // OEM Name in ASCII
  unsigned short BPB_BytsPerSec;    // Bytes per sector. Allowed values include 512, 1024, 2048, and 4096
  unsigned char  BPB_SecPerClus;    // Sectors per cluster (data unit). Allowed values are powers of 2, but the cluster size must be 32KB or smaller
  unsigned short BPB_RsvdSecCnt;    // Size in sectors of the reserved area
  unsigned char  BPB_NumFATs;       // Number of FATs
  unsigned short BPB_RootEntCnt;    // Maximum number of files in the root directory for FAT12 and FAT16. This is 0 for FAT32
  unsigned short BPB_TotSec16;      // 16-bit value of number of sectors in file system
  unsigned char  BPB_Media;         // Media type
  unsigned short BPB_FATSz16;       // 16-bit size in sectors of each FAT for FAT12 and FAT16. For FAT32, this field is 0
  unsigned short BPB_SecPerTrk;     // Sectors per track of storage device
  unsigned short BPB_NumHeads;      // Number of heads in storage device
  unsigned int   BPB_HiddSec;       // Number of sectors before the start of partition
  unsigned int   BPB_TotSec32;      // 32-bit value of number of sectors in file system. Either this value or the 16-bit value above must be 0
  unsigned int   BPB_FATSz32;       // 32-bit size in sectors of one FAT
  unsigned short BPB_ExtFlags;      // A flag for FAT
  unsigned short BPB_FSVer;         // The major and minor version number
  unsigned int   BPB_RootClus;      // Cluster where the root directory can be found
  unsigned short BPB_FSInfo;        // Sector where FSINFO structure can be found
  unsigned short BPB_BkBootSec;     // Sector where backup copy of boot sector is located
  unsigned char  BPB_Reserved[12];  // Reserved
  unsigned char  BS_DrvNum;         // BIOS INT13h drive number
  unsigned char  BS_Reserved1;      // Not used
  unsigned char  BS_BootSig;        // Extended boot signature to identify if the next three values are valid
  unsigned int   BS_VolID;          // Volume serial number
  unsigned char  BS_VolLab[11];     // Volume label in ASCII. User defines when creating the file system
  unsigned char  BS_FilSysType[8];  // File system type label in ASCII
} BootEntry;
#pragma pack(pop)

void invalidUsage();
void optionExecute(unsigned char optionConfiguration, int fd);
void printSystemInfo(int fd);
void printRootDirectory(int fd);
void formatFileName(char *directoryEntry, char *formatted_string);
void recoverSmallFile(char *directoryEntry, char *fileName);

void printDirectoryEntry(char *entry)
{
//DIR_FstClusHI = byte 20 (2 bytes)
            //DIR_FstClusLO = byte 26 (2 bytes)
            unsigned int clusterNumber = entry[26] | entry[27] << 8 |
            entry[20] << 16 | entry[21] << 24;

            //DIR_FileSize = byte 28 (4 bytes)
            unsigned int fileSize = entry[28] | entry[29] << 8 |
             entry[30] << 16 | entry[31] << 24;
           
            char isDirectory = entry[11] & 0x10;

            char formatted_string[12]={0};
            formatFileName(entry, formatted_string);
           //printf("%.11s/  \nstarting cluster: %u\nsize: %u \n\n", entry + i, clusterNumber, fileSize); 
            printf("%.11s \nstarting cluster: %u\nsize: %u \n\n", formatted_string, clusterNumber, fileSize); 
}

void recoverSmallFile(char *directoryEntry, char *fileName)
{

}

void formatFileName(char *directoryEntry, char *formatted_string)
{
            char isDirectory = directoryEntry[11] & 0x10;
        short position=0;
            for (int i = 0; i < 11; i++)
            {
                if (isDirectory && i == 8) break;

                if (!isDirectory && i ==8) {formatted_string[position] = '.'; position++;}

                if (directoryEntry[i] != ' ')
                {formatted_string[position] = directoryEntry[i]; position++;}
                else continue;
            }
            //if the directory attribute is set at a bit 5
           if (isDirectory) formatted_string[position] = '/';
           return;

}

void optionExecute(unsigned char optionConfiguration, int fd)
{
 //printf("optionConfiguration = 0x%02x\n", optionConfiguration);
if (optionConfiguration == 0x1) printSystemInfo(fd);
if (optionConfiguration == 0x2) printRootDirectory(fd);
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
   //WHEN THE COMPILER SEES A STRUCT IT TRANSLATES LITTLE ENDIAN TO HOST ENDIAN
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

void printRootDirectory(int fd)
{

    BootEntry *map = (BootEntry *) mmap(NULL, 90, PROT_READ | PROT_WRITE, MAP_SHARED, fd,0);
    //unsigned short bytesPerSector = map[11] |  map[12] << 8;
    
    //unsigned char sectorsPerCluster = map[13];
    //unsigned char numberOfFats = map[16];
    //unsigned short reservedSectorCount = map[14] | map[15] << 8;
    //unsigned int FATsize = map[36] | map[37] << 8 | map[38] << 16 | map[39] << 24;
    //unsigned int rootClusterNumber = map[44] | map[45] << 8 | map[46] << 16 | (map[47] << 24 & 0x0F);


    unsigned int startingByte = map->BPB_BytsPerSec *  map->BPB_RsvdSecCnt + map->BPB_NumFATs * map->BPB_FATSz32 * \
     map->BPB_BytsPerSec + ((map->BPB_RootClus - 2) 
     * map->BPB_BytsPerSec * map->BPB_SecPerClus);


    //This new map is the FAT region
    unsigned char *FAT = (unsigned char*) mmap(NULL, map->BPB_FATSz32 * map->BPB_BytsPerSec, PROT_READ, MAP_PRIVATE, fd, \
     map->BPB_BytsPerSec *  map->BPB_RsvdSecCnt);
    

    unsigned int currCluster = map->BPB_RootClus;
    int numberOfEntries = 0;

    while (currCluster < 0x0FFFFFF8) // >= 0x0FFFFFF8 is End of Cluster mark
    {
     //printf("currCluster : %u ", currCluster);

        //for each page in the root directory print all valid directory elements
        unsigned char *directoryPage = mmap(NULL, map->BPB_FATSz32 * map->BPB_BytsPerSec \
        , PROT_READ, MAP_PRIVATE, fd, startingByte + (currCluster - 2) * map->BPB_BytsPerSec * map->BPB_SecPerClus);
        

        for (int i = 0; i < map->BPB_BytsPerSec * map->BPB_SecPerClus; i += 32)
        {

            //If DIR_Name[0] == 0xE5, then the directory entry is free 
            //DIR_NAME[11] lower 4 bits one (0x0F) means that this directory entry belongs to a LFN (Long File Name) entry
            //which we will not be dealing with in this version
            if (directoryPage[i] == 0xE5 || directoryPage[i+11] & 0x0F == 0x0F) continue;


            //If DIR_Name[0] == 0x00, then the directory entry is free, and there are no allocated directory entries after this one 
            if (directoryPage[i] == 0x00) break;


            printDirectoryEntry(&directoryPage[i]);
            //•	If DIR_Name[0] == 0x05, then the actual file name character for this byte is 0xE5. \
            0xE5 is actually a valid KANJI lead byte value for the character set used in Japan.

        }
        munmap(directoryPage, map->BPB_FATSz32 * map->BPB_BytsPerSec);
        currCluster = FAT[currCluster*4] | FAT[currCluster*4+1] << 8 | FAT[currCluster*4+2] << 16 | FAT[currCluster*4+3] << 24 ;
        //currCluster = FAT[currCluster*4];
    }
    munmap(FAT, map->BPB_FATSz32 * map->BPB_BytsPerSec);
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

    if (optionConfiguration != 0x1 && optionConfiguration != 0x2 && optionConfiguration != 0x4 && optionConfiguration\
    != 0x14 && optionConfiguration != 0x18) invalidUsage();
    else
    optionExecute(optionConfiguration, fd);


    close(fd);
    return 0;
}

// Other functions (if any)
