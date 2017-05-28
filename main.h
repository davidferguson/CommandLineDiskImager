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

#ifndef MAIN
#define MAIN

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <windows.h>
#include <winioctl.h>
#include <dbt.h>
#include <shlobj.h>
#include <iostream>
#include <sstream>

#ifndef FSCTL_IS_VOLUME_MOUNTED
#define FSCTL_IS_VOLUME_MOUNTED        CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 10, METHOD_BUFFERED, FILE_ANY_ACCESS)
#endif

#define IOCTL_STORAGE_QUERY_PROPERTY   CTL_CODE(IOCTL_STORAGE_BASE, 0x0500, METHOD_BUFFERED, FILE_ANY_ACCESS)

enum Status {STATUS_IDLE=0, STATUS_READING, STATUS_WRITING, STATUS_EXIT, STATUS_CANCELED};
HANDLE hVolume;
HANDLE hFile;
HANDLE hRawDisk;
static const unsigned short ONE_SEC_IN_MS = 1000;
unsigned long long sectorsize;
int status;
char *sectorData;

HANDLE getHandleOnFile(const char *filelocation, DWORD access);
HANDLE getHandleOnDevice(int device, DWORD access);
HANDLE getHandleOnVolume(int volume, DWORD access);
bool getLockOnVolume(HANDLE handle);
bool removeLockOnVolume(HANDLE handle);
bool unmountVolume(HANDLE handle);
char *readSectorDataFromHandle(HANDLE handle, unsigned long long startsector, unsigned long long numsectors, unsigned long long sectorsize);
bool writeSectorDataToHandle(HANDLE handle, char *data, unsigned long long startsector, unsigned long long numsectors, unsigned long long sectorsize);
unsigned long long getNumberOfSectors(HANDLE handle, unsigned long long *sectorsize);
unsigned long long getFileSizeInSectors(HANDLE handle, unsigned long long sectorsize);
bool checkDriveType(char *name, ULONG *pid);
int getDriveNumberFromLetter(char lookingforname);

typedef struct _DEVICE_NUMBER
{
    DEVICE_TYPE  DeviceType;
    ULONG  DeviceNumber;
    ULONG  PartitionNumber;
} DEVICE_NUMBER, *PDEVICE_NUMBER;

typedef enum _STORAGE_PROPERTY_ID { 
  StorageDeviceProperty                  = 0,
  StorageAdapterProperty                 = 1,
  StorageDeviceIdProperty                = 2,
  StorageDeviceUniqueIdProperty          = 3,
  StorageDeviceWriteCacheProperty        = 4,
  StorageMiniportProperty                = 5,
  StorageAccessAlignmentProperty         = 6,
  StorageDeviceSeekPenaltyProperty       = 7,
  StorageDeviceTrimProperty              = 8,
  StorageDeviceWriteAggregationProperty  = 9,
  StorageDeviceDeviceTelemetryProperty   = 10, // 0xA
  StorageDeviceLBProvisioningProperty    = 11, // 0xB
  StorageDevicePowerProperty             = 12, // 0xC
  StorageDeviceCopyOffloadProperty       = 13, // 0xD
  StorageDeviceResiliencyProperty        = 14 // 0xE
} STORAGE_PROPERTY_ID, *PSTORAGE_PROPERTY_ID;

typedef struct _STORAGE_DEVICE_DESCRIPTOR {
  DWORD            Version;
  DWORD            Size;
  BYTE             DeviceType;
  BYTE             DeviceTypeModifier;
  BOOLEAN          RemovableMedia;
  BOOLEAN          CommandQueueing;
  DWORD            VendorIdOffset;
  DWORD            ProductIdOffset;
  DWORD            ProductRevisionOffset;
  DWORD            SerialNumberOffset;
  STORAGE_BUS_TYPE BusType;
  DWORD            RawPropertiesLength;
  BYTE             RawDeviceProperties[1];
} STORAGE_DEVICE_DESCRIPTOR, *PSTORAGE_DEVICE_DESCRIPTOR;

typedef enum _STORAGE_QUERY_TYPE { 
  PropertyStandardQuery    = 0,
  PropertyExistsQuery,
  PropertyMaskQuery,
  PropertyQueryMaxDefined
} STORAGE_QUERY_TYPE, *PSTORAGE_QUERY_TYPE;

typedef struct _STORAGE_PROPERTY_QUERY {
  STORAGE_PROPERTY_ID PropertyId;
  STORAGE_QUERY_TYPE  QueryType;
  BYTE                AdditionalParameters[1];
} STORAGE_PROPERTY_QUERY, *PSTORAGE_PROPERTY_QUERY;

#endif