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

#include "kernel.h"
#include "errno.h"
#include "globals.h"

#include "vm/vmmap.h"
#include "vm/shadow.h"
#include "vm/anon.h"

#include "proc/proc.h"

#include "util/debug.h"
#include "util/list.h"
#include "util/string.h"
#include "util/printf.h"

#include "fs/vnode.h"
#include "fs/file.h"
#include "fs/fcntl.h"
#include "fs/vfs_syscall.h"

#include "mm/slab.h"
#include "mm/page.h"
#include "mm/mm.h"
#include "mm/mman.h"
#include "mm/mmobj.h"

#include "mm/tlb.h"

static slab_allocator_t *vmmap_allocator;
static slab_allocator_t *vmarea_allocator;

void
vmmap_init(void)
{
        vmmap_allocator = slab_allocator_create("vmmap", sizeof(vmmap_t));
        KASSERT(NULL != vmmap_allocator && "failed to create vmmap allocator!");
        vmarea_allocator = slab_allocator_create("vmarea", sizeof(vmarea_t));
        KASSERT(NULL != vmarea_allocator && "failed to create vmarea allocator!");
}

vmarea_t *
vmarea_alloc(void)
{
        vmarea_t *newvma = (vmarea_t *) slab_obj_alloc(vmarea_allocator);
        if (newvma) {
                newvma->vma_vmmap = NULL;
        }
        return newvma;
}

void
vmarea_free(vmarea_t *vma)
{
        KASSERT(NULL != vma);
        slab_obj_free(vmarea_allocator, vma);
}

/* a debugging routine: dumps the mappings of the given address space. */
size_t
vmmap_mapping_info(const void *vmmap, char *buf, size_t osize)
{
        KASSERT(0 < osize);
        KASSERT(NULL != buf);
        KASSERT(NULL != vmmap);

        vmmap_t *map = (vmmap_t *)vmmap;
        vmarea_t *vma;
        ssize_t size = (ssize_t)osize;

        int len = snprintf(buf, size, "%21s %5s %7s %8s %10s %12s\n",
                           "VADDR RANGE", "PROT", "FLAGS", "MMOBJ", "OFFSET",
                           "VFN RANGE");

        list_iterate_begin(&map->vmm_list, vma, vmarea_t, vma_plink) {
                size -= len;
                buf += len;
                if (0 >= size) {
                        goto end;
                }

                len = snprintf(buf, size,
                               "%#.8x-%#.8x  %c%c%c  %7s 0x%p %#.5x %#.5x-%#.5x\n",
                               vma->vma_start << PAGE_SHIFT,
                               vma->vma_end << PAGE_SHIFT,
                               (vma->vma_prot & PROT_READ ? 'r' : '-'),
                               (vma->vma_prot & PROT_WRITE ? 'w' : '-'),
                               (vma->vma_prot & PROT_EXEC ? 'x' : '-'),
                               (vma->vma_flags & MAP_SHARED ? " SHARED" : "PRIVATE"),
                               vma->vma_obj, vma->vma_off, vma->vma_start, vma->vma_end);
        } list_iterate_end();

end:
        if (size <= 0) {
                size = osize;
                buf[osize - 1] = '\0';
        }
        /*
        KASSERT(0 <= size);
        if (0 == size) {
                size++;
                buf--;
                buf[0] = '\0';
        }
        */
        return osize - size;
}

/* Create a new vmmap, which has no vmareas and does
 * not refer to a process. */
vmmap_t *
vmmap_create(void)
{
        // NOT_YET_IMPLEMENTED("VM: vmmap_create");
        vmmap_t *new_map = (vmmap_t *) slab_obj_alloc(vmmap_allocator);
        list_init(&new_map->vmm_list);
        new_map->vmm_proc = NULL;
        dbg(DBG_PRINT, "(GRADING3A)\n");
        return new_map;
}

/* Removes all vmareas from the address space and frees the
 * vmmap struct. */
