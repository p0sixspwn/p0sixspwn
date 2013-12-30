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

backup_t *backup_open(const char *backupdir, const char *uuid)
{
    if (!backupdir || !uuid) {
        return NULL;
    }

    char *backup_path =
        (char *)malloc(strlen(backupdir) + 1 + strlen(uuid) + 1 + 4);
    strcpy(backup_path, backupdir);
    strcat(backup_path, "/");
    strcat(backup_path, uuid);

    char *mbdb_path =
        (char *)malloc(strlen(backup_path) + 1 + strlen("Manifest.mbdb") + 1 +
                       4);
    strcpy(mbdb_path, backup_path);
    strcat(mbdb_path, "/");
    strcat(mbdb_path, "Manifest.mbdb");

    mbdb_t *mbdb = mbdb_open(mbdb_path);
    if (mbdb) {
        DEBUG("Manifest.mbdb opened, %d records\n", mbdb->num_records);
    } else {
        ERROR("ERROR: could not open %s\n", mbdb_path);
        free(mbdb_path);
        return NULL;
    }
    free(mbdb_path);

    backup_t *backup = (backup_t *) malloc(sizeof(backup_t));
    if (backup == NULL) {
        free(mbdb);
        return NULL;
    }
    memset(backup, '\0', sizeof(backup_t));

    backup->mbdb = mbdb;
    backup->path = backup_path;

    return backup;
}

int backup_get_file_index(backup_t * backup, const char *domain,
                          const char *path)
{
    if (!backup || !backup->mbdb) {
        return -1;
    }
    int i = 0;
    int found = 0;
    mbdb_record_t *rec = NULL;
    for (i = 0; i < backup->mbdb->num_records; i++) {
        rec = backup->mbdb->records[i];
        if (rec->domain && !strcmp(rec->domain, domain) && rec->path
            && !strcmp(rec->path, path)) {
            found = 1;
            break;
        }
    }
    return (found) ? i : -1;
}

backup_file_t *backup_get_file(backup_t * backup, const char *domain,
                               const char *path)
{
    if (!backup || !backup->mbdb) {
        return NULL;
    }
    int idx = backup_get_file_index(backup, domain, path);
    if (idx < 0) {
        // not found
        return NULL;
    }
    mbdb_record_t *rec = backup->mbdb->records[idx];
    return backup_file_create_from_record(rec);
}

char *backup_get_file_path(backup_t * backup, backup_file_t * bfile)
{
    int res = 0;

    if (!backup || !bfile) {
        return NULL;
    }
    if (!backup->mbdb) {
        ERROR("%s: ERROR: no mbdb in given backup_t\n", __func__);
        return NULL;
    }

    char *bfntmp =
        (char *)malloc(bfile->mbdb_record->domain_size + 1 +
                       bfile->mbdb_record->path_size + 1 + 4);
    strcpy(bfntmp, bfile->mbdb_record->domain);
    strcat(bfntmp, "-");
    strcat(bfntmp, bfile->mbdb_record->path);

    char *backupfname = (char *)malloc(strlen(backup->path) + 1 + 40 + 1);
    unsigned char sha1[20] = { 0, };
    SHA1(bfntmp, strlen(bfntmp), sha1);
    free(bfntmp);

    strcpy(backupfname, backup->path);
    strcat(backupfname, "/");

    int i;
    char *p = backupfname + strlen(backup->path) + 1;
    for (i = 0; i < 20; i++) {
        sprintf(p + i * 2, "%02x", sha1[i]);
    }

    DEBUG("backup filename is %s\n", backupfname);

    return backupfname;
}

