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

#ifndef _BACKUP_FILE_H_
#define _BACKUP_FILE_H_

typedef struct backup_file_t {
    mbdb_record_t *mbdb_record;
    char *filepath;
    unsigned char *data;
    unsigned int size;
    int free_data;
} backup_file_t;

backup_file_t *backup_file_create(const char *filepath);
backup_file_t *backup_file_create_with_data(unsigned char *data,
                                            unsigned int size, int copy);
backup_file_t *backup_file_create_from_record(mbdb_record_t * record);

void backup_file_assign_file_data(backup_file_t * bfile, unsigned char *data,
                                  unsigned int size, int copy);
void backup_file_assign_file_path(backup_file_t * bfile, unsigned char *path);

void backup_file_set_domain(backup_file_t * bfile, const char *domain);
void backup_file_set_path(backup_file_t * bfile, const char *path);
void backup_file_set_target(backup_file_t * bfile, const char *target);
void backup_file_update_hash(backup_file_t * bfile);
void backup_file_disable_hash(backup_file_t * bfile);
void backup_file_set_mode(backup_file_t * bfile, unsigned short mode);
void backup_file_set_inode(backup_file_t * bfile, unsigned int inode);
void backup_file_set_uid(backup_file_t * bfile, unsigned int uid);
void backup_file_set_gid(backup_file_t * bfile, unsigned int gid);
void backup_file_set_time1(backup_file_t * bfile, unsigned int time1);
void backup_file_set_time2(backup_file_t * bfile, unsigned int time2);
void backup_file_set_time3(backup_file_t * bfile, unsigned int time3);
void backup_file_set_length(backup_file_t * bfile, unsigned long long length);
void backup_file_set_flag(backup_file_t * bfile, unsigned char flag);

void backup_file_free(backup_file_t * file);

int backup_file_get_record_data(backup_file_t * bfile, unsigned char **data,
                                unsigned int *size);

#endif
