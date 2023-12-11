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
#include "kernel.h"
#include "errno.h"

#include "util/debug.h"

#include "proc/proc.h"

#include "mm/mm.h"
#include "mm/mman.h"
#include "mm/page.h"
#include "mm/mmobj.h"
#include "mm/pframe.h"
#include "mm/pagetable.h"

#include "vm/pagefault.h"
#include "vm/vmmap.h"

#include "mm/tlb.h"
/*
 * This gets called by _pt_fault_handler in mm/pagetable.c The
 * calling function has already done a lot of error checking for
 * us. In particular it has checked that we are not page faulting
 * while in kernel mode. Make sure you understand why an
 * unexpected page fault in kernel mode is bad in Weenix. You
 * should probably read the _pt_fault_handler function to get a
 * sense of what it is doing.
 *
 * Before you can do anything you need to find the vmarea that
 * contains the address that was faulted on. Make sure to check
 * the permissions on the area to see if the process has
 * permission to do [cause]. If either of these checks does not
 * pass kill the offending process, setting its exit status to
 * EFAULT (normally we would send the SIGSEGV signal, however
 * Weenix does not support signals).
 *
 * Now it is time to find the correct page. Make sure that if the
 * user writes to the page it will be handled correctly. This
 * includes your shadow objects' copy-on-write magic working
 * correctly.
 *
 * Finally call pt_map to have the new mapping placed into the
 * appropriate page table.
 *
 * @param vaddr the address that was accessed to cause the fault
 *
 * @param cause this is the type of operation on the memory
 *              address which caused the fault, possible values
 *              can be found in pagefault.h
 */
void
handle_pagefault(uintptr_t vaddr, uint32_t cause)
{
        //NOT_YET_IMPLEMENTED("VM: handle_pagefault");

        // Find a vmarea that contains the address that was faulted on
        vmarea_t *vma = vmmap_lookup(curproc->p_vmmap, ADDR_TO_PN(vaddr));
        if (vma == NULL) { // no vm_area found
                dbg(DBG_PRINT, "(GRADING3D)\n");
                do_exit(EFAULT);
        }
        dbg(DBG_PRINT, "(GRADING3D)\n");

        // Check the permissions on the area to see if the process has permission to do [cause]
        if ((vma->vma_prot & PROT_NONE) ||
                ((cause & FAULT_WRITE) && !(vma->vma_prot & PROT_WRITE)) || 
                ((cause & FAULT_EXEC) && !(vma->vma_prot & PROT_EXEC)) ||
                (!((cause & FAULT_WRITE) || (cause & FAULT_EXEC)) && !(vma->vma_prot & PROT_READ))) {
                // Kill the offending process, setting its exit status to EFAULT
                dbg(DBG_PRINT, "(GRADING3D)\n");
                do_exit(EFAULT);
        }
        dbg(DBG_PRINT, "(GRADING3D)\n");

        // Find the correct page
        pframe_t *pf;
        int err = pframe_lookup(vma->vma_obj, 
                ADDR_TO_PN(vaddr) - vma->vma_start + vma->vma_off, 
                ((cause & FAULT_WRITE) ? 1 : 0),
                &pf);
        if (err < 0) {
                dbg(DBG_PRINT, "(GRADING3D)\n");
                do_exit(EFAULT);
        }
        KASSERT(pf);
        KASSERT(pf->pf_addr);
        dbg(DBG_PRINT, "(GRADING3A 5.a)\n");

        // Perform copy-on-write (mark it as dirty)
        if (cause & FAULT_WRITE) {
                pframe_pin(pf);
                pframe_dirty(pf);
                //        pframe_unpin(pf);
                //        dbg(DBG_PRINT, "(GRADING3D)\n");
                //        do_exit(EFAULT);
                //}
                pframe_unpin(pf);
                dbg(DBG_PRINT, "(GRADING3D)\n");
        }
        
        // Call pt_map to have the new mapping placed into the appropriate page table
        int pdflags = PD_PRESENT | PD_USER;
        int ptflags = PT_PRESENT | PT_USER;
        if (cause & FAULT_WRITE) {
                pdflags |= PD_WRITE;
                ptflags |= PT_WRITE;
                dbg(DBG_PRINT, "(GRADING3D)\n");
        }
        pt_map(curproc->p_pagedir, 
                (uintptr_t)PAGE_ALIGN_DOWN(vaddr), 
                pt_virt_to_phys((uintptr_t)pf->pf_addr), 
                pdflags, 
                ptflags);

        tlb_flush_all();
        dbg(DBG_PRINT, "(GRADING3D)\n");


}
