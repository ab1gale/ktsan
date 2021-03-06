/* kernel/rwsem.c: R/W semaphores, public implementation
 *
 * Written by David Howells (dhowells@redhat.com).
 * Derived from asm-i386/semaphore.h
 */

#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/export.h>
#include <linux/rwsem.h>
#include <linux/ktsan.h>

#include <linux/atomic.h>

#include "rwsem.h"

/*
 * lock for reading
 */
void __sched down_read(struct rw_semaphore *sem)
{
	might_sleep();
	ktsan_mtx_pre_lock(sem, false, false);
	rwsem_acquire_read(&sem->dep_map, 0, 0, _RET_IP_);

	LOCK_CONTENDED(sem, __down_read_trylock, __down_read);
	ktsan_mtx_post_lock(sem, false, false, true);
}

EXPORT_SYMBOL(down_read);

/*
 * trylock for reading -- returns 1 if successful, 0 if contention
 */
int down_read_trylock(struct rw_semaphore *sem)
{
	int ret;

	ktsan_mtx_pre_lock(sem, false, true);
	ret = __down_read_trylock(sem);

	if (ret == 1)
		rwsem_acquire_read(&sem->dep_map, 0, 1, _RET_IP_);
	ktsan_mtx_post_lock(sem, false, true, ret == 1);
	return ret;
}

EXPORT_SYMBOL(down_read_trylock);

/*
 * lock for writing
 */
void __sched down_write(struct rw_semaphore *sem)
{
	might_sleep();
	ktsan_mtx_pre_lock(sem, true, false);
	rwsem_acquire(&sem->dep_map, 0, 0, _RET_IP_);

	LOCK_CONTENDED(sem, __down_write_trylock, __down_write);
	rwsem_set_owner(sem);
	ktsan_mtx_post_lock(sem, true, false, true);
}

EXPORT_SYMBOL(down_write);

/*
 * trylock for writing -- returns 1 if successful, 0 if contention
 */
int down_write_trylock(struct rw_semaphore *sem)
{
	int ret;

	ktsan_mtx_pre_lock(sem, true, true);
	ret = __down_write_trylock(sem);

	if (ret == 1) {
		rwsem_acquire(&sem->dep_map, 0, 1, _RET_IP_);
		rwsem_set_owner(sem);
	}
	ktsan_mtx_post_lock(sem, true, true, ret == 1);

	return ret;
}

EXPORT_SYMBOL(down_write_trylock);

/*
 * release a read lock
 */
void up_read(struct rw_semaphore *sem)
{
	ktsan_mtx_pre_unlock(sem, false);
	rwsem_release(&sem->dep_map, 1, _RET_IP_);

	__up_read(sem);
	ktsan_mtx_post_unlock(sem, false);
}

EXPORT_SYMBOL(up_read);

/*
 * release a write lock
 */
void up_write(struct rw_semaphore *sem)
{
	ktsan_mtx_pre_unlock(sem, true);
	rwsem_release(&sem->dep_map, 1, _RET_IP_);

	rwsem_clear_owner(sem);
	__up_write(sem);
	ktsan_mtx_post_unlock(sem, true);
}

EXPORT_SYMBOL(up_write);

/*
 * downgrade write lock to read lock
 */
void downgrade_write(struct rw_semaphore *sem)
{
	/*
	 * lockdep: a downgraded write will live on as a write
	 * dependency.
	 */
	rwsem_clear_owner(sem);
	__downgrade_write(sem);
}

EXPORT_SYMBOL(downgrade_write);

#ifdef CONFIG_DEBUG_LOCK_ALLOC

void down_read_nested(struct rw_semaphore *sem, int subclass)
{
	might_sleep();
	ktsan_mtx_pre_lock(sem, false, false);
	rwsem_acquire_read(&sem->dep_map, subclass, 0, _RET_IP_);

	LOCK_CONTENDED(sem, __down_read_trylock, __down_read);
	ktsan_mtx_post_lock(sem, false, false, true);
}

EXPORT_SYMBOL(down_read_nested);

void _down_write_nest_lock(struct rw_semaphore *sem, struct lockdep_map *nest)
{
	might_sleep();
	ktsan_mtx_pre_lock(sem, true, false);
	rwsem_acquire_nest(&sem->dep_map, 0, 0, nest, _RET_IP_);

	LOCK_CONTENDED(sem, __down_write_trylock, __down_write);
	rwsem_set_owner(sem);
	ktsan_mtx_post_lock(sem, true, false, true);
}

EXPORT_SYMBOL(_down_write_nest_lock);

void down_read_non_owner(struct rw_semaphore *sem)
{
	might_sleep();

	ktsan_mtx_pre_lock(sem, false, false);
	__down_read(sem);
	ktsan_mtx_post_lock(sem, false, false, true);
}

EXPORT_SYMBOL(down_read_non_owner);

void down_write_nested(struct rw_semaphore *sem, int subclass)
{
	might_sleep();
	ktsan_mtx_pre_lock(sem, true, false);
	rwsem_acquire(&sem->dep_map, subclass, 0, _RET_IP_);

	LOCK_CONTENDED(sem, __down_write_trylock, __down_write);
	rwsem_set_owner(sem);
	ktsan_mtx_post_lock(sem, true, false, true);
}

EXPORT_SYMBOL(down_write_nested);

void up_read_non_owner(struct rw_semaphore *sem)
{
	ktsan_mtx_pre_unlock(sem, false);
	__up_read(sem);
	ktsan_mtx_post_unlock(sem, false);
}

EXPORT_SYMBOL(up_read_non_owner);

#endif


