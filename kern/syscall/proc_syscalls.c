#include <types.h>
#include <kern/errno.h>
#include <kern/unistd.h>
#include <kern/wait.h>
#include <lib.h>
#include <syscall.h>
#include <current.h>
#include <proc.h>
#include <thread.h>
#include <addrspace.h>
#include <copyinout.h>
#include <mips/trapframe.h> 
#include <types.h>
#include <kern/errno.h>
#include "opt-A2.h"
#include <kern/limits.h>
#include <vfs.h>
#include <kern/fcntl.h>
#include <array.h>


#if OPT_A2
int
sys_execv(userptr_t progname, userptr_t args)
{

	struct addrspace *as;
	struct vnode *v;
	vaddr_t entrypoint, stackptr;
	int result;

  // copy in grogram name
  size_t pName_size = strlen((char *)progname)+1 ;
  char* pName_kernel = kmalloc(pName_size * sizeof(char));
  KASSERT(pName_kernel != NULL);
  result = copyinstr(progname,pName_kernel,pName_size,NULL);
  if(result){
    return result;
  }
 

  //copy in char pointer values
  char *arg_ptr;
  int argc = 0;
  do
  {
    copyin(args + argc*sizeof(char*), &arg_ptr,sizeof(char*));
    argc++;
   
  } while (arg_ptr != NULL);
  argc--;
 
  char **argv = kmalloc((argc+1)*sizeof(char*)); 
  

  for(int i = 0; i < argc ; i ++){
  
    copyin(args + i*sizeof(char*), &arg_ptr, sizeof(char*));
  
    argv[i] = kmalloc((strlen(arg_ptr) + 1) * sizeof(char));
  
    result = copyinstr((userptr_t)arg_ptr,argv[i], strlen(arg_ptr) + 1 , NULL);  
    if(result){
      return result;
    }


  }
  argv[argc] = NULL;
  //array_cleanup(arg_ptr_array);

	/* Open the file. */
	result = vfs_open(pName_kernel, O_RDONLY, 0, &v);
	if (result) {
		return result;
	}

	/* We should be a new process. */
	//KASSERT(curproc_getas() == NULL);

	/* Create a new address space. */
	as = as_create();
	if (as ==NULL) {
		vfs_close(v);
		return ENOMEM;
	}

	/* Switch to it and activate it. */
	struct addrspace * as_old = curproc_setas(as);
	as_activate();

	/* Load the executable. */
	result = load_elf(v, &entrypoint);
	if (result) {
		/* p_addrspace will go away when curproc is destroyed */
		vfs_close(v);
		return result;
	}


	/* Done with the file now. */
	vfs_close(v);

	/* Define the user stack in the address space */
	result = as_define_stack(as, &stackptr);
	if (result) {
		/* p_addrspace will go away when curproc is destroyed */
		return result;
	}

//copy agrs from kern to new address space

  vaddr_t *stack_arg_ptr = kmalloc((argc+1) * sizeof(vaddr_t));
  stack_arg_ptr[argc] = (vaddr_t)NULL;
  for(int i = argc - 1; i >= 0; i-- ){
    int argL = ROUNDUP(strlen(argv[i]) + 1, 4);
    stackptr -= argL;
    result = copyoutstr(argv[i],(userptr_t)stackptr , strlen(argv[i]) + 1,NULL);
    if(result){
      return result;
    }
    stack_arg_ptr[i] = stackptr;
  }

  for(int i = argc ; i >= 0; i--){
    int argL = ROUNDUP(sizeof(vaddr_t), 4);
    stackptr -= argL;
    result = copyout((void*)&stack_arg_ptr[i], (userptr_t)stackptr, sizeof(vaddr_t));
    if(result){
      return result;
    }

  }
  
  as_destroy(as_old);
	/* Warp to user mode. */
  
	enter_new_process(argc /*argc*/, (userptr_t)stackptr /*userspace addr of argv*/,
			  stackptr, entrypoint);
	
	/* enter_new_process does not return. */
	panic("enter_new_process returned\n");
	return EINVAL;
}

