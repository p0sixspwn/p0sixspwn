/**
  * backup_file.c
  * Copyright (C) 2010 Joshua Hill
  * Copyright (C) 2012 Hanéne Samara
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

#include <openssl/sha.h>

#include "common.h"

static backup_file_t *backup_file_new()
{
    backup_file_t *file = (backup_file_t *) malloc(sizeof(backup_file_t));
    if (file == NULL) {
        ERROR("Allocation Error\n");
        return NULL;
    }
    memset(file, '\0', sizeof(backup_file_t));
    return file;
}

backup_file_t *backup_file_create(const char *filepath)
{
    backup_file_t *file = backup_file_new();
    if (filepath) {
        file->filepath = strdup(filepath);
    }
    file->mbdb_record = mbdb_record_create();
    mbdb_record_init(file->mbdb_record);
    return file;
}

backup_file_t *backup_file_create_with_data(unsigned char *data,
                                            unsigned int size, int copy)
{
    backup_file_t *file = backup_file_new();
    if (!file) {
        return NULL;
    }
    file->mbdb_record = mbdb_record_create();
    mbdb_record_init(file->mbdb_record);
    backup_file_assign_file_data(file, data, size, copy);
    return file;
}

backup_file_t *backup_file_create_from_record(mbdb_record_t * record)
{
    if (!record) {
        return NULL;
    }
    backup_file_t *file = backup_file_new();
    if (!file)
        return NULL;

    file->mbdb_record = (mbdb_record_t *) malloc(sizeof(mbdb_record_t));
    if (file->mbdb_record == NULL) {
        ERROR("Allocation Error\n");
        return NULL;
    }
    // we need to make a real copy of the record
    memcpy(file->mbdb_record, record, sizeof(mbdb_record_t));
    if (record->domain) {
        file->mbdb_record->domain = strdup(record->domain);
    }
    if (record->path) {
        file->mbdb_record->path = strdup(record->path);
    }
    if (record->target) {
        file->mbdb_record->target = strdup(record->target);
    }
    if (record->datahash) {
        file->mbdb_record->datahash = (char *)malloc(record->datahash_size);
        memcpy(file->mbdb_record->datahash, record->datahash,
               record->datahash_size);
    }
    if (record->unknown1) {
        file->mbdb_record->unknown1 = (char *)malloc(record->unknown1_size);
        memcpy(file->mbdb_record->unknown1, record->unknown1,
               record->unknown1_size);
    }
    if (record->property_count > 0) {
        file->mbdb_record->properties =
            (mbdb_record_property_t **) malloc(sizeof(mbdb_record_property_t *)
                                               * record->property_count);
        int i;
        for (i = 0; i < record->property_count; i++) {
            mbdb_record_property_t *prop =
                malloc(sizeof(mbdb_record_property_t));
            prop->name_size = record->properties[i]->name_size;
            prop->name = (char *)malloc(prop->name_size + 1);
            memcpy(prop->name, record->properties[i]->name, prop->name_size);
            prop->value_size = record->properties[i]->value_size;
            prop->value = (char *)malloc(prop->value_size + 1);
            memcpy(prop->value, record->properties[i]->value, prop->value_size);
        }
    }

    return file;
}

void backup_file_assign_file_data(backup_file_t * bfile, unsigned char *data,
                                  unsigned int size, int copy)
{
    if (copy) {
        bfile->data = malloc(size);
        memcpy(bfile->data, data, size);
        bfile->size = size;
        bfile->free_data = 1;
    } else {
        bfile->data = data;
        bfile->size = size;
        bfile->free_data = 0;
    }
    if (bfile->filepath) {
        free(bfile->filepath);
        bfile->filepath = NULL;
    }
}

void backup_file_assign_file_path(backup_file_t * bfile, unsigned char *path)
{
    if (bfile->data && bfile->free_data) {
        free(bfile->data);
        bfile->data = NULL;
        bfile->free_data = 0;
    }
    if (bfile->filepath) {
        free(bfile->filepath);
    }
    bfile->filepath = strdup(path);
}

void backup_file_free(backup_file_t * file)
{
    if (file) {
        if (file->mbdb_record) {
            mbdb_record_free(file->mbdb_record);
        }
        if (file->filepath) {
            free(file->filepath);
        }
        if (file->data && file->free_data) {
            free(file->data);
        }
        free(file);
    }
}

void backup_file_set_domain(backup_file_t * bfile, const char *domain)
{
    if (!bfile)
        return;
    mbdb_record_set_domain(bfile->mbdb_record, domain);
}

void backup_file_set_path(backup_file_t * bfile, const char *path)
{
    if (!bfile)
        return;
    mbdb_record_set_path(bfile->mbdb_record, path);
}

void backup_file_set_target(backup_file_t * bfile, const char *target)
{
    if (!bfile)
        return;
    mbdb_record_set_target(bfile->mbdb_record, target);
}

static void debug_hash(const unsigned char *hash, int len)
{
    int i;
    for (i = 0; i < len; i++) {
        printf("%02x", hash[i]);
    }
    printf("\n");
}

void backup_file_update_hash(backup_file_t * bfile)
{
    if (!bfile)
        return;
    if (bfile->filepath) {
        FILE *f = fopen(bfile->filepath, "rb");
        if (!f) {
            ERROR("%s: ERROR: Could not open file '%s'\n", __func__,
                  bfile->filepath);
        }
        unsigned char buf[8192];
        size_t bytes;
        unsigned char sha1[20] = { 0, };
        SHA_CTX shactx;
        SHA1_Init(&shactx);
        while (!feof(f)) {
            bytes = fread(buf, 1, sizeof(buf), f);
            if (bytes > 0) {
                SHA1_Update(&shactx, buf, bytes);
            }
        }
        SHA1_Final(sha1, &shactx);
        fclose(f);
        DEBUG("setting datahash to ");
        debug_hash(sha1, 20);
        mbdb_record_set_datahash(bfile->mbdb_record, sha1, 20);
    } else if (bfile->data) {
        unsigned char sha1[20] = { 0, };
        SHA1(bfile->data, bfile->size, sha1);
        DEBUG("setting datahash to ");
        debug_hash(sha1, 20);
        mbdb_record_set_datahash(bfile->mbdb_record, sha1, 20);
    } else {
        ERROR
            ("%s: ERROR: neither filename nor data given, setting hash to N/A\n",
             __func__);
        mbdb_record_set_datahash(bfile->mbdb_record, NULL, 0);
    }
}

void backup_file_disable_hash(backup_file_t * bfile)
{
    if (!bfile)
        return;
    mbdb_record_set_datahash(bfile->mbdb_record, NULL, 0);
}

void backup_file_set_mode(backup_file_t * bfile, unsigned short mode)
{
    if (!bfile)
        return;
    mbdb_record_set_mode(bfile->mbdb_record, mode);
}

void backup_file_set_inode(backup_file_t * bfile, unsigned int inode)
{
    if (!bfile)
        return;
    mbdb_record_set_inode(bfile->mbdb_record, inode);
}

void backup_file_set_uid(backup_file_t * bfile, unsigned int uid)
{
    if (!bfile)
        return;
    mbdb_record_set_uid(bfile->mbdb_record, uid);
}

void backup_file_set_gid(backup_file_t * bfile, unsigned int gid)
{
    if (!bfile)
        return;
    mbdb_record_set_gid(bfile->mbdb_record, gid);
}

void backup_file_set_time1(backup_file_t * bfile, unsigned int time1)
{
    if (!bfile)
        return;
    mbdb_record_set_time1(bfile->mbdb_record, time1);
}

void backup_file_set_time2(backup_file_t * bfile, unsigned int time2)
{
    if (!bfile)
        return;
    mbdb_record_set_time2(bfile->mbdb_record, time2);
}

void backup_file_set_time3(backup_file_t * bfile, unsigned int time3)
{
    if (!bfile)
        return;
    mbdb_record_set_time3(bfile->mbdb_record, time3);
}

void backup_file_set_length(backup_file_t * bfile, unsigned long long length)
{
    if (!bfile)
        return;
    mbdb_record_set_length(bfile->mbdb_record, length);
}

void backup_file_set_flag(backup_file_t * bfile, unsigned char flag)
{
    if (!bfile)
        return;
    mbdb_record_set_flag(bfile->mbdb_record, flag);
}

int backup_file_get_record_data(backup_file_t * bfile, unsigned char **data,
                                unsigned int *size)
{
    if (!bfile)
        return;
    if (!bfile->mbdb_record) {
        ERROR("%s: ERROR: no mbdb_record present\n", __func__);
        return -1;
    }

    if (mbdb_record_build(bfile->mbdb_record, data, size) < 0) {
        ERROR("%s: ERROR: could not build mbdb_record data\n", __func__);
        return -1;
    }

    return 0;
}