int backup_update_file(backup_t * backup, backup_file_t * bfile)
{
    int res = 0;

    if (!backup || !bfile) {
        return -1;
    }
    if (!backup->mbdb) {
        ERROR("%s: ERROR: no mbdb in given backup_t\n", __func__);
        return -1;
    }

    unsigned char *rec = NULL;
    unsigned int rec_size = 0;

    if (backup_file_get_record_data(bfile, &rec, &rec_size) < 0) {
        ERROR("%s: ERROR: could not build mbdb_record data\n", __func__);
        return -1;
    }

    unsigned int newsize = 0;
    unsigned char *newdata = NULL;

    // find record
    int idx =
        backup_get_file_index(backup, bfile->mbdb_record->domain,
                              bfile->mbdb_record->path);
    if (idx < 0) {
        // append record to mbdb
        newsize = backup->mbdb->size + rec_size;
        newdata = (unsigned char *)malloc(newsize);

        memcpy(newdata, backup->mbdb->data, backup->mbdb->size);
        memcpy(newdata + backup->mbdb->size, rec, rec_size);
    } else {
        // update record in mbdb
        backup_file_t *oldfile =
            backup_file_create_from_record(backup->mbdb->records[idx]);
        unsigned int oldsize = oldfile->mbdb_record->this_size;
        backup_file_free(oldfile);

        newsize = backup->mbdb->size - oldsize + rec_size;
        newdata = (unsigned char *)malloc(newsize);

        char *p = newdata;
        memcpy(p, backup->mbdb->data, sizeof(mbdb_header_t));
        p += sizeof(mbdb_header_t);

        mbdb_record_t *r;
        unsigned char *rd;
        unsigned int rs;
        int i;

        for (i = 0; i < idx; i++) {
            r = backup->mbdb->records[i];
            rd = NULL;
            rs = 0;
            mbdb_record_build(r, &rd, &rs);
            memcpy(p, rd, rs);
            free(rd);
            p += rs;
        }
        memcpy(p, rec, rec_size);
        p += rec_size;
        for (i = idx + 1; i < backup->mbdb->num_records; i++) {
            r = backup->mbdb->records[i];
            rd = NULL;
            rs = 0;
            mbdb_record_build(r, &rd, &rs);
            memcpy(p, rd, rs);
            free(rd);
            p += rs;
        }
    }

    if (!newdata) {
        ERROR("Uh, could not re-create mbdb data?!\n");
        return -1;
    }

    mbdb_free(backup->mbdb);
    free(rec);

    // parse the new data
    backup->mbdb = mbdb_parse(newdata, newsize);
    free(newdata);

    // write out the file data
    char *bfntmp =
        (char *)malloc(bfile->mbdb_record->domain_size + 1 +
                       bfile->mbdb_record->path_size + 1 + 4);
    strcpy(bfntmp, bfile->mbdb_record->domain);
    strcat(bfntmp, "-");
    strcat(bfntmp, bfile->mbdb_record->path);

    char *backupfname = (char *)malloc(strlen(backup->path) + 1 + 40 + 1);
    unsigned char sha1[20] = { 0, };
    SHA1(bfntmp, strlen(bfntmp), sha1);
    free(bfntmp);

    strcpy(backupfname, backup->path);
    strcat(backupfname, "/");

    int i;
    char *p = backupfname + strlen(backup->path) + 1;
    for (i = 0; i < 20; i++) {
        sprintf(p + i * 2, "%02x", sha1[i]);
    }

    DEBUG("backup filename is %s\n", backupfname);

    if (bfile->filepath) {
        // copy file to backup dir
        if (file_copy(bfile->filepath, backupfname) < 0) {
            ERROR("%s: ERROR: could not copy file '%s' to '%s'\n", __func__,
                  bfile->filepath, backupfname);
            res = -1;
        }
    } else if (bfile->data) {
        // write data buffer to file
        if (file_write(backupfname, bfile->data, bfile->size) < 0) {
            ERROR("%s: ERROR: could not write to '%s'\n", __func__,
                  backupfname);
            res = -1;
        }
    } else if ((bfile->mbdb_record->mode) & 040000) {
        // directory!
    } else {
        DEBUG("%s: WARNING: file data not updated, no filename or data given\n",
              __func__);
    }

    free(backupfname);

    return res;
}