void
vmmap_destroy(vmmap_t *map)
{
        // NOT_YET_IMPLEMENTED("VM: vmmap_destroy");

        KASSERT(NULL != map);
        dbg(DBG_PRINT, "(GRADING3A 3.a)\n");

        vmarea_t *vma_t_p = NULL;
        list_iterate_begin(&map->vmm_list, vma_t_p, vmarea_t, vma_plink) 
        {
                list_remove(&vma_t_p->vma_plink);
                list_remove(&vma_t_p->vma_olink);
                vma_t_p->vma_obj->mmo_ops->put(vma_t_p->vma_obj);
                vmarea_free(vma_t_p);
                dbg(DBG_PRINT, "(GRADING3A)\n");
        }
        list_iterate_end();

        slab_obj_free(vmmap_allocator, map);
        dbg(DBG_PRINT, "(GRADING3A)\n");
        return;
}

/* Add a vmarea to an address space. Assumes (i.e. asserts to some extent)
 * the vmarea is valid.  This involves finding where to put it in the list
 * of VM areas, and adding it. Don't forget to set the vma_vmmap for the
 * area. */
void
vmmap_insert(vmmap_t *map, vmarea_t *newvma)
{
        // NOT_YET_IMPLEMENTED("VM: vmmap_insert");
        KASSERT(NULL != map && NULL != newvma);
        KASSERT(NULL == newvma->vma_vmmap);
        KASSERT(newvma->vma_start < newvma->vma_end);
        KASSERT(ADDR_TO_PN(USER_MEM_LOW) <= newvma->vma_start && ADDR_TO_PN(USER_MEM_HIGH) >= newvma->vma_end);
        dbg(DBG_PRINT, "(GRADING3A 3.b)\n");

        newvma->vma_vmmap = map;
        vmarea_t *vma_tmp = NULL;
        list_iterate_begin(&map->vmm_list, vma_tmp, vmarea_t, vma_plink)
        {
                if(vma_tmp->vma_start >= newvma->vma_end){
                        list_insert_before(&vma_tmp->vma_plink, &newvma->vma_plink);
                        dbg(DBG_PRINT, "(GRADING3A)\n");
                        return;
                }
        }
        list_iterate_end();

        list_insert_tail(&map->vmm_list, &newvma->vma_plink);
        dbg(DBG_PRINT, "(GRADING3A)\n");
        return;
}

/* Find a contiguous range of free virtual pages of length npages in
 * the given address space. Returns starting vfn for the range,
 * without altering the map. Returns -1 if no such range exists.
 *
 * Your algorithm should be first fit. If dir is VMMAP_DIR_HILO, you
 * should find a gap as high in the address space as possible; if dir
 * is VMMAP_DIR_LOHI, the gap should be as low as possible. */
int
vmmap_find_range(vmmap_t *map, uint32_t npages, int dir)
{
        // NOT_YET_IMPLEMENTED("VM: vmmap_find_range");
        vmarea_t *vm_area;
        uint32_t unit32_start = ADDR_TO_PN(USER_MEM_HIGH);
        dbg(DBG_PRINT, "(GRADING3A)\n");

        // as high in the address space as possible
        if(dir == VMMAP_DIR_HILO){
                list_iterate_reverse(&(map->vmm_list), vm_area, vmarea_t, vma_plink)
                {
                        if(npages <= unit32_start - vm_area->vma_end)
                        {
                                dbg(DBG_PRINT, "(GRADING3A)\n");
                                return unit32_start - npages;
                        }
                        unit32_start = vm_area->vma_start;
                        dbg(DBG_PRINT, "(GRADING3D 1)\n");
                }
                list_iterate_end();

                
                vmarea_t *vm_area_head = list_head(&map->vmm_list, vmarea_t, vma_plink);
                dbg(DBG_PRINT, "(GRADING3D 1)\n");

                if(vm_area_head->vma_start - ADDR_TO_PN(USER_MEM_LOW) >= npages)
                {      
                        dbg(DBG_PRINT, "(GRADING3D 1)\n");
                        return vm_area_head->vma_start - npages;
                }
                dbg(DBG_PRINT, "(GRADING3D 2)\n");
        }
        dbg(DBG_PRINT, "(GRADING3D 2)\n");
        return -1;
}

