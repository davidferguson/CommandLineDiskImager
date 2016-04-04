/**********************************************************************
 *  This program is free software; you can redistribute it and/or     *
 *  modify it under the terms of the GNU General Public License       *
 *  as published by the Free Software Foundation; either version 2    *
 *  of the License, or (at your option) any later version.            *
 *                                                                    *
 *  This program is distributed in the hope that it will be useful,   *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of    *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the     *
 *  GNU General Public License for more details.                      *
 *                                                                    *
 *  You should have received a copy of the GNU General Public License *
 *  along with this program; if not, see http://gnu.org/licenses/
 *  ---                                                               *
 *  Copyright (C) 2009, Justin Davis <tuxdavis@gmail.com>             *
 *  Copyright (C) 2009-2014 ImageWriter developers                    *
 *                          https://launchpad.net/~image-writer-devs  *
 *  Copyright (C) 2016, David Ferguson <fergusondavid6@gmail.com>     *
 **********************************************************************/

#ifndef WINVER
#define WINVER 0x0601
#endif

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <windows.h>
#include <winioctl.h>
#include <dbt.h>
#include <shlobj.h>
#include <iostream>
#include <sstream>

#include "main.h"


int main(int argc, char *argv[])
{
    if ( argc != 3 )
    {
        printf("error - not enough arguments");
        return 1;
    }

    const char ltr = argv[2][0];
    const char *imagefile = argv[1];
    printf("writing file %s to device %c:", imagefile, ltr);

    status = STATUS_IDLE;
    hVolume = INVALID_HANDLE_VALUE;
    hFile = INVALID_HANDLE_VALUE;
    hRawDisk = INVALID_HANDLE_VALUE;
    sectorData = NULL;
    sectorsize = 0ul;

    if (strcmp( imagefile, "") != 0)
    {
        if(INVALID_FILE_ATTRIBUTES == GetFileAttributes(imagefile) && GetLastError()==ERROR_FILE_NOT_FOUND)
        {
            printf("\nerror - file not found");
            return 1;
        }
        else
        {
            status = STATUS_WRITING;

            unsigned long long i, availablesectors, numsectors;
            int volumeID = ltr - 'A';
            int deviceID = getDriveNumberFromLetter(ltr);
            if( deviceID == -1 )
            {
                printf("\nerror - device not found");
                return 1;
            }

            hVolume = getHandleOnVolume(volumeID, GENERIC_WRITE);
            if (hVolume == INVALID_HANDLE_VALUE)
            {
                status = STATUS_IDLE;
                printf("\nerror - invalid handle value for volume");
                return 1;
            }
            if (!getLockOnVolume(hVolume))
            {
                CloseHandle(hVolume);
                status = STATUS_IDLE;
                hVolume = INVALID_HANDLE_VALUE;
                printf("\nerror - can't get a lock on volume");
                return 1;
            }
            if (!unmountVolume(hVolume))
            {
                removeLockOnVolume(hVolume);
                CloseHandle(hVolume);
                status = STATUS_IDLE;
                hVolume = INVALID_HANDLE_VALUE;
                printf("\nerror - can't unmount volume");
                return 1;
            }
            hFile = getHandleOnFile(imagefile, GENERIC_READ);
            if (hFile == INVALID_HANDLE_VALUE)
            {
                removeLockOnVolume(hVolume);
                CloseHandle(hVolume);
                status = STATUS_IDLE;
                hVolume = INVALID_HANDLE_VALUE;
                printf("\nerror - invalid handle value for file");
                return 1;
            }
            hRawDisk = getHandleOnDevice(deviceID, GENERIC_WRITE);
            if (hRawDisk == INVALID_HANDLE_VALUE)
            {
                removeLockOnVolume(hVolume);
                CloseHandle(hFile);
                CloseHandle(hVolume);
                status = STATUS_IDLE;
                hVolume = INVALID_HANDLE_VALUE;
                hFile = INVALID_HANDLE_VALUE;
                printf("\nerror - invalid handle value for disk");
                return 1;
            }
            availablesectors = getNumberOfSectors(hRawDisk, &sectorsize);
            numsectors = getFileSizeInSectors(hFile, sectorsize);
            if (numsectors > availablesectors)
            {
                printf("\nerror - not enough space");
                return 1;
            }
            for (i = 0ul; i < numsectors && status == STATUS_WRITING; i += 1024ul)
            {
                sectorData = readSectorDataFromHandle(hFile, i, (numsectors - i >= 1024ul) ? 1024ul:(numsectors - i), sectorsize);
                if (sectorData == NULL)
                {
                    removeLockOnVolume(hVolume);
                    CloseHandle(hRawDisk);
                    CloseHandle(hFile);
                    CloseHandle(hVolume);
                    status = STATUS_IDLE;
                    hRawDisk = INVALID_HANDLE_VALUE;
                    hFile = INVALID_HANDLE_VALUE;
                    hVolume = INVALID_HANDLE_VALUE;
                    printf("\nerror - data is null");
                    return 1;
                }
                if (!writeSectorDataToHandle(hRawDisk, sectorData, i, (numsectors - i >= 1024ul) ? 1024ul:(numsectors - i), sectorsize))
                {
                    delete[] sectorData;
                    removeLockOnVolume(hVolume);
                    CloseHandle(hRawDisk);
                    CloseHandle(hFile);
                    CloseHandle(hVolume);
                    status = STATUS_IDLE;
                    sectorData = NULL;
                    hRawDisk = INVALID_HANDLE_VALUE;
                    hFile = INVALID_HANDLE_VALUE;
                    hVolume = INVALID_HANDLE_VALUE;
                    printf("\nerror - error whilst writing");
                    return 1;
                }
                delete[] sectorData;
                sectorData = NULL;
            }
            removeLockOnVolume(hVolume);
            CloseHandle(hRawDisk);
            CloseHandle(hFile);
            CloseHandle(hVolume);
            hRawDisk = INVALID_HANDLE_VALUE;
            hFile = INVALID_HANDLE_VALUE;
            hVolume = INVALID_HANDLE_VALUE;
        }
        printf("\ninfo - write successful");
        return 0;
    }
    else
    {
        printf("\nerror - no image file specified");
        return 1;
    }
    status = STATUS_IDLE;
    return 1;
}

