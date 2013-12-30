/**
 * afc.c
 * Copyright (C) 2010 Joshua Hill
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 **/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libimobiledevice/afc.h>

#include "common.h"

afc_t *afc_create()
{
    afc_t *afc = (afc_t *) malloc(sizeof(afc_t));
    if (afc != NULL) {
        memset(afc, '\0', sizeof(afc));
    }
    return afc;
}

afc_t *afc_connect(device_t * device)
{
    int err = 0;
    uint16_t port = 0;
    afc_t *afc = NULL;
    lockdown_t *lockdown = NULL;

    lockdown = lockdown_open(device);
    if (lockdown == NULL) {
        ERROR("Unable to open connection to lockdownd\n");
        return NULL;
    }

    err = lockdown_start_service(lockdown, "com.apple.afc", &port);
    if (err < 0) {
        ERROR("Unable to start AFC service\n");
        return NULL;
    }
    lockdown_close(lockdown);

    afc = afc_open(device, port);
    if (afc == NULL) {
        ERROR("Unable to open connection to AFC service\n");
        return NULL;
    }

    return afc;
}

afc_t *afc_open(device_t * device, uint16_t port)
{
    afc_error_t err = AFC_E_SUCCESS;
    afc_t *afc = afc_create();
    if (afc != NULL) {
        lockdownd_service_descriptor_t desc =
            malloc(sizeof(struct lockdownd_service_descriptor));

        if (!desc)
            return NULL;
        memset(desc, '\0', sizeof(struct lockdownd_service_descriptor));

        desc->port = port;

        DEBUG("Service %p\n", desc);

        err = afc_client_new(device->client, desc, &(afc->client));
        if (err != AFC_E_SUCCESS) {
            WARN("Unable to create new AFC client\n");
            afc_free(afc);
            return NULL;
        }
        afc->device = device;
        afc->port = port;
    }
    return afc;
}

int afc_close(afc_t * afc)
{
    //TODO: Implement Me
    return -1;
}

void afc_free(afc_t * afc)
{
    if (afc) {
        free(afc);
    }
}

void afc_free_dictionary(char **dictionary) //ghetto i know, not sure where/how to put a global function for this
{
    int i = 0;

    if (!dictionary)
        return;

    for (i = 0; dictionary[i]; i++) {
        free(dictionary[i]);
    }
    free(dictionary);
}

void apparition_afc_get_file_contents(afc_t * afc, const char *filename,
                                      char **data, uint64_t * size)
{
    if (!afc || !data || !size) {
        return;
    }

    char **fileinfo = NULL;
    uint32_t fsize = 0;

    afc_get_file_info(afc->client, filename, &fileinfo);
    if (!fileinfo) {
        return;
    }
    int i;
    for (i = 0; fileinfo[i]; i += 2) {
        if (!strcmp(fileinfo[i], "st_size")) {
            fsize = atol(fileinfo[i + 1]);
            break;
        }
    }
    afc_free_dictionary(fileinfo);

    if (fsize == 0) {
        return;
    }

    uint64_t f = 0;
    afc_file_open(afc->client, filename, AFC_FOPEN_RDONLY, &f);
    if (!f) {
        return;
    }
    char *buf = (char *)malloc((uint32_t) fsize);
    uint32_t done = 0;
    while (done < fsize) {
        uint32_t bread = 0;
        afc_file_read(afc->client, f, buf + done, 65536, &bread);
        if (bread > 0) {

        } else {
            break;
        }
        done += bread;
    }
    if (done == fsize) {
        *size = fsize;
        *data = buf;
    } else {
        free(buf);
    }
    afc_file_close(afc->client, f);
}

int afc_send_file(afc_t * afc, const char *local, const char *remote)
{

    uint64_t lockfile = 0;
    uint64_t my_file = 0;
    unsigned int bytes = 0;

    afc_file_open(afc->client, remote, AFC_FOPEN_WR, &my_file);
    if (my_file) {
        //char *outdatafile = strdup("THIS IS HOW WE DO IT, WHORE\n");

        //FIXME: right here its just sending "local/file.txt, rather than the contents of the file.

        //afc_file_write(afc->client, my_file, outdatafile, strlen(outdatafile), &bytes);// <-- old code
        afc_file_write(afc->client, my_file, local, strlen(local), &bytes);
        //free(outdatafile);
        if (bytes > 0)
            printf("File transferred successfully\n");
        else
            printf("File write failed!!! :(\n");
        afc_file_close(afc->client, my_file);
    }

    printf("afc all done.\n");

    return 0;
}
