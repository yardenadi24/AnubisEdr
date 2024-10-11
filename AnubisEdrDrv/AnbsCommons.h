#pragma once
#include <ntddk.h>

#define ANUBIS_PREFIX "Anubis: "
#define MAX_PROCESSES 100

// Some print macros
#define DbgInfo(s,...) DbgPrint(ANUBIS_PREFIX "<Info>" "::%s:: " s "\n", __FUNCTION__, __VA_ARGS__)
#define DbgError(s,...) DbgPrint(ANUBIS_PREFIX "<Error>" "::%s:: "  s "\n", __FUNCTION__, __VA_ARGS__)
#define DbgWarning(s,...) DbgPrint(ANUBIS_PREFIX "<Warning>" "::%s:: " s "\n", __FUNCTION__, __VA_ARGS__)

// Memory tags
// TODO: Move to a dedicated memory managing file
#define ANUBIS_MEM_TAG 'sbnA'