/* Find the vm_area that vfn lies in. Simply scan the address space
 * looking for a vma whose range covers vfn. If the page is unmapped,
 * return NULL. */
vmarea_t *
vmmap_lookup(vmmap_t *map, uint32_t vfn)
{
        // NOT_YET_IMPLEMENTED("VM: vmmap_lookup");

        KASSERT(NULL != map); 
        dbg(DBG_PRINT, "(GRADING3A 3.c)\n");

        vmarea_t *vm_area = NULL;
        list_iterate_begin(&map->vmm_list, vm_area, vmarea_t, vma_plink)
        {
                dbg(DBG_PRINT, "(GRADING3A)\n");
                if(vfn < vm_area->vma_end && vfn >= vm_area->vma_start)
                {
                        dbg(DBG_PRINT, "(GRADING3A)\n");
                        return vm_area;
                }
        }
        list_iterate_end();

        dbg(DBG_PRINT, "(GRADING3D 1)\n");
        return NULL;
}

/* Allocates a new vmmap containing a new vmarea for each area in the
 * given map. The areas should have no mmobjs set yet. Returns pointer
 * to the new vmmap on success, NULL on failure. This function is
 * called when implementing fork(2). */
vmmap_t *
vmmap_clone(vmmap_t *map)
{
        // NOT_YET_IMPLEMENTED("VM: vmmap_clone");
        vmmap_t *clone_map = vmmap_create();

        vmarea_t *tmp_area;
        list_iterate_begin(&map->vmm_list, tmp_area, vmarea_t, vma_plink) {
                vmarea_t *clone_area = vmarea_alloc();

                // copy each member of vmarea_t
                clone_area->vma_vmmap = clone_map;
                clone_area->vma_obj = NULL;
                clone_area->vma_start = tmp_area->vma_start;
                clone_area->vma_end = tmp_area->vma_end;
                clone_area->vma_flags = tmp_area->vma_flags;
                clone_area->vma_off = tmp_area->vma_off;
                clone_area->vma_prot = tmp_area->vma_prot;
                list_init(&(clone_area->vma_plink));
                list_init(&(clone_area->vma_olink));

                list_insert_tail(&(clone_map->vmm_list), &(clone_area->vma_plink));

                dbg(DBG_PRINT, "(GRADING3A)\n");

        } list_iterate_end();

        dbg(DBG_PRINT, "(GRADING3A)\n");

        return clone_map;
}

/* Insert a mapping into the map starting at lopage for npages pages.
 * If lopage is zero, we will find a range of virtual addresses in the
 * process that is big enough, by using vmmap_find_range with the same
 * dir argument.  If lopage is non-zero and the specified region
 * contains another mapping that mapping should be unmapped.
 *
 * If file is NULL an anon mmobj will be used to create a mapping
 * of 0's.  If file is non-null that vnode's file will be mapped in
 * for the given range.  Use the vnode's mmap operation to get the
 * mmobj for the file; do not assume it is file->vn_obj. Make sure all
 * of the area's fields except for vma_obj have been set before
 * calling mmap.
 *
 * If MAP_PRIVATE is specified set up a shadow object for the mmobj.
 *
 * All of the input to this function should be valid (KASSERT!).
 * See mmap(2) for for description of legal input.
 * Note that off should be page aligned.
 *
 * Be very careful about the order operations are performed in here. Some
 * operation are impossible to undo and should be saved until there
 * is no chance of failure.
 *
 * If 'new' is non-NULL a pointer to the new vmarea_t should be stored in it.
 */