int sys_fork(struct trapframe *tf,int* resval){
  //create a process structure for child process
  struct proc *child = proc_create_runprogram(curproc->p_name);
  if( child == NULL ){
    return ENPROC;
  }
  //
	//child->p_addrspace = tmp;
  child->parent = curproc;
  //parent/child relationship
  array_add(curproc->children,child,NULL);
  struct proc_info *cif = kmalloc(sizeof(struct proc_info));
  cif->pid = child->pid;
  cif->exitcode = child->exitcode;
  cif->exit = false;
  array_add(curproc->children_info,cif,NULL);

  //creates a new address space and copies the pages from the old address space to the new one.
  struct addrspace *tmp = NULL;
  int as_copy_res  = as_copy(curproc_getas(),&tmp);
  if(as_copy_res == ENOMEM){
    return (ENOMEM);
  }
  
  //create a thread for the child process
  
  struct trapframe *child_tf = kmalloc(sizeof(struct trapframe));
  if(child_tf == NULL){
    proc_destroy(child);
    return ENOMEM;
  }
  *child_tf = *tf;
  
  int thread_fork_res = thread_fork("child",child,enter_forked_process,child_tf,(unsigned long)tmp);
  if(thread_fork_res){
    kfree(child_tf);
    proc_destroy(child);
    return thread_fork_res;
  }
  *resval = child->pid;
  

  return 0;
}
/* this implementation of sys__exit does not do anything with the exit code */
  /* this needs to be fixed to get exit() and waitpid() working properly */

void sys__exit(int exitcode) {

  struct addrspace *as;
  struct proc *p = curproc;
  /* for now, just include this to keep the compiler from complaining about
     an unused variable */
  (void)exitcode;

  DEBUG(DB_SYSCALL,"Syscall: _exit(%d)\n",exitcode);

  KASSERT(curproc->p_addrspace != NULL);
  
  
  // bool deleteC = true;
  // for(unsigned i = 0; i < array_num(p->children_info) ; i++){
  //   struct proc* tmp = array_get(p->children_info,i);
  //   if(tmp){
  //     tmp->parent = NULL;
  //     if(!tmp->exit){
  //       deleteC = false;
  //     }
  //   }
  // }
  // if(deleteC){
  //   for(unsigned i = 0; i < array_num(p->children) ; i++){
  //     struct proc* tmp = array_get(p->children,i);
  //     if(tmp){
  //       proc_destroy(tmp);
  //     }
  //   }
  // }
  if(p->parent == NULL){
  }else{
    p->exit = true;
    
    struct proc_info* tmp = NULL;
    lock_acquire(p->parent->wait_child_lock);
    // struct proc *parent = p->parent;
    // struct proc_info *pif = p->parent->children_info;
    for(unsigned int i = 0; i < array_num(p->parent->children_info) ; i++){
      tmp = array_get(curproc->parent->children_info,i);
      if(tmp && tmp->pid == p->pid){
        tmp->exit = true;
        break;
      }
    }
    if(tmp == NULL){
      panic("impossible");
      return;
    }
		tmp->exitcode = exitcode;
    
		cv_signal(p->parent->wait_child, p->parent->wait_child_lock);
		lock_release(p->parent->wait_child_lock);
  }


  as_deactivate();
  /*
   * clear p_addrspace before calling as_destroy. Otherwise if
   * as_destroy sleeps (which is quite possible) when we
   * come back we'll be calling as_activate on a
   * half-destroyed address space. This tends to be
   * messily fatal.
   */
  as = curproc_setas(NULL);
  as_destroy(as);
  /* detach this thread from its process */
  /* note: curproc cannot be used after this call */
  proc_remthread(curthread);
  /* if this is the last user process in the system, proc_destroy()
     will wake up the kernel menu thread */
  //below not needed
  proc_destroy(p);
  
  thread_exit();
  /* thread_exit() does not return, so we should never get here */
  panic("return from thread_exit in sys_exit\n");
}


