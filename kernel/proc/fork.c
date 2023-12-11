/******************************************************************************/
/* Important Fall 2023 CSCI 402 usage information:                            */
/*                                                                            */
/* This fils is part of CSCI 402 kernel programming assignments at USC.       */
/*         53616c7465645f5fd1e93dbf35cbffa3aef28f8c01d8cf2ffc51ef62b26a       */
/*         f9bda5a68e5ed8c972b17bab0f42e24b19daa7bd408305b1f7bd6c7208c1       */
/*         0e36230e913039b3046dd5fd0ba706a624d33dbaa4d6aab02c82fe09f561       */
/*         01b0fd977b0051f0b0ce0c69f7db857b1b5e007be2db6d42894bf93de848       */
/*         806d9152bd5715e9                                                   */
/* Please understand that you are NOT permitted to distribute or publically   */
/*         display a copy of this file (or ANY PART of it) for any reason.    */
/* If anyone (including your prospective employer) asks you to post the code, */
/*         you must inform them that you do NOT have permissions to do so.    */
/* You are also NOT permitted to remove or alter this comment block.          */
/* If this comment block is removed or altered in a submitted file, 20 points */
/*         will be deducted.                                                  */
/******************************************************************************/

#include "types.h"
#include "globals.h"
#include "errno.h"

#include "util/debug.h"
#include "util/string.h"

#include "proc/proc.h"
#include "proc/kthread.h"

#include "mm/mm.h"
#include "mm/mman.h"
#include "mm/page.h"
#include "mm/pframe.h"
#include "mm/mmobj.h"
#include "mm/pagetable.h"
#include "mm/tlb.h"

#include "fs/file.h"
#include "fs/vnode.h"

#include "vm/shadow.h"
#include "vm/vmmap.h"

#include "api/exec.h"

#include "main/interrupt.h"

/* Pushes the appropriate things onto the kernel stack of a newly forked thread
 * so that it can begin execution in userland_entry.
 * regs: registers the new thread should have on execution
 * kstack: location of the new thread's kernel stack
 * Returns the new stack pointer on success. */
static uint32_t
fork_setup_stack(const regs_t *regs, void *kstack)
{
        /* Pointer argument and dummy return address, and userland dummy return
         * address */
        uint32_t esp = ((uint32_t) kstack) + DEFAULT_STACK_SIZE - (sizeof(regs_t) + 12);
        *(void **)(esp + 4) = (void *)(esp + 8); /* Set the argument to point to location of struct on stack */
        memcpy((void *)(esp + 8), regs, sizeof(regs_t)); /* Copy over struct */
        return esp;
}

/*
 * The implementation of fork(2). Once this works,
 * you're practically home free. This is what the
 * entirety of Weenix has been leading up to.
 * Go forth and conquer.
 */