int
vmmap_map(vmmap_t *map, vnode_t *file, uint32_t lopage, uint32_t npages,
          int prot, int flags, off_t off, int dir, vmarea_t **new)
{
        // NOT_YET_IMPLEMENTED("VM: vmmap_map");
        KASSERT(NULL != map);

        dbg(DBG_PRINT, "(GRADING3A 3.d)\n");

        KASSERT(0 < npages);

        dbg(DBG_PRINT, "(GRADING3A 3.d)\n");

        KASSERT((MAP_SHARED & flags) || (MAP_PRIVATE & flags));

        dbg(DBG_PRINT, "(GRADING3A 3.d)\n");

        KASSERT((0 == lopage) || (ADDR_TO_PN(USER_MEM_LOW) <= lopage));

        dbg(DBG_PRINT, "(GRADING3A 3.d)\n");

        KASSERT((0 == lopage) || (ADDR_TO_PN(USER_MEM_HIGH) >= (lopage + npages)));

        dbg(DBG_PRINT, "(GRADING3A 3.d)\n");

        KASSERT(PAGE_ALIGNED(off));

        dbg(DBG_PRINT, "(GRADING3A 3.d)\n");

        if (lopage == 0) {
                int va_range = vmmap_find_range(map, npages, dir);
                if (va_range < 0) {

                        dbg(DBG_PRINT, "(GRADING3D 2)\n");

                        return va_range;
                }
                lopage = va_range;

                dbg(DBG_PRINT, "(GRADING3A)\n");

        } else if (!vmmap_is_range_empty(map, lopage, npages)) {

                dbg(DBG_PRINT, "(GRADING3A)\n");

                vmmap_remove(map, lopage, npages);
        }

        vmarea_t *vm_area = vmarea_alloc();
        vm_area->vma_start = lopage;
        vm_area->vma_end = lopage + npages;
        vm_area->vma_prot = prot;
        vm_area->vma_flags = flags;
        vm_area->vma_off = ADDR_TO_PN(off);
        list_init(&(vm_area->vma_plink));
        list_init(&(vm_area->vma_olink));
        vmmap_insert(map, vm_area);

        mmobj_t *mm_obj = NULL;
        if (file == NULL) {
                mm_obj = anon_create();

                dbg(DBG_PRINT, "(GRADING3A)\n");

        } else {
                int ret_no = file->vn_ops->mmap(file, vm_area, &mm_obj);

                dbg(DBG_PRINT, "(GRADING3A)\n");

        }
        vm_area->vma_obj = mm_obj;
        list_insert_head(&(mm_obj->mmo_un.mmo_vmas), &(vm_area->vma_olink));

        if (MAP_PRIVATE & vm_area->vma_flags) {
                mmobj_t *shadow_obj = shadow_create();
                shadow_obj->mmo_shadowed = mm_obj;
                shadow_obj->mmo_un.mmo_bottom_obj = mmobj_bottom_obj(mm_obj);
                vm_area->vma_obj = shadow_obj;

                dbg(DBG_PRINT, "(GRADING3A)\n");

        }

        if (new != NULL) {
                *new = vm_area;

                dbg(DBG_PRINT, "(GRADING3A)\n");

        }

        dbg(DBG_PRINT, "(GRADING3A)\n");

        return 0;
}

/*
 * We have no guarantee that the region of the address space being
 * unmapped will play nicely with our list of vmareas.
 *
 * You must iterate over each vmarea that is partially or wholly covered
 * by the address range [addr ... addr+len). The vm-area will fall into one
 * of four cases, as illustrated below:
 *
 * key:
 *          [             ]   Existing VM Area
 *        *******             Region to be unmapped
 *
 * Case 1:  [   ******    ]
 * The region to be unmapped lies completely inside the vmarea. We need to
 * split the old vmarea into two vmareas. be sure to increment the
 * reference count to the file associated with the vmarea.
 *
 * Case 2:  [      *******]**
 * The region overlaps the end of the vmarea. Just shorten the length of
 * the mapping.
 *
 * Case 3: *[*****        ]
 * The region overlaps the beginning of the vmarea. Move the beginning of
 * the mapping (remember to update vma_off), and shorten its length.
 *
 * Case 4: *[*************]**
 * The region completely contains the vmarea. Remove the vmarea from the
 * list.
 */
