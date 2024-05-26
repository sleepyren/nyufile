# Project Description

Have you ever accidentally deleted a file? Do you know that it could be recovered? This project, Need You to Undelete my FILE (nyufile), is a FAT32 file recovery tool. FAT32, a simple and widely compatible file system, is still prevalent in SD cards and USB flash drives. This tool helps recover deleted files from FAT32 systems.

## Features

    1. Validate Usage
    2. Print File System Information
    3. List Root Directory
    4. Recover Small Files
    5. Recover Large Contiguously-Allocated Files
    6. Detect Ambiguous File Recovery Requests
    7. Recover Contiguously-Allocated Files with SHA-1 Hash

When multiple deleted entries with the same name are found, the program detects ambiguity and requests the user to provide a SHA-1 hash for precise recovery.

## Usage
```
./nyufile disk [options]
-i                     Print the file system information.
-l                     List the root directory.
-r filename [-s sha1]  Recover a contiguous file.
```

## Build and Run

To build the project:
```
make
```

### Recommended Environment:
Run this tool in a virtualized environment like a Linux Docker image due to its nature. The provided LinuxKit image can be used:

```
docker pull ytang/os
docker run -it --privileged --rm -v /CHOOSE_DIRECTORY:/cs202 -w /cs202 ytang/os bash
```

Your current directory will be placed in /cs202/ and any changes outside of /cs202 will not be reflected.
### Creating a Test FAT32 Disk

To create a small FAT32 disk for program testing:

1. Create a 256KB empty file named fat32.disk:
```
dd if=/dev/zero of=fat32.disk bs=256k count=1
```

2. Format the disk with FAT32
```
mkfs.fat -F 32 -f 2 -S 512 -s 1 -R 32 fat32.disk
```

3. Create a mounting point and mount the disk
```
mkdir /mnt/disk
mount fat32.disk /mnt/disk
```
4. When done unmount the disk
```
umount fat32.disk
```

### Code Overview

The main functionalities of the nyufile tool are implemented in the procedure.c file. Here's an overview of some key functions:

    restoreDeletedFATEntries: Restores the FAT entries of a deleted directory based on file size and cluster size.
    charToHex & stringHexToRealHex: Convert string representations of hex values to actual hex values.
    areHashesEqual: Checks if a hash and the theoretical hash of a deleted directory entry’s contents are equal.
    printDirectoryEntry: Prints the details of a directory entry.
    recoverSmallFile: Recovers a file based on its name, considering SHA-1 hash for ambiguity resolution.
    printRootDirectory: Prints the root directory of the FAT32 file system.
    printSystemInfo: Prints the file system information.
### Example Code Snippets

```
void restoreDeletedFATEntries(unsigned int OSclusterSize, unsigned int *FAT, DirEntry *directory) {
    unsigned int fileSize = directory->DIR_FileSize;
    unsigned int fileClusterCount = (fileSize % OSclusterSize == 0) ? (fileSize / OSclusterSize) : (fileSize / OSclusterSize + 1);
    unsigned int directoryCluster = directory->DIR_FstClusLO | (directory->DIR_FstClusHI << 16);
    for (unsigned int i = 0; i < fileClusterCount; i++) {
        if (i == fileClusterCount - 1) {
            FAT[i + directoryCluster] = FAT[1]; // End of chain
            break;
        }
        FAT[i + directoryCluster] = directoryCluster + i + 1;
    }
}
```

### Additional Resources

[Microsoft FAT32 Specification Document](https://download.microsoft.com/download/1/6/1/161ba512-40e2-4cc9-843a-923143f3456c/fatgen103.doc)
[OpenSSL SHA Documentation](https://www.openssl.org/docs/man1.0.2/man3/sha.html)
