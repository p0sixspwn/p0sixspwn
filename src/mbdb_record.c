/**
  * mbdb_record.c
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

#include "common.h"

mbdb_record_t *mbdb_record_create()
{
    mbdb_record_t *record = (mbdb_record_t *) malloc(sizeof(mbdb_record_t));
    if (record == NULL) {
        ERROR("Allocation Error!\n");
        return NULL;
    }
    memset(record, '\0', sizeof(mbdb_record_t));

    return record;
}

mbdb_record_t *mbdb_record_parse(unsigned char *data)
{
    unsigned int offset = 0;
    mbdb_record_t *record = mbdb_record_create();
    if (record == NULL) {
        ERROR("Unable to parse mbdb record\n");
        return NULL;
    }
    // Parse Domain
    unsigned short strsize = be16toh(*((unsigned short *)&data[offset]));
    if (strsize > 0 && strsize < 0xFFFF) {
        record->domain = (char *)malloc(strsize + 1);
        if (record->domain == NULL) {
            ERROR("Allocation Error!\n");
            return NULL;
        }
        offset += 2;
        memcpy(record->domain, &data[offset], strsize);
        record->domain[strsize] = 0;
        offset += strsize;
    } else {
        record->domain = NULL;
        offset += 2;
    }
    record->domain_size = strsize;

    // Parse Path
    strsize = be16toh(*((unsigned short *)&data[offset]));
    if (strsize > 0 && strsize < 0xFFFF) {
        record->path = (char *)malloc(strsize + 1);
        if (record->path == NULL) {
            ERROR("Allocation Error!\n");
            return NULL;
        }
        offset += 2;
        memcpy(record->path, &data[offset], strsize);
        record->path[strsize] = 0;
        offset += strsize;
    } else {
        record->path = NULL;
        offset += 2;
    }
    record->path_size = strsize;

    // Parse Target
    strsize = be16toh(*((unsigned short *)&data[offset]));
    if (strsize > 0 && strsize < 0xFFFF) {
        record->target = (char *)malloc(strsize + 1);
        if (record->target == NULL) {
            ERROR("Allocation Error!\n");
            return NULL;
        }
        offset += 2;
        memcpy(record->target, &data[offset], strsize);
        record->target[strsize] = 0;
        offset += strsize;
    } else {
        record->target = NULL;
        offset += 2;
    }
    record->target_size = strsize;

    // parse DataHash
    strsize = be16toh(*((unsigned short *)&data[offset]));
    if (strsize > 0 && strsize < 0xFFFF) {
        record->datahash = (char *)malloc(strsize);
        if (record->datahash == NULL) {
            ERROR("Allocation Error!\n");
            return NULL;
        }
        offset += 2;
        memcpy(record->datahash, &data[offset], strsize);
        offset += strsize;
    } else {
        record->datahash = NULL;
        offset += 2;
    }
    record->datahash_size = strsize;

    // parse unknown1
    strsize = be16toh(*((unsigned short *)&data[offset]));
    if (strsize > 0 && strsize < 0xFFFF) {
        record->unknown1 = (char *)malloc(strsize + 1);
        if (record->unknown1 == NULL) {
            ERROR("Allocation Error!\n");
            return NULL;
        }
        offset += 2;
        memcpy(record->unknown1, &data[offset], strsize);
        record->unknown1[strsize] = 0;
        offset += strsize;
    } else {
        record->unknown1 = NULL;
        offset += 2;
    }
    record->unknown1_size = strsize;

    record->mode = be16toh(*((unsigned short *)&data[offset]));
    offset += 2;

    record->unknown2 = be32toh(*((unsigned int *)&data[offset]));
    offset += 4;

    record->inode = be32toh(*((unsigned int *)&data[offset]));
    offset += 4;

    record->uid = be32toh(*((unsigned int *)&data[offset]));
    offset += 4;

    record->gid = be32toh(*((unsigned int *)&data[offset]));
    offset += 4;

    record->time1 = be32toh(*((unsigned int *)&data[offset]));
    offset += 4;

    record->time2 = be32toh(*((unsigned int *)&data[offset]));
    offset += 4;

    record->time3 = be32toh(*((unsigned int *)&data[offset]));
    offset += 4;

    record->length = be64toh(*((unsigned long long *)&data[offset]));
    offset += 8;

    record->flag = *((unsigned char *)&data[offset]);
    offset += 1;

    record->property_count = *((unsigned char *)&data[offset]);
    offset += 1;

    if (record->property_count > 0) {
        record->properties =
            (mbdb_record_property_t **) malloc(sizeof(mbdb_record_property_t *)
                                               * record->property_count);
        int i;
        for (i = 0; i < record->property_count; i++) {
            mbdb_record_property_t *prop =
                malloc(sizeof(mbdb_record_property_t));
            prop->name_size = be16toh(*((unsigned short *)&data[offset]));
            prop->name = (char *)malloc(prop->name_size + 1);
            offset += 2;
            memcpy(prop->name, &data[offset], prop->name_size);
            prop->name[prop->name_size] = 0;
            offset += prop->name_size;

            prop->value_size = be16toh(*((unsigned short *)&data[offset]));
            prop->value = (char *)malloc(prop->value_size + 1);
            offset += 2;
            memcpy(prop->value, &data[offset], prop->value_size);
            prop->value[prop->value_size] = 0;
            offset += prop->value_size;

            record->properties[i] = prop;
        }
    }
    record->this_size = offset;

    //mbdb_record_debug(record);

    return record;
}

/*
 struct mbdb_record_t {
 char* domain;
 char* path;
 char* target;	                  // absolute path
 char* datahash;	                  // SHA1 hash
 char* unknown1;
 unsigned short mode;	          // Axxx = symlink, 4xxx = dir, 8xxx = file
 unsigned int unknown2;
 unsigned int inode;
 unsigned int uid;
 unsigned int gid;
 unsigned int time1;
 unsigned int time2;
 unsigned int time3;
 unsigned long long length;	      // 0 if link or dir
 unsigned char flag;	              // 0 if link or dir
 unsigned char properties;	      // number of properties
 } __attribute__((__packed__));
 */

