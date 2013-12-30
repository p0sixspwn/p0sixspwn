/**
 * GreenPois0n
 * Copyright (C) 2010 Chronic-Dev Team
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

#ifndef _MBDB_H_
#define _MBDB_H_

struct mbdb_t;

struct mbdb_record_property_t {
    unsigned short name_size;
    char *name;
    unsigned short value_size;
    char *value;
} __attribute__ ((__packed__));

typedef struct mbdb_record_property_t mbdb_record_property_t;

struct mbdb_record_t {
    unsigned short domain_size;
    char *domain;
    unsigned short path_size;
    char *path;
    unsigned short target_size;
    char *target;               // absolute path
    unsigned short datahash_size;
    char *datahash;             // SHA1 hash
    unsigned short unknown1_size;
    char *unknown1;
    unsigned short mode;        // Axxx = symlink, 4xxx = dir, 8xxx = file
    unsigned int unknown2;
    unsigned int inode;
    unsigned int uid;
    unsigned int gid;
    unsigned int time1;
    unsigned int time2;
    unsigned int time3;
    unsigned long long length;  // 0 if link or dir
    unsigned char flag;         // 0 if link or dir
    unsigned char property_count;   // number of properties
    mbdb_record_property_t **properties;    // properties
    unsigned int this_size;     // size of this record in bytes
} __attribute__ ((__packed__));

typedef struct mbdb_record_t mbdb_record_t;

mbdb_record_t *mbdb_record_create();
mbdb_record_t *mbdb_record_parse(unsigned char *data);
void mbdb_record_debug(mbdb_record_t * record);
void mbdb_record_free(mbdb_record_t * record);

void mbdb_record_init(mbdb_record_t * record);
void mbdb_record_set_domain(mbdb_record_t * record, const char *domain);
void mbdb_record_set_path(mbdb_record_t * record, const char *path);
void mbdb_record_set_target(mbdb_record_t * record, const char *target);
void mbdb_record_set_datahash(mbdb_record_t * record, const char *hash,
                              unsigned short hash_size);
void mbdb_record_set_unknown1(mbdb_record_t * record, const char *data,
                              unsigned short size);
void mbdb_record_set_mode(mbdb_record_t * record, unsigned short mode);
void mbdb_record_set_unknown2(mbdb_record_t * record, unsigned int unknown2);
void mbdb_record_set_inode(mbdb_record_t * record, unsigned int inode);
void mbdb_record_set_uid(mbdb_record_t * record, unsigned int uid);
void mbdb_record_set_gid(mbdb_record_t * record, unsigned int gid);
void mbdb_record_set_time1(mbdb_record_t * record, unsigned int time1);
void mbdb_record_set_time2(mbdb_record_t * record, unsigned int time2);
void mbdb_record_set_time3(mbdb_record_t * record, unsigned int time3);
void mbdb_record_set_length(mbdb_record_t * record, unsigned long long length);
void mbdb_record_set_flag(mbdb_record_t * record, unsigned char flag);
// TODO sth like mbdb_record_add_property()

int mbdb_record_build(mbdb_record_t * record, unsigned char **data,
                      unsigned int *size);

#define MBDB_MAGIC "\x6d\x62\x64\x62\x05\x00"

typedef struct mbdb_header {
    unsigned char magic[6];     // 'mbdb\5\0'
} mbdb_header_t;

typedef struct mbdb_t {
    unsigned int size;
    unsigned char *data;
    mbdb_header_t *header;
    int num_records;
    mbdb_record_t **records;
} mbdb_t;

extern mbdb_t *apparition_mbdb;

mbdb_t *mbdb_create();
mbdb_t *mbdb_open(unsigned char *file);
mbdb_t *mbdb_parse(unsigned char *data, unsigned int size);
mbdb_record_t *mbdb_get_record(mbdb_t * mbdb, unsigned int offset);
void mbdb_free(mbdb_t * mbdb);

#endif
