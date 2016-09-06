#include <sys/ptrace.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <err.h>
#include <sys/user.h>
#include <asm/ptrace.h>
#include <sys/wait.h>
#include <asm/unistd.h>
#include <signal.h>
#include <string.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/types.h>
#include <pwd.h>

struct sandbox {
  pid_t child;
  const char *progname;
};

struct sandb_syscall {
  int syscall;
  void (*callback)(struct sandbox*, struct user_regs_struct *regs);
};

struct sandb_syscall sandb_syscalls[] = {
  {__NR_read,            NULL},
  {__NR_write,           NULL},
  {__NR_exit,            NULL},
  {__NR_brk,             NULL},
  {__NR_mmap,            NULL},
  {__NR_access,          NULL},
  {__NR_open,            NULL},
  {__NR_fstat,           NULL},
  {__NR_close,           NULL},
  {__NR_mprotect,        NULL},
  {__NR_munmap,          NULL},
  {__NR_arch_prctl,      NULL},
  {__NR_exit_group,      NULL},
  {__NR_getdents,        NULL},
};

void sandb_kill(struct sandbox *sandb) {
  kill(sandb->child, SIGKILL);
  wait(NULL);
  exit(EXIT_FAILURE);
}

void sandb_handle_syscall(struct sandbox *sandb) {
  int i;
  struct user_regs_struct regs;
  int syscall;
  if(ptrace(PTRACE_GETREGS, sandb->child, NULL, &regs) < 0)
    err(EXIT_FAILURE, "[SANDBOX] Failed to PTRACE_GETREGS:");
  syscall=regs.orig_rax;
  if(syscall == __NR_open) {
	//printf("%s", syscall_names[syscall-1]); /* System call name */
	printf("%llu ", regs.rax); /* Address of the path */
	printf("%llu ", regs.rcx); /* Flag */
	printf("%llu\n ", regs.rdx); /* Mode */
  }
  return;
   
  
  /*
  if(regs.orig_rax == -1) {
    printf("[SANDBOX] Segfault ?! KILLING !!!\n");
  } else {
    printf("[SANDBOX] Trying to use devil syscall (%llu) ?!? KILLING !!!\n", regs.orig_rax);
  }
  sandb_kill(sandb);*/
}

void sandb_init(struct sandbox *sandb, int argc, char **argv) {
  pid_t pid;
 
  pid = fork();
  if(pid == -1)
    err(EXIT_FAILURE, "[SANDBOX] Error on fork:");
   if(pid == 0) {
	
    if(ptrace(PTRACE_TRACEME, 0, NULL, NULL) < 0)
      err(EXIT_FAILURE, "[SANDBOX] Failed to PTRACE_TRACEME:");
	
    if(execvp(argv[0], argv) < 0)
      err(EXIT_FAILURE, "[SANDBOX] Failed to execv:");

  } else {
	
    sandb->child = pid;
    sandb->progname = argv[0];
    wait(NULL);
  }
}

void sandb_run(struct sandbox *sandb) {
  int status;

  if(ptrace(PTRACE_SYSCALL, sandb->child, NULL, NULL) < 0) {
    if(errno == ESRCH) {
      waitpid(sandb->child, &status, __WALL | WNOHANG);
      sandb_kill(sandb);
    } else {
      err(EXIT_FAILURE, "[SANDBOX] Failed to PTRACE_SYSCALL:");
    }
  }

  wait(&status);

  if(WIFEXITED(status))
    exit(EXIT_SUCCESS);

  if(WIFSTOPPED(status)) {
    sandb_handle_syscall(sandb);
  }
}

int main(int argc, char **argv) {
  struct sandbox sandb;
  int i;
  FILE *fp;
  char ch;
  
  if ( argc >1 && strcmp(argv[1], "-c") == 0 )  /* Process optional arguments. */
  {
	fp = fopen(argv[2], "r");
  }
  else
  {
	fp = fopen(".fendrc", "r");
	if(fp==NULL)
	{
		struct passwd *pw = getpwuid(getuid());
		//printf("%s",strcat(pw->pw_dir,"/.fendrc"));
		fp = fopen(strcat(pw->pw_dir,"/.fendrc"),"r");
		if(fp==NULL)
		{
			printf("Must provide a config file.\n");
			exit(EXIT_FAILURE);
		}
	}
  }

  
  /*
  if(argc < 2) {
    errx(EXIT_FAILURE, "[SANDBOX] Usage : %s <elf> [<arg1...>]", argv[0]);
  }
 */
  //Have to change this call if -c is used
  
  
  sandb_init(&sandb, argc-1, argv+1);

  for(;;) {
    sandb_run(&sandb);
	
  }
  

  
  return EXIT_SUCCESS;
}