void mbdb_record_free(mbdb_record_t * record)
{
    if (record) {
        if (record->domain) {
            free(record->domain);
        }
        if (record->path) {
            free(record->path);
        }
        if (record->target) {
            free(record->target);
        }
        if (record->datahash) {
            free(record->datahash);
        }
        if (record->unknown1) {
            free(record->unknown1);
        }
        if (record->property_count > 0) {
            int i;
            for (i = 0; i < record->property_count; i++) {
                if (record->properties[i]->name) {
                    free(record->properties[i]->name);
                }
                if (record->properties[i]->value) {
                    free(record->properties[i]->value);
                }
                free(record->properties[i]);
            }
            free(record->properties);
        }
        free(record);
    }
}

void mbdb_record_debug(mbdb_record_t * record)
{
    DEBUG("mbdb record\n");
    DEBUG("\tdomain = %s\n", record->domain);
    DEBUG("\tpath = %s\n", record->path);
    DEBUG("\ttarget = %s\n", record->target);
    DEBUG("\tdatahash = %p\n", record->datahash);
    DEBUG("\tunknown1 = %s\n", record->unknown1);
    DEBUG("\tmode = 0%o (0x%x)\n", record->mode, record->mode);
    DEBUG("\tunknown2 = 0x%x\n", record->unknown2);
    DEBUG("\tinode = 0x%x\n", record->inode);
    DEBUG("\tuid = %d\n", record->uid);
    DEBUG("\tgid = %d\n", record->gid);
    DEBUG("\ttime1 = 0x%x\n", record->time1);
    DEBUG("\ttime2 = 0x%x\n", record->time2);
    DEBUG("\ttime3 = 0x%x\n", record->time3);
    DEBUG("\tlength = %llu\n", record->length);
    DEBUG("\tflag = 0x%x\n", record->flag);
    DEBUG("\tproperty_count = %d\n", record->property_count);
}

void mbdb_record_init(mbdb_record_t * record)
{
    if (!record) {
        return;
    }
    memset(record, '\0', sizeof(mbdb_record_t));
    record->target_size = 0xFFFF;
    record->datahash_size = 0xFFFF;
    record->unknown1_size = 0xFFFF;
    record->this_size =
        2 + 2 + 2 + 2 + 2 + 2 + 4 + 4 + 4 + 4 + 4 + 4 + 4 + 8 + 1 + 1;
}