int backup_remove_file(backup_t * backup, backup_file_t * bfile)
{
    int res = 0;

    if (!backup || !bfile) {
        return -1;
    }
    if (!backup->mbdb) {
        ERROR("%s: ERROR: no mbdb in given backup_t\n", __func__);
        return -1;
    }

    unsigned int newsize = 0;
    unsigned char *newdata = NULL;

    // find record
    int idx =
        backup_get_file_index(backup, bfile->mbdb_record->domain,
                              bfile->mbdb_record->path);
    if (idx < 0) {
        DEBUG("file %s-%s not found in backup so not removed.\n",
              bfile->mbdb_record->domain, bfile->mbdb_record->path);
        return -1;
    } else {
        // remove record from mbdb
        backup_file_t *oldfile =
            backup_file_create_from_record(backup->mbdb->records[idx]);
        unsigned int oldsize = oldfile->mbdb_record->this_size;
        backup_file_free(oldfile);

        newsize = backup->mbdb->size - oldsize;
        newdata = (unsigned char *)malloc(newsize);

        char *p = newdata;
        memcpy(p, backup->mbdb->data, sizeof(mbdb_header_t));
        p += sizeof(mbdb_header_t);

        mbdb_record_t *r;
        unsigned char *rd;
        unsigned int rs;
        int i;

        for (i = 0; i < idx; i++) {
            r = backup->mbdb->records[i];
            rd = NULL;
            rs = 0;
            mbdb_record_build(r, &rd, &rs);
            memcpy(p, rd, rs);
            free(rd);
            p += rs;
        }
        for (i = idx + 1; i < backup->mbdb->num_records; i++) {
            r = backup->mbdb->records[i];
            rd = NULL;
            rs = 0;
            mbdb_record_build(r, &rd, &rs);
            memcpy(p, rd, rs);
            free(rd);
            p += rs;
        }
    }

    if (!newdata) {
        ERROR("Uh, could not re-create mbdb data?!\n");
        return -1;
    }

    mbdb_free(backup->mbdb);

    // parse the new data
    backup->mbdb = mbdb_parse(newdata, newsize);
    free(newdata);

    // write out the file data
    char *bfntmp =
        (char *)malloc(bfile->mbdb_record->domain_size + 1 +
                       bfile->mbdb_record->path_size + 1 + 4);
    strcpy(bfntmp, bfile->mbdb_record->domain);
    strcat(bfntmp, "-");
    strcat(bfntmp, bfile->mbdb_record->path);

    char *backupfname = (char *)malloc(strlen(backup->path) + 1 + 40 + 1);
    unsigned char sha1[20] = { 0, };
    SHA1(bfntmp, strlen(bfntmp), sha1);
    free(bfntmp);

    strcpy(backupfname, backup->path);
    strcat(backupfname, "/");

    int i;
    char *p = backupfname + strlen(backup->path) + 1;
    for (i = 0; i < 20; i++) {
        sprintf(p + i * 2, "%02x", sha1[i]);
    }

    if (!(bfile->mbdb_record->mode & 040000)) {
        DEBUG("deleting file %s\n", backupfname);
        remove(backupfname);
    }

    free(backupfname);

    return res;
}

int backup_write_mbdb(backup_t * backup)
{
    if (!backup || !backup->path || !backup->mbdb) {
        return -1;
    }

    char *mbdb_path =
        (char *)malloc(strlen(backup->path) + 1 + strlen("Manifest.mbdb") + 1);
    strcpy(mbdb_path, backup->path);
    strcat(mbdb_path, "/");
    strcat(mbdb_path, "Manifest.mbdb");

    int res = file_write(mbdb_path, backup->mbdb->data, backup->mbdb->size);
    free(mbdb_path);
    return res;
}

void backup_free(backup_t * backup)
{
    if (backup) {
        if (backup->mbdb) {
            mbdb_free(backup->mbdb);
        }
        if (backup->path) {
            free(backup->path);
        }
        free(backup);
    }
}

int inode_start = 54327;        /* Whatever. */

