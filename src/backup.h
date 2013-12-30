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

#ifndef _BACKUP_H_
#define _BACKUP_H_

#include "mbdb.h"
#include "backup_file.h"

typedef struct backup_t {
    char *path;
    mbdb_t *mbdb;
} backup_t;

backup_t *backup_open(const char *directory, const char *uuid);
int backup_get_file_index(backup_t * backup, const char *domain,
                          const char *path);
char *backup_get_file_path(backup_t * backup, backup_file_t * bfile);
backup_file_t *backup_get_file(backup_t * backup, const char *domain,
                               const char *path);
int backup_update_file(backup_t * backup, backup_file_t * bfile);
int backup_remove_file(backup_t * backup, backup_file_t * bfile);
void backup_free(backup_t * backup);
int backup_mkdir(backup_t * backup, char *domain, char *path, int mode, int uid,
                 int gid, int flag);
int backup_symlink(backup_t * backup, char *domain, char *path, char *to,
                   int uid, int gid, int flag);
int backup_add_file_from_path(backup_t * backup, char *domain, char *localpath,
                              char *path, int mode, int uid, int gid, int flag);
int backup_add_file_from_data(backup_t * backup, char *domain, char *data,
                              unsigned int size, char *path, int mode, int uid,
                              int gid, int flag);

#endif