/* stub handler for getpid() system call                */
int
sys_getpid(pid_t *retval)
{
  /* for now, this is just a stub that always returns a PID of 1 */
  /* you need to fix this to make it work properly */
  *retval = curproc->pid;
  return 0;
}

/* stub handler for waitpid() system call                */

int
sys_waitpid(pid_t pid,
	    userptr_t status,
	    int options,
	    pid_t *retval)
{
  int exitstatus;
  int result;

  /* this is just a stub implementation that always reports an
     exit status of 0, regardless of the actual exit status of
     the specified process.   
     In fact, this will return 0 even if the specified process
     is still running, and even if it never existed in the first place.

     Fix this!
  */
  if (options != 0) {
    return(EINVAL);
  }
  /* for now, just pretend the exitstatus is 0 */


  // struct proc *c = NULL;
  // for(unsigned i = 0;i < array_num(curproc->children); i++){
  //   struct proc *tmp = array_get(curproc->children,i);
  //   if(tmp != NULL && tmp->pid == pid){
  //     c = tmp;
  //     break;
  //   }
  // }
  // if(c == NULL){
  //   return ECHILD;
  // }
  
  bool found = false;
  lock_acquire(curproc->wait_child_lock);
  for(unsigned i = 0; i< array_num(curproc->children_info);i++){
    struct proc_info * tmpcif = array_get(curproc->children_info, i);
      if (pid == tmpcif->pid) {
        found = true;
        while (tmpcif->exit == false) {
          cv_wait(curproc->wait_child, curproc->wait_child_lock);
        }
        exitstatus = _MKWAIT_EXIT(tmpcif->exitcode);
        
        break;
      }
  }
  
  lock_release(curproc->wait_child_lock);
  if(!found){
    return (ESRCH);
  }


  result = copyout((void *)&exitstatus,status,sizeof(int));
  *retval = pid;
  //c->parent=NULL;
  //proc_destroy(c);
  return(0);
}
#else
/* this implementation of sys__exit does not do anything with the exit code */
  /* this needs to be fixed to get exit() and waitpid() working properly */

void sys__exit(int exitcode) {

  struct addrspace *as;
  struct proc *p = curproc;
  /* for now, just include this to keep the compiler from complaining about
     an unused variable */
  (void)exitcode;

  DEBUG(DB_SYSCALL,"Syscall: _exit(%d)\n",exitcode);

  KASSERT(curproc->p_addrspace != NULL);
  as_deactivate();
  /*
   * clear p_addrspace before calling as_destroy. Otherwise if
   * as_destroy sleeps (which is quite possible) when we
   * come back we'll be calling as_activate on a
   * half-destroyed address space. This tends to be
   * messily fatal.
   */
  as = curproc_setas(NULL);
  as_destroy(as);

  /* detach this thread from its process */
  /* note: curproc cannot be used after this call */
  proc_remthread(curthread);

  /* if this is the last user process in the system, proc_destroy()
     will wake up the kernel menu thread */
  proc_destroy(p);
  
  thread_exit();
  /* thread_exit() does not return, so we should never get here */
  panic("return from thread_exit in sys_exit\n");
}


/* stub handler for getpid() system call                */
int
sys_getpid(pid_t *retval)
{
  /* for now, this is just a stub that always returns a PID of 1 */
  /* you need to fix this to make it work properly */
  *retval = 1;
  return(0);
}

/* stub handler for waitpid() system call                */

int
sys_waitpid(pid_t pid,
	    userptr_t status,
	    int options,
	    pid_t *retval)
{
  int exitstatus;
  int result;

  /* this is just a stub implementation that always reports an
     exit status of 0, regardless of the actual exit status of
     the specified process.   
     In fact, this will return 0 even if the specified process
     is still running, and even if it never existed in the first place.

     Fix this!
  */

  if (options != 0) {
    return(EINVAL);
  }
  /* for now, just pretend the exitstatus is 0 */
  exitstatus = 0;
  result = copyout((void *)&exitstatus,status,sizeof(int));
  if (result) {
    return(result);
  }
  *retval = pid;
  return(0);
}
#endif /* OPT_A2 */

  