void mbdb_record_set_domain(mbdb_record_t * record, const char *domain)
{
    if (!record)
        return;
    unsigned short old_size = record->domain_size;
    if (record->domain) {
        free(record->domain);
        record->domain = NULL;
    }
    if (record->domain_size > 0 && record->domain_size < 0xFFFF) {
        record->this_size -= record->domain_size;
    }
    if (domain && (strlen(domain) > 0)) {
        record->domain_size = strlen(domain);
        record->domain = strdup(domain);
        record->this_size += record->domain_size;
    } else {
        record->domain_size = 0;
    }
}

void mbdb_record_set_path(mbdb_record_t * record, const char *path)
{
    if (!record)
        return;
    unsigned short old_size = record->path_size;
    if (record->path) {
        free(record->path);
        record->path = NULL;
    }
    if (record->path_size > 0 && record->path_size < 0xFFFF) {
        record->this_size -= record->path_size;
    }
    if (path && (strlen(path) > 0)) {
        record->path_size = strlen(path);
        record->path = strdup(path);
        record->this_size += record->path_size;
    } else {
        record->path_size = 0;
    }
}

void mbdb_record_set_target(mbdb_record_t * record, const char *target)
{
    if (!record)
        return;
    unsigned short old_size = record->target_size;
    if (record->target) {
        free(record->target);
        record->target = NULL;
    }
    if (record->target_size > 0 && record->target_size < 0xFFFF) {
        record->this_size -= record->target_size;
    }
    if (target && (strlen(target) > 0)) {
        record->target_size = strlen(target);
        record->target = strdup(target);
        record->this_size += record->target_size;
    } else {
        record->target_size = 0xFFFF;
    }
}

void mbdb_record_set_datahash(mbdb_record_t * record, const char *hash,
                              unsigned short hash_size)
{
    if (!record)
        return;
    unsigned short old_size = record->datahash_size;
    if (record->datahash) {
        free(record->datahash);
        record->datahash = NULL;
    }
    if (record->datahash_size > 0 && record->datahash_size < 0xFFFF) {
        record->this_size -= record->datahash_size;
    }
    if (hash && (hash_size > 0)) {
        record->datahash_size = hash_size;
        record->datahash = (char *)malloc(hash_size);
        memcpy(record->datahash, hash, hash_size);
        record->this_size += record->datahash_size;
    } else {
        record->datahash_size = 0xFFFF;
    }
}

void mbdb_record_set_unknown1(mbdb_record_t * record, const char *data,
                              unsigned short size)
{
    if (!record)
        return;
    unsigned short old_size = record->unknown1_size;
    if (record->unknown1) {
        free(record->unknown1);
        record->unknown1 = NULL;
    }
    if (record->unknown1_size > 0 && record->unknown1_size < 0xFFFF) {
        record->this_size -= record->unknown1_size;
    }
    if (data && (size > 0)) {
        record->unknown1_size = size;
        record->unknown1 = (char *)malloc(size);
        memcpy(record->unknown1, data, size);
        record->this_size += record->unknown1_size;
    } else {
        record->unknown1_size = 0xFFFF;
    }
}

void mbdb_record_set_mode(mbdb_record_t * record, unsigned short mode)
{
    if (!record)
        return;
    record->mode = mode;
}

void mbdb_record_set_unknown2(mbdb_record_t * record, unsigned int unknown2)
{
    if (!record)
        return;
    record->unknown2 = unknown2;
}

void mbdb_record_set_inode(mbdb_record_t * record, unsigned int inode)
{
    if (!record)
        return;
    record->inode = inode;
}

void mbdb_record_set_uid(mbdb_record_t * record, unsigned int uid)
{
    if (!record)
        return;
    record->uid = uid;
}

void mbdb_record_set_gid(mbdb_record_t * record, unsigned int gid)
{
    if (!record)
        return;
    record->gid = gid;
}

void mbdb_record_set_time1(mbdb_record_t * record, unsigned int time1)
{
    if (!record)
        return;
    record->time1 = time1;
}

void mbdb_record_set_time2(mbdb_record_t * record, unsigned int time2)
{
    if (!record)
        return;
    record->time2 = time2;
}

void mbdb_record_set_time3(mbdb_record_t * record, unsigned int time3)
{
    if (!record)
        return;
    record->time3 = time3;
}

void mbdb_record_set_length(mbdb_record_t * record, unsigned long long length)
{
    if (!record)
        return;
    record->length = length;
}

