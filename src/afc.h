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

#ifndef _AFC_H_
#define _AFC_H_

typedef struct afc_t {
    uint16_t port;
    device_t *device;
    afc_client_t client;
} afc_t;

afc_t *afc_create();
afc_t *afc_connect(device_t * device);
afc_t *afc_open(device_t * device, uint16_t port);

int afc_send_file(afc_t * afc, const char *local, const char *remote);
int afc_close(afc_t * afc);
void afc_free(afc_t * afc);
void apparition_afc_get_file_contents(afc_t * afc, const char *filename,
                                      char **data, uint64_t * size);

#endif
