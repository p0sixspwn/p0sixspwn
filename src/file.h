/**
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

#ifndef _FILE_H_
#define _FILE_H_

typedef struct file_t {
    FILE *desc;
    char *path;
    uint64_t size;
    uint64_t offset;
    unsigned char *data;
} file_t;

file_t *file_create();
void file_close(file_t * file);
void file_free(file_t * file);
file_t *file_open(const char *path);

int file_read(const char *file, unsigned char **buf, unsigned int *length);
int file_write(const char *file, unsigned char *buf, unsigned int length);
int file_copy(const char *from, const char *to);

#endif
