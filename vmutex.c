/*
 * lionfs, The Link Over Network File System
 * Copyright (C) 2016  Ricardo Biehl Pasquali <rbpoficial@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/* We have two sides which can lock our vmutexes:
 *  1st side uses acquire/release functions and it can only hold mutex when
 *    `counter` is zero. If counter is not zero `acquire` waits.
 *  2nd side uses increment/decrement functions and when it holds mutex it
 *    increments `counter`. This side can't hold mutex while 1st side is already
 *    holding it.
 */

#include <pthread.h>
#include <stdlib.h>

#include "vmutex.h"

void
vmutex_init (vmutex_t *vmtx) {
	pthread_mutex_init(&vmtx->control, NULL);
	pthread_cond_init(&vmtx->cond, NULL);
	vmtx->counter = 0;
}

void
vmutex_destroy (vmutex_t *vmtx) {
	pthread_mutex_destroy(&vmtx->control);
	pthread_cond_destroy(&vmtx->cond);
}

void
vmutex_increment (vmutex_t *vmtx) {
	pthread_mutex_lock(&vmtx->control);
	vmtx->counter++;
	pthread_mutex_unlock(&vmtx->control);
}

void
vmutex_decrement (vmutex_t *vmtx) {
	pthread_mutex_lock(&vmtx->control);
	vmtx->counter--;
	if(!vmtx->counter)
		pthread_cond_signal(&vmtx->cond);
	pthread_mutex_unlock(&vmtx->control);
}

void
vmutex_acquire (vmutex_t *vmtx) {
	pthread_mutex_lock(&vmtx->control);
	if(vmtx->counter)
		pthread_cond_wait(&vmtx->cond, &vmtx->control);
}

void
vmutex_release (vmutex_t *vmtx) {
	pthread_mutex_unlock(&vmtx->control);
}
