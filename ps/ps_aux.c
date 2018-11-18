#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <pwd.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

static const char HEADER[] =
    "USER\tPID\t%%CPU\t%%MEM\tVSZ\tRSS\tTTY\tSTAT\tSTART\tTIME\tCOMMAND\n";

static int ispid(const char *d_name) {
  while (*d_name) {
    if (!isdigit(*(d_name++)))
      return 0;
  }
  return 1;
}

// output data
const size_t USER_SIZE = 1024;
const size_t TTY_SIZE = 32;
const size_t START_SIZE = 6;
const size_t TIME_SIZE = 32;
const size_t COMMAND_SIZE = 1024;
struct proc {
  char user[USER_SIZE];
  int pid;
  double cpu, mem;
  unsigned long vsz;
  long rss;
  char tty[TTY_SIZE];
  char state;
  char start[START_SIZE], time[TIME_SIZE], command[COMMAND_SIZE];
};

// /proc/###/stat file data
struct proc_stat {
  int pid;
  char state;
  int tty_nr;
  unsigned long utime, stime;
  unsigned long long starttime;
  unsigned long vsize;
  long rss;
  unsigned long start_data;
};

static int get_user(const char *pid, struct proc *proc) {
  char dir[PATH_MAX];
  snprintf(dir, sizeof(dir), "/proc/%s", pid);

  struct stat dir_stat;
  if (stat(dir, &dir_stat) != 0) {
    fprintf(stderr, "Failed to get %s stat: %s\n", dir, strerror(errno));
    int ret = errno;
    errno = 0;
    return ret;
  }

  struct passwd *pwd;
  if ((pwd = getpwuid(dir_stat.st_uid)) == NULL) {
    fprintf(stderr, "Failed to get passwd for %s owner: %s\n", dir,
            strerror(errno));
    int ret = errno;
    errno = 0;
    return ret;
  }
  strncpy(proc->user, pwd->pw_name, USER_SIZE);

  return 0;
}

static int scanf_from_file(const char *file, const char *fmt, const int count,
                           ...) {
  int ret = 0;
  int fd = -1;
  if ((fd = open(file, O_RDONLY)) < 0) {
    fprintf(stderr, "Failed to open %s for read: %s\n", file, strerror(errno));
    ret = errno;
    errno = 0;
    goto scanf_from_file_cleanup;
  }

  static const size_t BUF_SIZE = 1024;
  char buf[BUF_SIZE];
  ssize_t rd = 0;
  if ((rd = read(fd, (void *)buf, BUF_SIZE)) < 0) {
    fprintf(stderr, "Failed to read from %s: %s\n", file, strerror(errno));
    ret = errno;
    errno = 0;
    goto scanf_from_file_cleanup;
  }
  rd = (rd == BUF_SIZE) ? rd - 1 : rd;
  buf[rd] = '\0';

  va_list args;
  va_start(args, count);
  if (vsscanf(buf, fmt, args) < count) {
    fprintf(stderr, "Wrong format of %s: %s\n", file, strerror(errno));
    ret = errno;
    errno = 0;
    goto scanf_from_file_cleanup;
  }

scanf_from_file_cleanup:
  if (fd >= 0)
    close(fd);

  return ret;
}

static int get_proc_stat(const char *pid, struct proc_stat *stat) {
  int ret = 0;

  char statfile[PATH_MAX];
  snprintf(statfile, sizeof(statfile), "/proc/%s/stat", pid);
  if ((ret = scanf_from_file(
           statfile,
           "%d %*s %c %*d %*d %*d %d %*d %*u %*lu %*lu %*lu %*lu %lu %lu "
           "%*ld %*ld %*ld %*ld %*ld %*ld %llu %lu %ld",
           8, &stat->pid, &stat->state, &stat->tty_nr, &stat->utime,
           &stat->stime, &stat->starttime, &stat->vsize, &stat->rss)) != 0)
    return ret;

  return 0;
}

static int get_uptime(int *uptime) {
  int ret = 0;
  static const char uptimefile[] = "/proc/uptime";
  if ((ret = scanf_from_file(uptimefile, "%d", 1, uptime)) != 0)
    return ret;

  return 0;
}

static int get_cpu_and_time(const struct proc_stat *proc_stat,
                            struct proc *proc) {
  int ret = 0;
  unsigned long long total_ticks = proc_stat->utime + proc_stat->stime;
  int uptime = 0;
  unsigned long hz = sysconf(_SC_CLK_TCK);

  if ((ret = get_uptime(&uptime)) != 0)
    return ret;

  proc->cpu =
      (unsigned long long)uptime > proc_stat->starttime
          ? 100.0 * total_ticks / (hz * (uptime - proc_stat->starttime / hz))
          : 0;

  unsigned long long seconds = total_ticks / hz;
  snprintf(proc->time, TIME_SIZE, "%llu:%llu", seconds / 60, seconds % 60);

  return 0;
}

