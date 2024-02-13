#include "procedure.h"
/*
Procedures used in FAT32 file system program
written by Renaldo Hyacinthe
for Dr. Yang Tang's Operating Systems course
at NYU

https://download.microsoft.com/download/1/6/1/161ba512-40e2-4cc9-843a-923143f3456c/fatgen103.doc
This is Microsoft FAT32 Spec Document. All FAT32 fields are referenced from there.

references:
 https://www.openssl.org/docs/man1.0.2/man3/sha.html


*/
//SHA1 NULL HEX CONSTANT USED IN PROJECT FILES
unsigned char SHA1_NULL[] = { 0xda, 0x39, 0xa3, 0xee, 0x5e, 0x6b, 0x4b, 0x0d, 0x32, 0x55, \
                                    0xbf, 0xef, 0x95, 0x60, 0x18, 0x90, 0xaf, 0xd8, 0x07, 0x09 };



//returns the HEX value of the inputted char \
of 0xFF if there is an error
unsigned char charToHex(char num)
{
    if ((unsigned char) num >= '0' && (unsigned char) num <= '9' )
        return (unsigned char) num - '0';
    
    else if ((unsigned char) num >= 'A' && (unsigned char) num <= 'F')
        return (unsigned char) num - 'A' + 10;
    
    else if ((unsigned char) num >= 'a' && (unsigned char) num <= 'f')
        return (unsigned char) num - 'a' + 10;
    else 
       return 0xFF;

}

//converts the user provided string hex, into an actual 20 byte hex
//if the provided char is 0xFF then it was not a valid char
unsigned char *stringHexToRealHex(char *string, unsigned char *buffer)
{
    for (int i = 0; i < SHA_DIGEST_LENGTH; i++)
    {
        unsigned char high4bits = charToHex(string[i*2]);

        unsigned char low4bits = charToHex(string[i*2+1]);
        
        if ((high4bits == 0xFF )|| (low4bits == 0xFF))
            return NULL;
        buffer[i] = low4bits | (high4bits << 4);
    }
return buffer;

}


//checks if a hash and the hash of a directory entry's contents are equal
int areHashesEqual(unsigned char *disk, DirEntry *directory, unsigned char hash[])
{
BootEntry *map = (BootEntry *) disk;

    unsigned int currCluster = directory->DIR_FstClusLO | directory->DIR_FstClusHI << 16;

unsigned int startingByte = map->BPB_BytsPerSec *  map->BPB_RsvdSecCnt + map->BPB_NumFATs * map->BPB_FATSz32 * \
     map->BPB_BytsPerSec + ((currCluster - 2) * map->BPB_BytsPerSec * map->BPB_SecPerClus);
     

    unsigned char *FATBegin = disk + map->BPB_BytsPerSec * map->BPB_RsvdSecCnt;
    unsigned int *FAT = (unsigned int *) FATBegin;

    unsigned int fileSize = directory->DIR_FileSize;
    if (fileSize == 0)
    {
        for (int i = 0; i < SHA_DIGEST_LENGTH; i++)
        {
            if (hash[i]!=SHA1_NULL[i])
                return -1;
        }
        printf("Hash match found\n");
        return 1;
    }
    unsigned int bufferCount = 0;
    unsigned char *buffer = (char *) malloc(fileSize);
    if (buffer==NULL) {fprintf(stderr,"Memory Allocation Failed.\n");
        exit(1);}


    while (currCluster < 0x0FFFFFF8)
    {
    
    
        for (int i = 0; i < map->BPB_BytsPerSec * map->BPB_SecPerClus; i++)
        {
            buffer[bufferCount]= disk[i+startingByte];
            bufferCount++;

            if (bufferCount == fileSize) break;
        }

        currCluster = FAT[currCluster];
        startingByte = map->BPB_BytsPerSec *  map->BPB_RsvdSecCnt + map->BPB_NumFATs * map->BPB_FATSz32 * \
     map->BPB_BytsPerSec + ((currCluster - 2) * map->BPB_BytsPerSec * map->BPB_SecPerClus);
    }


    unsigned char temphash[SHA_DIGEST_LENGTH];
    SHA1(buffer,fileSize,temphash);
    free(buffer);

    for(int i = 0; i < SHA_DIGEST_LENGTH; i++)
        if (temphash[i]!=hash[i]) return -1;
    

     printf("Hash match found\n");
    return 1;
}