void mbdb_record_set_flag(mbdb_record_t * record, unsigned char flag)
{
    if (!record)
        return;
    record->flag = flag;
}

int mbdb_record_build(mbdb_record_t * record, unsigned char **data,
                      unsigned int *size)
{
    unsigned int offset = 0;
    unsigned char *data_buf = NULL;

    if (!record) {
        return -1;
    }

    data_buf = (unsigned char *)malloc(record->this_size);
    if (!data_buf) {
        ERROR("Allocation Error!\n");
        return -1;
    }

    unsigned short strsize;

    // append Domain
    strsize = htobe16(record->domain_size);
    memcpy(&data_buf[offset], &strsize, 2);
    offset += 2;
    if (record->domain != NULL) {
        memcpy(&data_buf[offset], record->domain, record->domain_size);
        offset += record->domain_size;
    }
    // append Path
    strsize = htobe16(record->path_size);
    memcpy(&data_buf[offset], &strsize, 2);
    offset += 2;
    if (record->path != NULL) {
        memcpy(&data_buf[offset], record->path, record->path_size);
        offset += record->path_size;
    }
    // append Target
    strsize = htobe16(record->target_size);
    memcpy(&data_buf[offset], &strsize, 2);
    offset += 2;
    if (record->target != NULL) {
        memcpy(&data_buf[offset], record->target, record->target_size);
        offset += record->target_size;
    }
    // append DataHash
    strsize = htobe16(record->datahash_size);
    memcpy(&data_buf[offset], &strsize, 2);
    offset += 2;
    if (record->datahash != NULL) {
        memcpy(&data_buf[offset], record->datahash, record->datahash_size);
        offset += record->datahash_size;
    }
    // append unknown1
    strsize = htobe16(record->unknown1_size);
    memcpy(&data_buf[offset], &strsize, 2);
    offset += 2;
    if (record->unknown1 != NULL) {
        memcpy(&data_buf[offset], record->unknown1, record->unknown1_size);
        offset += record->unknown1_size;
    }

    unsigned short mode = htobe16(record->mode);
    memcpy(&data_buf[offset], &mode, 2);
    offset += 2;

    int unknown2 = htobe32(record->unknown2);
    memcpy(&data_buf[offset], &unknown2, 4);
    offset += 4;

    int inode = htobe32(record->inode);
    memcpy(&data_buf[offset], &inode, 4);
    offset += 4;

    int uid = htobe32(record->uid);
    memcpy(&data_buf[offset], &uid, 4);
    offset += 4;

    int gid = htobe32(record->gid);
    memcpy(&data_buf[offset], &gid, 4);
    offset += 4;

    int time1 = htobe32(record->time1);
    memcpy(&data_buf[offset], &time1, 4);
    offset += 4;

    int time2 = htobe32(record->time2);
    memcpy(&data_buf[offset], &time2, 4);
    offset += 4;

    int time3 = htobe32(record->time3);
    memcpy(&data_buf[offset], &time3, 4);
    offset += 4;

    unsigned long long length = htobe64(record->length);
    memcpy(&data_buf[offset], &length, 8);
    offset += 8;

    unsigned char flag = record->flag;
    memcpy(&data_buf[offset], &flag, 1);
    offset++;

    unsigned char prop = record->property_count;
    memcpy(&data_buf[offset], &prop, 1);
    offset++;

    // add properties
    int i;
    for (i = 0; i < (int)prop; i++) {
        mbdb_record_property_t *property = record->properties[i];

        unsigned short pnsize = htobe16(property->name_size);
        memcpy(&data_buf[offset], &pnsize, 2);
        offset += 2;
        memcpy(&data_buf[offset], property->name, property->name_size);
        offset += property->name_size;

        unsigned short pvsize = htobe16(property->value_size);
        memcpy(&data_buf[offset], &pvsize, 2);
        offset += 2;
        memcpy(&data_buf[offset], property->value, property->value_size);
        offset += property->value_size;
    }

    if (record->this_size != offset) {
        *data = NULL;
        *size = 0;
        ERROR
            ("%s: ERROR: inconsistent record size (present %d != created %d)\n",
             __func__, record->this_size, offset);
        return -1;
    }

    *data = data_buf;
    *size = offset;

    return 0;
}