int getDriveNumberFromLetter(char lookingforname)
{
    unsigned long driveMask = GetLogicalDrives();
    int i = 0;
    ULONG pID;

    while (driveMask != 0)
    {
        if (driveMask & 1)
        {
            // the "A" in drivename will get incremented by the # of bits
            // we've shifted
            char drivename[] = "\\\\.\\A:\\";
            drivename[4] += i;

            if (checkDriveType(drivename, &pID))
            {
                if (lookingforname == drivename[4])
                {
                    return pID;
                }
            }
        }
        driveMask >>= 1;
        ++i;
    }
    return -1;
}

HANDLE getHandleOnFile(const char *filelocation, DWORD access)
{
    HANDLE hFile;
    hFile = CreateFileA(filelocation, access, (access == GENERIC_READ) ? FILE_SHARE_READ : 0, NULL, (access == GENERIC_READ) ? OPEN_EXISTING:CREATE_ALWAYS, 0, NULL);
    if (hFile == INVALID_HANDLE_VALUE)
    {
        printf("\nerror - not able to get handle on image file");
    }
    return hFile;
}

HANDLE getHandleOnDevice(int device, DWORD access)
{
    HANDLE hDevice;
    char devicename[18];
    sprintf(devicename, "\\\\.\\PhysicalDrive%d", device);

    hDevice = CreateFile(devicename, access, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
    if (hDevice == INVALID_HANDLE_VALUE)
    {
        printf("\nerror - not able to get handle on device");
    }
    return hDevice;
}

HANDLE getHandleOnVolume(int volume, DWORD access)
{
    HANDLE hVolume;
    char volumename[] = "\\\\.\\A:";
    volumename[4] += volume;

    hVolume = CreateFile(volumename, access, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
    if (hVolume == INVALID_HANDLE_VALUE)
    {
        printf("\nerror - not able to get handle on volume");
    }
    return hVolume;
}

bool getLockOnVolume(HANDLE handle)
{
    DWORD bytesreturned;
    BOOL bResult;
    bResult = DeviceIoControl(handle, FSCTL_LOCK_VOLUME, NULL, 0, NULL, 0, &bytesreturned, NULL);
    if (!bResult)
    {
        printf("\nerror - not able to lock volume");
    }
    return (bResult);
}

bool removeLockOnVolume(HANDLE handle)
{
    DWORD junk;
    BOOL bResult;
    bResult = DeviceIoControl(handle, FSCTL_UNLOCK_VOLUME, NULL, 0, NULL, 0, &junk, NULL);
    if (!bResult)
    {
        printf("\nerror - not able to unlock volume");
    }
    return (bResult);
}

bool unmountVolume(HANDLE handle)
{
    DWORD junk;
    BOOL bResult;
    bResult = DeviceIoControl(handle, FSCTL_DISMOUNT_VOLUME, NULL, 0, NULL, 0, &junk, NULL);
    if (!bResult)
    {
        printf("\nerror - not able to dismount volume");
    }
    return (bResult);
}

char *readSectorDataFromHandle(HANDLE handle, unsigned long long startsector, unsigned long long numsectors, unsigned long long sectorsize)
{
    unsigned long bytesread;
    char *data = new char[sectorsize * numsectors];
    LARGE_INTEGER li;
    li.QuadPart = startsector * sectorsize;
    SetFilePointer(handle, li.LowPart, &li.HighPart, FILE_BEGIN);
    if (!ReadFile(handle, data, sectorsize * numsectors, &bytesread, NULL))
    {
        printf("\nerror - not able to read data from handle");
        delete[] data;
        data = NULL;
    }
    if (bytesread < (sectorsize * numsectors))
    {
            memset(data + bytesread,0,(sectorsize * numsectors) - bytesread);
    }
    return data;
}

bool writeSectorDataToHandle(HANDLE handle, char *data, unsigned long long startsector, unsigned long long numsectors, unsigned long long sectorsize)
{
    unsigned long byteswritten;
    BOOL bResult;
    LARGE_INTEGER li;
    li.QuadPart = startsector * sectorsize;
    SetFilePointer(handle, li.LowPart, &li.HighPart, FILE_BEGIN);
    bResult = WriteFile(handle, data, sectorsize * numsectors, &byteswritten, NULL);
    if (!bResult)
    {
        printf("\nerror - not able to write data from handle");
    }
    return (bResult);
}

unsigned long long getNumberOfSectors(HANDLE handle, unsigned long long *sectorsize)
{
    DWORD junk;
    DISK_GEOMETRY_EX diskgeometry;
    BOOL bResult;
    bResult = DeviceIoControl(handle, IOCTL_DISK_GET_DRIVE_GEOMETRY_EX, NULL, 0, &diskgeometry, sizeof(diskgeometry), &junk, NULL);
    if (!bResult)
    {
        printf("\nerror - not able to get number of sectors");
        return 0;
    }
    if (sectorsize != NULL)
    {
        *sectorsize = (unsigned long long)diskgeometry.Geometry.BytesPerSector;
    }
    return (unsigned long long)diskgeometry.DiskSize.QuadPart / (unsigned long long)diskgeometry.Geometry.BytesPerSector;
}

unsigned long long getFileSizeInSectors(HANDLE handle, unsigned long long sectorsize)
{
    unsigned long long retVal = 0;
    LARGE_INTEGER filesize;
    if(GetFileSizeEx(handle, &filesize) == 0)
    {
        printf("\nerror - not able to get image file size");
        retVal = 0;
    }
    else
    {
        retVal = ((unsigned long long)filesize.QuadPart / sectorsize ) + (((unsigned long long)filesize.QuadPart % sectorsize )?1:0);
    }
    return(retVal);
}

BOOL GetDisksProperty(HANDLE hDevice, PSTORAGE_DEVICE_DESCRIPTOR pDevDesc,
                      DEVICE_NUMBER *devInfo)
{
    STORAGE_PROPERTY_QUERY Query; // input param for query
    DWORD dwOutBytes; // IOCTL output length
    BOOL bResult; // IOCTL return val
    BOOL retVal = true;
    DWORD cbBytesReturned;

    // specify the query type
    Query.PropertyId = StorageDeviceProperty;
    Query.QueryType = PropertyStandardQuery;

    // Query using IOCTL_STORAGE_QUERY_PROPERTY
    bResult = ::DeviceIoControl(hDevice, IOCTL_STORAGE_QUERY_PROPERTY,
                &Query, sizeof(STORAGE_PROPERTY_QUERY), pDevDesc,
                pDevDesc->Size, &dwOutBytes, (LPOVERLAPPED)NULL);
    if (bResult)
    {
        bResult = ::DeviceIoControl(hDevice, IOCTL_STORAGE_GET_DEVICE_NUMBER,
                    NULL, 0, devInfo, sizeof(DEVICE_NUMBER), &dwOutBytes,
                    (LPOVERLAPPED)NULL);
        if (!bResult)
        {
            retVal = false;
            printf("\nerror - not able to get device number, is something accessing the device?");
        }
    }
    else
    {
        if (DeviceIoControl(hDevice, IOCTL_STORAGE_CHECK_VERIFY2, NULL, 0, NULL, 0, &cbBytesReturned,
                            (LPOVERLAPPED) NULL))
        {
            printf("\nerror - not able to get device properties, is something accessing the device?");
        }
            retVal = false;
    }

    return(retVal);
}

bool slashify(char *str, char **slash, char **noSlash)
{
    bool retVal = false;
    int strLen = strlen(str);
    if ( strLen > 0 )
    {
        if ( *(str + strLen - 1) == '\\' )
        {
            // trailing slash exists
            if (( (*slash = (char *)calloc( (strLen + 1), sizeof(char))) != NULL) &&
                    ( (*noSlash = (char *)calloc(strLen, sizeof(char))) != NULL))
            {
                strncpy(*slash, str, strLen);
                strncpy(*noSlash, *slash, (strLen - 1));
                retVal = true;
            }
        }
        else
        {
            // no trailing slash exists
            if ( ((*slash = (char *)calloc( (strLen + 2), sizeof(char))) != NULL) &&
                 ((*noSlash = (char *)calloc( (strLen + 1), sizeof(char))) != NULL) )
            {
                strncpy(*noSlash, str, strLen);
                sprintf(*slash, "%s\\", *noSlash);
                retVal = true;
            }
        }
    }
    return(retVal);
}

bool checkDriveType(char *name, ULONG *pid)
{
    HANDLE hDevice;
    PSTORAGE_DEVICE_DESCRIPTOR pDevDesc;
    DEVICE_NUMBER deviceInfo;
    bool retVal = false;
    char *nameWithSlash;
    char *nameNoSlash;
    int driveType;
    DWORD cbBytesReturned;

    // some calls require no tailing slash, some require a trailing slash...
    if ( !(slashify(name, &nameWithSlash, &nameNoSlash)) )
    {
        return(retVal);
    }

    driveType = GetDriveType(nameWithSlash);
    switch( driveType )
    {
    case DRIVE_REMOVABLE: // The media can be removed from the drive.
    case DRIVE_FIXED:     // The media cannot be removed from the drive.
        hDevice = CreateFile(nameNoSlash, FILE_READ_ATTRIBUTES, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
        if (hDevice == INVALID_HANDLE_VALUE)
        {
            printf("\nerror - not able to get handle on device");
        }
        else
        {
            int arrSz = sizeof(STORAGE_DEVICE_DESCRIPTOR) + 512 - 1;
            pDevDesc = (PSTORAGE_DEVICE_DESCRIPTOR)new BYTE[arrSz];
            pDevDesc->Size = arrSz;

            // get the device number if the drive is
            // removable or (fixed AND on the usb bus, SD, or MMC (undefined in XP/mingw))
            if(GetDisksProperty(hDevice, pDevDesc, &deviceInfo) &&
                    ( ((driveType == DRIVE_REMOVABLE) && (pDevDesc->BusType != BusTypeSata))
                      || ( (driveType == DRIVE_FIXED) && ((pDevDesc->BusType == BusTypeUsb)
                      || (pDevDesc->BusType == /*BusTypeSd*/0xC ) || (pDevDesc->BusType == /*BusTypeMmc*/0xD )) ) ) )
            {
                // ensure that the drive is actually accessible
                // multi-card hubs were reporting "removable" even when empty
                if(DeviceIoControl(hDevice, IOCTL_STORAGE_CHECK_VERIFY2, NULL, 0, NULL, 0, &cbBytesReturned, (LPOVERLAPPED) NULL))
                {
                    *pid = deviceInfo.DeviceNumber;
                    retVal = true;
                }
                else
                    // IOCTL_STORAGE_CHECK_VERIFY2 fails on some devices under XP/Vista, try the other (slower) method, just in case.
                {
                    CloseHandle(hDevice);
                    hDevice = CreateFile(nameNoSlash, FILE_READ_DATA, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
                    if(DeviceIoControl(hDevice, IOCTL_STORAGE_CHECK_VERIFY, NULL, 0, NULL, 0, &cbBytesReturned, (LPOVERLAPPED) NULL))
                    {
                        *pid = deviceInfo.DeviceNumber;
                        retVal = true;
                    }
                }
            }

            delete[] pDevDesc;
            CloseHandle(hDevice);
        }

        break;
    default:
        retVal = false;
    }

    // free the strings allocated by slashify
    free(nameWithSlash);
    free(nameNoSlash);

    return(retVal);
}
