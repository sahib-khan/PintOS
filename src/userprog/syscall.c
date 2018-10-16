#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"

#include "filesys/filesys.h"
#include "filesys/file.h"
#include "threads/init.h"
#include <string.h>
#include "threads/synch.h"

static void syscall_handler (struct intr_frame *);

#define TOTAL_ACTIONS 13

typedef struct action{
  int argc;
  int (*function) ();
  void (*func) ();
}action;

static void validate (const int *ptr)
{
  uint32_t *pd = thread_current ()->pagedir;
  if ( ptr == NULL || !is_user_vaddr (ptr) || pagedir_get_page (pd, ptr) == NULL)
  {
    exit (-1);
  }
}

static int
is_valid_fd (int fd)
{
  return fd >= 0 && fd < MAX_FILES; 
}

static void halt(){
  power_off();
}

void exit (int status){
  char *name = thread_current ()->name, *save;
  name = strtok_r (name, " ", &save);

  printf ("%s: exit(%d)\n", name, status); 
  thread_exit ();	
}


static int exec (const char *cmd_line){
  validate(cmd_line);
  thread_exit();
  return -1;
}


static int wait (int pid){

  return -1;
}


static int create (const char *file, unsigned initial_size){
	validate(file);
  int status = filesys_create (file, initial_size);
  return status;
}


static int remove (const char *file){
	validate(file);
	int status = filesys_remove(file);
	return status;
}


static int open (const char *file){
  validate(file);
  struct file* f = filesys_open (file);
  if(f==NULL){
  	return -1;
  }
  
  struct thread *t = thread_current ();

  int i;
  for (i = 2; i<MAX_FILES; i++)
  {
    if (t->files[i] == NULL){
      t->files[i] = f;
      break;
    }
  }

  if (i == MAX_FILES)
    return -1;
  else
    return i;

}


static int filesize (int fd){
  struct thread *t = thread_current ();

  if (is_valid_fd (fd) && t->files[fd] != NULL)
  {  

    int size = file_length (t->files[fd]);
    return size;
  }
  return -1;
}


static int read (int fd, void *buffer, unsigned size){

  validate(buffer);
  validate(buffer+size);

  struct file *f;
  int ret=-1;

  if(fd==0)
  {
    int i;
    for(i=0;i<size;i++)
    {
      *(uint8_t *)(buffer + i) = input_getc ();
    }
    ret = size;
  }
  else if(fd > 1 && fd<128 ) 
  {
      f = thread_current()->files[fd];
      if (f)
        ret = file_read (f, buffer, size);
  }

  return ret;
}

static int write (int fd, const void *buffer, unsigned size){
 
 struct file *f;  
  int ret;
  ret = 0;
  
  validate(buffer);
  validate(buffer+size);

  if (fd == 1){
    ret = size; 
    putbuf ((char *)buffer, size);
  }
  else if(fd > 1 && fd<128 ) 
  {
    f = thread_current()->files[fd];
    if (f)
      ret = file_write (f, buffer, size);
  }

  return ret;  
}


static void seek (int fd, unsigned position){
  
  struct thread *t = thread_current ();
  if (is_valid_fd (fd) && t->files[fd] != NULL)
  {
    file_seek (t->files[fd], position);
  }
}


static int tell (int fd){
  
  struct thread *t = thread_current ();

  if (is_valid_fd (fd) && t->files[fd] != NULL)
  {  
    int position = file_tell (t->files[fd]);
    return position;
  }
  return -1;
}

static void close (int fd){
 	
 	if (is_valid_fd (fd))
   {
  	struct thread *t = thread_current ();
  	if (t->files[fd] != NULL)
  	{  
	    file_close (t->files[fd]);
    	t->files[fd] = NULL;
  	}
  }
}

static const action actions[]={
  {0, NULL, halt},
  {1, NULL, exit},
  {1, exec, NULL},
  {1, wait, NULL},
  {2, create, NULL},
  {1, remove, NULL},
  {1, open, NULL},
  {1, filesize, NULL},
  {3, read, NULL},
  {3, write, NULL},
  {2, NULL, seek},
  {1, tell, NULL},
  {1 ,NULL, close}
};

void
syscall_init (void) 
{
    intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void
syscall_handler (struct intr_frame *f) 
{
  int* esp = f->esp;

  validate(esp);

  int syscall_num = *esp;
  esp+= sizeof(void);

  if(syscall_num>=0 && syscall_num<TOTAL_ACTIONS){
    action sys_to_be_called = actions[syscall_num]; 
    int arg_num = sys_to_be_called.argc;

    validate(esp+arg_num);

    if(sys_to_be_called.function==NULL){
	    if(arg_num==0){
	      sys_to_be_called.func();
	    }
	    if(arg_num==1){
	      sys_to_be_called.func(*(esp));
	    }
	    if(arg_num==2){
	      sys_to_be_called.func(*(esp), *(esp+1));
	    }
	    if(arg_num==3){
	      sys_to_be_called.func(*(esp), *(esp+1), *(esp+2));
	    }
	    esp += arg_num;
    }
    else{
	    int ret;
	    if(arg_num==0){
	      ret = sys_to_be_called.function();
	    }
	    if(arg_num==1){
	      ret = sys_to_be_called.function(*(esp));
	    }
	    if(arg_num==2){
	      ret = sys_to_be_called.function(*(esp), *(esp+1));
	    }
	    if(arg_num==3){
	      ret = sys_to_be_called.function(*(esp), *(esp+1), *(esp+2));
	    }
	    f->eax = ret;
	    esp += arg_num;
	  }
	}
}
