#include "procedure.h"


void printDirectoryEntry(DirEntry *entry)
{
//DIR_FstClusHI = byte 20 (2 bytes)
            //DIR_FstClusLO = byte 26 (2 bytes)
           // unsigned int clusterNumber = entry[26] | entry[27] << 8 | entry[20] << 16 | entry[21] << 24;
            unsigned int clusterNumber = entry->DIR_FstClusLO | entry->DIR_FstClusHI << 16;

            //DIR_FileSize = byte 28 (4 bytes)
            //unsigned int fileSize = entry[28] | entry[29] << 8 | entry[30] << 16 | entry[31] << 24;
            unsigned int fileSize = entry->DIR_FileSize;

             //char isDirectory = entry[11] & 0x10;
            char isDirectory = entry->DIR_Attr & 0x10;

            char formatted_string[12]={0};
            formatFileName( (char *) entry, formatted_string);
           //printf("%.11s/  \nstarting cluster: %u\nsize: %u \n\n", entry + i, clusterNumber, fileSize); 
            printf("%.11s \nstarting cluster: %u\nsize: %u \n\n", formatted_string, clusterNumber, fileSize); 
}

void recoverSmallFile(int fd, char *fileName)
{

    int fileFoundBoolean = 0;
    BootEntry *map = (BootEntry *) mmap(NULL, 90, PROT_READ | PROT_WRITE, MAP_SHARED, fd,0);

    
    unsigned int startingByte = map->BPB_BytsPerSec *  map->BPB_RsvdSecCnt + map->BPB_NumFATs * map->BPB_FATSz32 * \
     map->BPB_BytsPerSec + ((map->BPB_RootClus - 2) 
     * map->BPB_BytsPerSec * map->BPB_SecPerClus);

    unsigned int *FAT =  (unsigned int *) mmap(NULL, map->BPB_FATSz32 * map->BPB_BytsPerSec, PROT_READ, MAP_PRIVATE, fd, \
     map->BPB_BytsPerSec *  map->BPB_RsvdSecCnt);

    unsigned int currCluster = map->BPB_RootClus;
    


while (currCluster < 0x0FFFFFF8)
{
//for each page in the root directory check all deleted directory elements
        unsigned char *directoryPage = mmap(NULL, map->BPB_FATSz32 * map->BPB_BytsPerSec \
        , PROT_READ | PROT_WRITE, MAP_SHARED, fd, startingByte + (currCluster - 2) * map->BPB_BytsPerSec * map->BPB_SecPerClus);

for (int i = 0; i < map->BPB_FATSz32 * map->BPB_BytsPerSec; i += 32)
        {
            if (directoryPage[i] != 0xe5) continue;

            char formattedDirectoryName[12] = {0};
            formatFileName(&directoryPage[i], formattedDirectoryName);
            if (strcmp(formattedDirectoryName + 1, fileName + 1) == 0)
                    {
                        fileFoundBoolean = 1;
                    directoryPage[i] = fileName[0];
                        printf("\n%s successfully recovered\n", fileName);
                    break;//because our work is done here
                    }
            
                
        }
        munmap(directoryPage, map->BPB_FATSz32 * map->BPB_BytsPerSec);
        currCluster = FAT[currCluster];
}
if (!fileFoundBoolean) printf("%s : file not found\n", fileName);

munmap(FAT,map->BPB_FATSz32 * map->BPB_BytsPerSec);
munmap(map, 90);


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



    
    //unsigned char *FAT = (unsigned char*) mmap(NULL, map->BPB_FATSz32 * map->BPB_BytsPerSec, PROT_READ, MAP_PRIVATE, fd, \
     map->BPB_BytsPerSec *  map->BPB_RsvdSecCnt);
    unsigned int *FAT =  (unsigned int *) mmap(NULL, map->BPB_FATSz32 * map->BPB_BytsPerSec, PROT_READ, MAP_PRIVATE, fd, \
     map->BPB_BytsPerSec *  map->BPB_RsvdSecCnt);

    unsigned int currCluster = map->BPB_RootClus;
    int numberOfEntries = 0;

    while (currCluster < 0x0FFFFFF8) // >= 0x0FFFFFF8 is End of Cluster mark
    {

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


            printDirectoryEntry( (DirEntry *) &directoryPage[i]);
            numberOfEntries++;
            //•	If DIR_Name[0] == 0x05, then the actual file name character for this byte is 0xE5. \
            0xE5 is actually a valid KANJI lead byte value for the character set used in Japan.

        }
        printf("number of entries: %d\n", numberOfEntries);
        munmap(directoryPage, map->BPB_FATSz32 * map->BPB_BytsPerSec);
        
        //currCluster = FAT[currCluster*4] | FAT[currCluster*4+1] << 8 | FAT[currCluster*4+2] << 16 | FAT[currCluster*4+3] << 24 ;
        currCluster = FAT[currCluster];
    }

    munmap(FAT, map->BPB_FATSz32 * map->BPB_BytsPerSec);
    munmap(map, 90);



}