//given a Directory Entry print its name, starting cluster, and size
void printDirectoryEntry(DirEntry *entry)
{
//DIR_FstClusHI = byte 20 (2 bytes)
            //DIR_FstClusLO = byte 26 (2 bytes)
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

//Recover a file based on its name
//If there is only one matching deleted file restore it
//if the optional SHAhex is given then a matching SHA 
//takes precedence
void recoverSmallFile(int fd, char *fileName, char *stringHex)
{

    
    unsigned char providedHash[SHA_DIGEST_LENGTH]={0};


    if (stringHex)
    {
     unsigned char *convertedHash = stringHexToRealHex(stringHex, providedHash); 
     stringHex = convertedHash ? stringHex : NULL;
        
    }

    

    //This is also a boolean
    //if it is 0 then none have been found so recovery failed
    //if it is one then recover the one found
    //if it is more than one then recovery possible but too ambiguous so failure
    //if a sha is provided then compare the file with the sha and return even if there are other candidates

    unsigned int matchingFileBoolean = 0;

    struct stat disk_stat;
    if (fstat(fd,&disk_stat)== -1)
    {
     fprintf(stderr,"FSTAT ON DISK FAILED\n");
        exit(1);   
    }

    
    unsigned char *disk = mmap(NULL, disk_stat.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd,0);
    BootEntry *map = (BootEntry *) disk;
    int potentialAnswerEntry ;
    
    unsigned int startingByte = map->BPB_BytsPerSec *  map->BPB_RsvdSecCnt + map->BPB_NumFATs * map->BPB_FATSz32 * \
     map->BPB_BytsPerSec + ((map->BPB_RootClus - 2) 
     * map->BPB_BytsPerSec * map->BPB_SecPerClus);

     unsigned char *FATBegin = disk + map->BPB_BytsPerSec * map->BPB_RsvdSecCnt;
    unsigned int *FAT = (unsigned int *) FATBegin;

    unsigned int currCluster = map->BPB_RootClus;


while (currCluster < 0x0FFFFFF8)
{
//for each page in the root directory check all deleted directory elements

        unsigned char *directoryPage = disk + startingByte + (currCluster - 2) * map->BPB_BytsPerSec * map->BPB_SecPerClus;
//if two elements match the provided filename, exit because this is file recovery request is\
too ambiguous to process
for (int i = 0; i < map->BPB_SecPerClus* map->BPB_BytsPerSec; i += 32)
        {
            if (directoryPage[i] != 0xe5) continue;

            char formattedDirectoryName[12] = {0};
            formatFileName(&directoryPage[i], formattedDirectoryName);
            //if all letters match except the first
            if (strcmp(formattedDirectoryName + 1, fileName + 1) == 0)
                    {
                        potentialAnswerEntry = i + startingByte;
                        

    
                        int boolin = areHashesEqual(disk, (DirEntry *) (char*)(disk + startingByte + i), providedHash);
                        if (stringHex && boolin)
                        {
                        matchingFileBoolean = 1;
                        break;
                        }


                        matchingFileBoolean++;

                    }
            
                
        }

        currCluster = FAT[currCluster];
}
if (matchingFileBoolean > 1) fprintf(stderr, "FAILURE: %s : multiple candidates found", fileName);
else if (matchingFileBoolean == 1) 
{

        unsigned char *entry = disk+potentialAnswerEntry;

    entry[0] = fileName[0];
    printf("%s successfully recovered\n", fileName);

}
else printf("%s : file not found\n", fileName);

munmap(FAT,map->BPB_FATSz32 * map->BPB_BytsPerSec);
munmap(map, disk_stat.st_size);


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


//given the fd of the disk, print it OUT
void printRootDirectory(int fd)
{

    BootEntry *map = (BootEntry *) mmap(NULL, 90, PROT_READ | PROT_WRITE, MAP_SHARED, fd,0);

    unsigned int startingByte = map->BPB_BytsPerSec *  map->BPB_RsvdSecCnt + map->BPB_NumFATs * map->BPB_FATSz32 * \
     map->BPB_BytsPerSec + ((map->BPB_RootClus - 2) 
     * map->BPB_BytsPerSec * map->BPB_SecPerClus);



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
        

        currCluster = FAT[currCluster];
    }

    munmap(FAT, map->BPB_FATSz32 * map->BPB_BytsPerSec);
    munmap(map, 90);



}
