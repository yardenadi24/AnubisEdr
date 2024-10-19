#pragma once
#include <ntddk.h>

// Define the IOCTL code
#define FILE_DEVICE_ANUBIS 0x00008335

// METHOD_BUFFERED : 
//      Data from the user buffer is copied to the system buffer
//      The driver read the data from the system buffer, if the operation
//      was read, the data is then being copied to the user buffer.
// DIRECT IO: (Direct in\ Direct out)
//      the i\o manager locks the user memory from being paged out
//      Then the driver uses the user buffer directly
// 
// NEITHER I\O:
//      No help from IO manger.
//

#define IOCTL_ANUBIS_READ_LOGS \
    CTL_CODE(FILE_DEVICE_ANUBIS, \
             0x800, \
             METHOD_OUT_DIRECT, \
             FILE_READ_ACCESS)

#define IOCTL_ANUBIS_GET_PROCCESS_JOB \
    CTL_CODE(FILE_DEVICE_ANUBIS, \
             0x800, \
             METHOD_OUT_DIRECT, \
             FILE_READ_ACCESS)

#define IOCTL_ANUBIS_SEND_PROCCESS_JOB_RESULT \
    CTL_CODE(FILE_DEVICE_ANUBIS, \
             0x800, \
             METHOD_OUT_DIRECT, \
             FILE_READ_ACCESS)