int
vmmap_remove(vmmap_t *map, uint32_t lopage, uint32_t npages)
{
        // NOT_YET_IMPLEMENTED("VM: vmmap_remove");
        vmarea_t *vm_area = NULL;
        uint32_t bound = lopage + npages;
        list_iterate_begin(&map->vmm_list, vm_area, vmarea_t, vma_plink) {
                if (vm_area->vma_end > bound && vm_area->vma_start < lopage) {
                        vmarea_t *new_vm_area = vmarea_alloc();
                        new_vm_area->vma_start = bound;
                        new_vm_area->vma_end = vm_area->vma_end;
                        new_vm_area->vma_off = vm_area->vma_off + bound - vm_area->vma_start;
                        new_vm_area->vma_prot = vm_area->vma_prot;
                        new_vm_area->vma_flags = vm_area->vma_flags;
                        new_vm_area->vma_obj = vm_area->vma_obj;
                        list_init(&(new_vm_area->vma_plink));
                        list_init(&(new_vm_area->vma_olink));
                        vmmap_insert(map, new_vm_area);
                        new_vm_area->vma_obj->mmo_ops->ref(new_vm_area->vma_obj);

                        vm_area->vma_end = lopage;

                        dbg(DBG_PRINT, "(GRADING3D 2)\n");

                } else if (vm_area->vma_end > lopage && vm_area->vma_end <= bound && vm_area->vma_start < lopage) {
                        vm_area->vma_end = lopage;

                        dbg(DBG_PRINT, "(GRADING3D 1)\n");

                } else if (vm_area->vma_end > bound && vm_area->vma_start >= lopage && vm_area->vma_start < bound) {
                        vm_area->vma_off += bound - vm_area->vma_start;
                        vm_area->vma_start = bound;

                        dbg(DBG_PRINT, "(GRADING3D 2)\n");

                } else if (vm_area->vma_end <= bound && vm_area->vma_start >= lopage) {
                        vm_area->vma_obj->mmo_ops->put(vm_area->vma_obj);
                        list_remove(&(vm_area->vma_plink));
                        list_remove(&(vm_area->vma_olink));
                        vmarea_free(vm_area);

                        dbg(DBG_PRINT, "(GRADING3D 1)\n");

                }
        } list_iterate_end();

        tlb_flush_all();
        pt_unmap_range(curproc->p_pagedir, (uintptr_t) PN_TO_ADDR(lopage), (uintptr_t) PN_TO_ADDR(bound));


        dbg(DBG_PRINT, "(GRADING3D 1)\n");

        return 0;
}

/*
 * Returns 1 if the given address space has no mappings for the
 * given range, 0 otherwise.
 */
int
vmmap_is_range_empty(vmmap_t *map, uint32_t startvfn, uint32_t npages)
{
        // NOT_YET_IMPLEMENTED("VM: vmmap_is_range_empty");
        KASSERT((startvfn < startvfn + npages) && (ADDR_TO_PN(USER_MEM_LOW) <= startvfn) && (ADDR_TO_PN(USER_MEM_HIGH) >= startvfn + npages));

        dbg(DBG_PRINT, "(GRADING3A 3.e)\n");

        vmarea_t *vm_area = NULL;
        list_iterate_begin(&map->vmm_list, vm_area, vmarea_t, vma_plink) {
                if (vm_area->vma_start < startvfn + npages && vm_area->vma_end > startvfn) {

                        dbg(DBG_PRINT, "(GRADING3A)\n");

                        return 0;
                }

        } list_iterate_end();

        dbg(DBG_PRINT, "(GRADING3A)\n");

        return 1;
}

