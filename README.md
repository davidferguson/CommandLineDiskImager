# CommandLineDiskImager
A command line version of Win32DiskImager

Originally modified for use in PiBakery, CommandLineDiskImager is a command line version of the popular Win32DiskImager program used to write images to removable storage devices, such as SD cards and USB drives.

Currently only writing an image file to a device is supported, you can't read a device to a file just yet. MD5 checks have also not been implemented yet.

To use:

1. Download CommandLineDiskImager.exe from [the release page](https://github.com/davidferguson/CommandLineDiskImager/releases/latest)
2. Open an elevated (administrator) Command Prompt.
3. cd into the directory that contains CommandLineDiskImager.exe
4. Run `CommandLineDiskImager.exe pathToImageFile driveLetter` where `pathToImageFile` is the path to the image file you want to write, and `driveLetter` is the single letter of the drive that you want the image written to

Example:

`CommandLineDiskImager.exe C:\Users\John\Downloads\raspbian.img G`

would write the image file located `C:\Users\John\Downloads\raspbian.img G` to the drive `G:`
