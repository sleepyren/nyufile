# nyufile

FAT32 has been around for over 25 years. Because of its simplicity, it is the most widely compatible file system. Although recent computers have adopted newer file systems, FAT32 (and its variant, exFAT) is still dominant in SD cards and USB flash drives due to its compatibility.

Have you ever accidentally deleted a file? Do you know that it could be recovered? In this lab, you will build a FAT32 file recovery tool called Need You to Undelete my FILE, or nyufile for short.


This is my implementation
# from Professor Yang Tang's Operating Systems Course

# Usage

Usage: ./nyufile disk <options>
-i                     Print the file system information.
-l                     List the root directory.
-r filename [-s sha1]  Recover a contiguous file.