/* Read into 'buf' from the virtual address space of 'map' starting at
 * 'vaddr' for size 'count'. To do so, you will want to find the vmareas
 * to read from, then find the pframes within those vmareas corresponding
 * to the virtual addresses you want to read, and then read from the
 * physical memory that pframe points to. You should not check permissions
 * of the areas. Assume (KASSERT) that all the areas you are accessing exist.
 * Returns 0 on success, -errno on error.
 */
int
vmmap_read(vmmap_t *map, const void *vaddr, void *buf, size_t count)
{
        // NOT_YET_IMPLEMENTED("VM: vmmap_read");
        size_t curr_addr = (size_t) vaddr;
        size_t unread = count;
        size_t readed = 0;

        while (unread > 0) {
                vmarea_t *vm_area = vmmap_lookup(map, ADDR_TO_PN(curr_addr));
                pframe_t *pframe = NULL;
                size_t page_num = vm_area->vma_off - vm_area->vma_start + ADDR_TO_PN(curr_addr);
                vm_area->vma_obj->mmo_ops->lookuppage(vm_area->vma_obj, page_num, 0, &pframe);

                size_t num = 0;
                size_t bound = PAGE_SIZE - PAGE_OFFSET(curr_addr);
                if (unread <= bound) {
                        num = unread;

                        dbg(DBG_PRINT, "(GRADING3A)\n");

                } else {
                        num = bound;

                        dbg(DBG_PRINT, "(GRADING3D 1)\n");

                }
                memcpy((char*)buf + readed, (char*)pframe->pf_addr + PAGE_OFFSET(curr_addr), num);
                readed += num;
                curr_addr += num;
                unread -= num;

                dbg(DBG_PRINT, "(GRADING3A)\n");

        }

        dbg(DBG_PRINT, "(GRADING3A)\n");

        return 0;
}

/* Write from 'buf' into the virtual address space of 'map' starting at
 * 'vaddr' for size 'count'. To do this, you will need to find the correct
 * vmareas to write into, then find the correct pframes within those vmareas,
 * and finally write into the physical addresses that those pframes correspond
 * to. You should not check permissions of the areas you use. Assume (KASSERT)
 * that all the areas you are accessing exist. Remember to dirty pages!
 * Returns 0 on success, -errno on error.
 */
int
vmmap_write(vmmap_t *map, void *vaddr, const void *buf, size_t count)
{
        // NOT_YET_IMPLEMENTED("VM: vmmap_write");
        size_t curr_addr = (size_t) vaddr;
        size_t unwritten = count;
        size_t written = 0;

        while (unwritten > 0) {
                vmarea_t *vm_area = vmmap_lookup(map, ADDR_TO_PN(curr_addr));
                pframe_t *pframe = NULL;
                size_t page_num = vm_area->vma_off - vm_area->vma_start + ADDR_TO_PN(curr_addr);
                vm_area->vma_obj->mmo_ops->lookuppage(vm_area->vma_obj, page_num, 1, &pframe);

                size_t num = 0;
                size_t bound = PAGE_SIZE - PAGE_OFFSET(curr_addr);
                if (unwritten <= bound) {
                        num = unwritten;

                        dbg(DBG_PRINT, "(GRADING3A)\n");

                } else {
                        num = bound;

                        dbg(DBG_PRINT, "(GRADING3D 1)\n");

                }
                memcpy((char*)pframe->pf_addr + PAGE_OFFSET(curr_addr), (char*)buf + written, num);
                written += num;
                curr_addr += num;
                unwritten -= num;

                pframe_dirty(pframe);

                dbg(DBG_PRINT, "(GRADING3A)\n");

        }

        dbg(DBG_PRINT, "(GRADING3A)\n");

        return 0;
}
