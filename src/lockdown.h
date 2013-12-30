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

#ifndef _LOCKDOWN_H_
#define _LOCKDOWN_H_

typedef struct lockdown_t {
    device_t *device;
    lockdownd_client_t client;
} lockdown_t;

lockdown_t *lockdown_open(device_t * device);
int lockdown_get_value(lockdown_t * lockdown, const char *domain,
                       const char *key, plist_t * value);
int lockdown_get_string(lockdown_t * lockdown, const char *key, char **value);
int lockdown_start_service(lockdown_t * lockdown, const char *service,
                           uint16_t * port);
int lockdown_stop_service(lockdown_t * lockdown, const char *service);
int lockdown_close(lockdown_t * lockdown);
void lockdown_free(lockdown_t * lockdown);

#endif