int backup_mkdir(backup_t * backup, char *domain, char *path, int mode, int uid,
                 int gid, int flag)
{
    int ret = -1;
    backup_file_t *file = backup_file_create(NULL);

    DEBUG("[backup] MKDIR: (%s):%s\n", domain, path);

    if (file) {
        backup_file_set_domain(file, domain);
        backup_file_set_path(file, path);
        backup_file_set_mode(file, mode | 040000);
        inode_start++;

        backup_file_set_inode(file, inode_start);
        backup_file_set_uid(file, uid);
        backup_file_set_gid(file, gid);
        backup_file_set_time1(file, time(NULL));
        backup_file_set_time2(file, time(NULL));
        backup_file_set_time3(file, time(NULL));
        backup_file_set_flag(file, flag);

        if (backup_update_file(backup, file) >= 0)
            ret = 0;
        else
            ret = -1;
        backup_file_free(file);

        if (!ret)
            backup_write_mbdb(backup);
    }
    return ret;
}

int backup_symlink(backup_t * backup, char *domain, char *path, char *to,
                   int uid, int gid, int flag)
{
    int ret = -1;
    backup_file_t *file = backup_file_create(NULL);

    DEBUG("[backup] SYMLINK: (%s):%s => %s\n", domain, path, to);

    if (file) {
        backup_file_set_domain(file, domain);
        backup_file_set_path(file, path);
        backup_file_set_target(file, to);
        backup_file_set_mode(file, 0120644);
        inode_start++;

        backup_file_set_inode(file, inode_start);
        backup_file_set_uid(file, uid);
        backup_file_set_gid(file, gid);
        backup_file_set_time1(file, time(NULL));
        backup_file_set_time2(file, time(NULL));
        backup_file_set_time3(file, time(NULL));
        backup_file_set_flag(file, flag);

        if (backup_update_file(backup, file) >= 0)
            ret = 0;
        else
            ret = -1;
        backup_file_free(file);

        if (!ret)
            backup_write_mbdb(backup);
    }
    return ret;
}

int backup_add_file_from_path(backup_t * backup, char *domain, char *localpath,
                              char *path, int mode, int uid, int gid, int flag)
{
    int ret = -1;
    unsigned int size = 0;
    unsigned char *data = NULL;
    struct stat buf;

    if (stat(localpath, &buf) == -1)
        ERROR("Could not open %s\n", localpath);

    file_read(localpath, &data, &size);

    DEBUG("[backup] FILE: (%s):%s\n", domain, path);

    backup_file_t *file = backup_file_create_with_data(data, size, 0);

    if (file) {
        backup_file_set_domain(file, domain);
        backup_file_set_path(file, path);
        backup_file_set_mode(file, mode | 0100000);
        inode_start++;

        backup_file_set_inode(file, inode_start);
        backup_file_set_uid(file, uid);
        backup_file_set_gid(file, gid);
        backup_file_set_time1(file, time(NULL));
        backup_file_set_time2(file, time(NULL));
        backup_file_set_time3(file, time(NULL));
        backup_file_set_flag(file, flag);

        backup_file_set_length(file, size);

        if (backup_update_file(backup, file) >= 0)
            ret = 0;
        else
            ret = -1;
        backup_file_free(file);

        if (!ret)
            backup_write_mbdb(backup);
    }
    return ret;
}

int backup_add_file_from_data(backup_t * backup, char *domain, char *data,
                              unsigned int size, char *path, int mode, int uid,
                              int gid, int flag)
{
    int ret = -1;
    backup_file_t *file = backup_file_create_with_data(data, size, 0);

    if (file) {
        backup_file_set_domain(file, domain);
        backup_file_set_path(file, path);
        backup_file_set_mode(file, mode | 0100000);
        inode_start++;

        backup_file_set_inode(file, inode_start);
        backup_file_set_uid(file, uid);
        backup_file_set_gid(file, gid);
        backup_file_set_time1(file, time(NULL));
        backup_file_set_time2(file, time(NULL));
        backup_file_set_time3(file, time(NULL));
        backup_file_set_flag(file, flag);

        backup_file_set_length(file, size);

        if (backup_update_file(backup, file) >= 0)
            ret = 0;
        else
            ret = -1;
        backup_file_free(file);

        if (!ret)
            backup_write_mbdb(backup);
    }
    return ret;
}