static int get_mem(struct proc_stat *proc_stat, struct proc *proc) {
  int ret = 0;

  static const char meminfofile[] = "/proc/meminfo";
  unsigned long total_mem = 0; // total usable mem in kB
  if ((ret = scanf_from_file(meminfofile, "%*s %lu", 1, &total_mem)) != 0)
    return ret;

  long pagesize = sysconf(_SC_PAGESIZE);
  long rss_kb = (proc_stat->rss * pagesize) / 1024L;
  proc->mem = 100L * rss_kb / total_mem;
  proc->rss = rss_kb;
  proc->vsz = proc_stat->vsize / 1024UL;

  return 0;
}

static int get_started_time(const char *started, struct proc *proc) {
  strncpy(proc->start, started + 11, START_SIZE);
  proc->start[START_SIZE - 1] = '\0';
  return 0;
}

static int get_started_date(const char *started, struct proc *proc) {
  strncpy(proc->start, started + 4, 3);     // mmm
  strncpy(proc->start + 3, started + 8, 2); // dd
  proc->start[START_SIZE - 1] = '\0';
  return 0;
}

static int get_started(const struct proc_stat *proc_stat, struct proc *proc) {
  int ret = 0;
  time_t curtime = time(NULL);
  int uptime = 0;
  if ((ret = get_uptime(&uptime)) != 0)
    return ret;

  time_t sys_starttime = curtime - uptime;
  time_t proc_starttime = sys_starttime + proc_stat->starttime;
  char *started = ctime(&proc_starttime);

  static const time_t SECONDS_IN_DAY = 24 * 60 * 60;
  if (curtime - proc_starttime > SECONDS_IN_DAY)
    ret = get_started_date(started, proc);
  else
    ret = get_started_time(started, proc);

  return 0;
}

static int get_stat(const char *pid, struct proc *proc) {
  int ret = 0;

  struct proc_stat proc_stat;
  if ((ret = get_proc_stat(pid, &proc_stat)) != 0)
    return ret;

  if ((ret = get_cpu_and_time(&proc_stat, proc)) !=
      0) // TIME is accumulated cpu time, user + system
    return ret;

  if ((ret = get_mem(&proc_stat, proc)) != 0)
    return ret;

  if ((ret = get_started(&proc_stat, proc)) != 0)
    return ret;

  proc->pid = proc_stat.pid;
  proc->state = proc_stat.state;
  snprintf(proc->tty, TTY_SIZE, "tty/%d", proc_stat.tty_nr);

  return 0;
}

static int get_command(const char *pid, struct proc *proc) {
  int ret = 0;
  char cmdline[PATH_MAX];
  snprintf(cmdline, sizeof(cmdline), "/proc/%s/cmdline", pid);
  if ((ret = scanf_from_file(cmdline, "%[^\n]", 1, proc->command)) != 0)
    return ret;

  return 0;
}

static int get_proc(const char *pid, struct proc *proc) {
  int ret = 0;

  if ((ret = get_user(pid, proc)) != 0)
    return ret;

  if ((ret = get_stat(pid, proc)) != 0)
    return ret;

  if ((ret = get_command(pid, proc)) != 0)
    return ret;

  return 0;
}

static void print_proc(const struct proc *proc) {
  printf("%s\t%d\t%.1f\t%.1f\t%lu\t%ld\t%s\t%c\t%s\t%s\t%s\n", proc->user,
         proc->pid, proc->cpu, proc->mem, proc->vsz, proc->rss, proc->tty,
         proc->state, proc->start, proc->time, proc->command);
}

static int report_process(const char *pid) {
  int ret = 0;

  struct proc proc;
  if ((ret = get_proc(pid, &proc)) != 0)
    return ret;

  print_proc(&proc);

  return 0;
}

int main(void) {
  int ret = 0;
  DIR *proc = NULL;
  struct dirent *dir = NULL;

  printf(HEADER);

  if ((proc = opendir("/proc")) == NULL) {
    ret = errno;
    errno = 0;
    fprintf(stderr, "Failed to open /proc: %s\n", strerror(ret));
    goto cleanup;
  }

  while ((dir = readdir(proc)) != NULL) {
    const char *d_name = dir->d_name;
    if (ispid(d_name)) {
      if ((ret = report_process(d_name)) != 0)
        goto cleanup;
    }
  }

cleanup:
  if (proc)
    closedir(proc);

  return ret;
}

