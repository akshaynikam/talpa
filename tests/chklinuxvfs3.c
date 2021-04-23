/*
 * TALPA test program
 *
 * Copyright (C) 2021 Sophos Limited, Oxford, England.
 *
 * This program is free software; you can redistribute it and/or modify it under the terms of the
 * GNU General Public License Version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without
 * even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with this program; if not,
 * write to the Free Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 *
 */

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "tlp-test.h"


int main(int argc, char *argv[])
{
    const char* sysctl;
    int offset;
    int fd;
    
    ssize_t len;
    char *buffer = "test";

    if (argc != 3)
    {
        return 77;
    }

    sysctl = argv[1];
    offset = atoi(argv[2]);
    
    fd = open(sysctl, O_RDONLY);
    if ( fd < 0 )
    {
        perror("Failed to open sysctl");
        return 77;
    }
    
    printf("Writing to '%s' at offset %d\n", sysctl, offset);
    len = pwrite(fd, buffer, 1, offset);
    
    if (len < 0)
    {
        perror("Write failed");
        return errno;
    }
    
    return 0;
}

