/*
 * Copyright 2000 Juergen Schmied
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __WINE_NTDLL_MISC_H
#define __WINE_NTDLL_MISC_H

#include "winnt.h"
#include "winternl.h"
#include "thread.h"

/* debug helper */
extern LPCSTR debugstr_us( const UNICODE_STRING *str );
extern void dump_ObjectAttributes (const OBJECT_ATTRIBUTES *ObjectAttributes);

/* module handling */
extern FARPROC MODULE_GetProcAddress( HMODULE hModule, LPCSTR function, int hint, BOOL snoop );

static inline HANDLE ntdll_get_process_heap(void)
{
    HANDLE *pdb = (HANDLE *)NtCurrentTeb()->process;
    return pdb[0x18 / sizeof(HANDLE)];  /* get dword at offset 0x18 in pdb */
}

#endif