int
do_fork(struct regs *regs)
{
        vmarea_t *vma, *clone_vma;
        pframe_t *pf;
        mmobj_t *to_delete, *new_shadowed;

        //NOT_YET_IMPLEMENTED("VM: do_fork");
        KASSERT(regs != NULL);
        KASSERT(curproc != NULL);
        KASSERT(curproc->p_state == PROC_RUNNING);
        dbg(DBG_PRINT, "(GRADING3A 7.a)\n");

        // Allocate proc_t
        proc_t *new_proc = proc_create("new_proc");
        //if (new_proc == NULL) {
        //        dbg(DBG_PRINT, "(GRADING3D)\n");
        //        return -ENOMEM;
        //}
        dbg(DBG_PRINT, "(GRADING3A)\n");
        
        // Copy the vmmap_t from the parent process into the child
        new_proc->p_vmmap = vmmap_clone(curproc->p_vmmap);
        //if (new_proc->p_vmmap == NULL) {
        //        dbg(DBG_PRINT, "(GRADING3D)\n");
        //        return -ENOMEM;
        //}
        new_proc->p_vmmap->vmm_proc = new_proc;
        dbg(DBG_PRINT, "(GRADING3A)\n");
        
        // Iterate through both the parent and child vmareas
        int err = 0;
        list_t *parent_list = &curproc->p_vmmap->vmm_list;
        list_t *child_list = &new_proc->p_vmmap->vmm_list;
        list_link_t *parent_link = parent_list->l_next;
        list_link_t *child_link = child_list->l_next;
        dbg(DBG_PRINT, "(GRADING3A)\n");
        while (parent_link != parent_list && child_link != child_list) {
                vma = list_item(parent_link, vmarea_t, vma_plink);
                clone_vma = list_item(child_link, vmarea_t, vma_plink);
                // Copy the chain from parent to child
                // clone_vma->vma_obj = vma->vma_obj;
                // increase refcount of the chain's head
                // vma->vma_obj->mmo_ops->ref(vma->vma_obj); 
                if ((vma->vma_flags & MAP_TYPE) == MAP_PRIVATE) {

                        vma->vma_obj->mmo_ops->ref(vma->vma_obj);

                        mmobj_t *vma_shadow = shadow_create();
                        vma_shadow->mmo_shadowed = vma->vma_obj;
                        vma_shadow->mmo_un.mmo_bottom_obj = vma->vma_obj->mmo_un.mmo_bottom_obj;

                        mmobj_t *c_shadow = shadow_create();
                        c_shadow->mmo_shadowed = vma->vma_obj;
                        c_shadow->mmo_un.mmo_bottom_obj = vma->vma_obj->mmo_un.mmo_bottom_obj;
                        vma->vma_obj = vma_shadow;
                        clone_vma->vma_obj = c_shadow;
                        // For each private mapping, point vmarea_t at the new shadow object
                        // err = setup_mapping(vma, clone_vma);
                        // if (err < 0) {
                        //         dbg(DBG_PRINT, "(GRADING3D)\n");
                        //         break;
                        // }
                        dbg(DBG_PRINT, "(GRADING3A)\n");
                }
                else
                {
                    clone_vma->vma_obj = vma->vma_obj;
                    clone_vma->vma_obj->mmo_ops->ref(clone_vma->vma_obj);
                    dbg(DBG_PRINT, "(GRADING3A)\n");
                }
                parent_link = parent_link->l_next;
                child_link = child_link->l_next;
                dbg(DBG_PRINT, "(GRADING3D)\n");
        }
        
        //if (err < 0) {
        //        error_cleanup(new_proc);
        //        dbg(DBG_PRINT, "(GRADING3D)\n");
        //        return err;
        //}
        dbg(DBG_PRINT, "(GRADING3D)\n");

        // Copy the thread and add it to the process
        kthread_t *new_thr = kthread_clone(curthr);
        //if (new_thr == NULL) {
        //        error_cleanup(new_proc);
        //        dbg(DBG_PRINT, "(GRADING3D)\n");
        //        return -ENOMEM;
        //}
        new_thr->kt_proc = new_proc;
        list_insert_tail(&new_proc->p_threads, &new_thr->kt_plink);

        // Set up the return value for child process so it can be directly copied into thread context
        regs->r_eax = 0;

        // Set up thread context
        new_thr->kt_ctx.c_pdptr = new_proc->p_pagedir;
        new_thr->kt_ctx.c_eip = (uint32_t)userland_entry;
        new_thr->kt_ctx.c_esp = fork_setup_stack(regs, new_thr->kt_kstack);
        new_thr->kt_ctx.c_kstack = (uintptr_t)new_thr->kt_kstack;
        new_thr->kt_ctx.c_kstacksz = DEFAULT_STACK_SIZE;
        dbg(DBG_PRINT, "(GRADING3D)\n");

        // Copy the file descriptor table
        int i;
        for (i = 0; i < NFILES; i++) {
                if (curproc->p_files[i] != NULL) {
                        new_proc->p_files[i] = curproc->p_files[i];
                        fref(new_proc->p_files[i]);
                        dbg(DBG_PRINT, "(GRADING3D)\n");
                }
                dbg(DBG_PRINT, "(GRADING3D)\n");
        }

        // Copy the current working directory
        // new_proc->p_cwd = curproc->p_cwd;
        // vref(new_proc->p_cwd);
        if(new_proc->p_cwd != NULL){
                vput(new_proc->p_cwd);
                dbg(DBG_PRINT, "(GRADING3D)\n");
        }
        new_proc->p_cwd = curproc->p_cwd;
        if(new_proc->p_cwd != NULL){
                vref(new_proc->p_cwd);
                dbg(DBG_PRINT, "(GRADING3D)\n");
        }
        // Unmap the user land page table entries and flush the TLB (using pt_unmap_range() and tlb_flush_all())
        pt_unmap_range(curproc->p_pagedir, USER_MEM_LOW, USER_MEM_HIGH);
        tlb_flush_all();

        // Set process break
        new_proc->p_brk = curproc->p_brk;
        new_proc->p_start_brk = curproc->p_start_brk;

        // Run the process
        sched_make_runnable(new_thr);
        dbg(DBG_PRINT, "(GRADING3D)\n");

        KASSERT(new_proc->p_state == PROC_RUNNING);
        KASSERT(new_proc->p_pagedir != NULL);
        KASSERT(new_thr->kt_kstack != NULL);
        dbg(DBG_PRINT, "(GRADING3A 7.a)\n");

        // Return the PID of the child process
        regs->r_eax = new_proc->p_pid;
        dbg(DBG_PRINT, "(GRADING3D)\n");

        return new_proc->p_pid;
}
