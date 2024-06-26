#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/queue.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

typedef uint32_t u32;
typedef int32_t i32;

struct process
{
  u32 pid;
  u32 arrival_time;
  u32 burst_time;

  TAILQ_ENTRY(process) pointers;

  /* Additional fields here */
  bool arrived_yet;
  u32 time_remaining;
  /* End of "Additional fields here" */
};

TAILQ_HEAD(process_list, process);

u32 next_int(const char **data, const char *data_end)
{
  u32 current = 0;
  bool started = false;
  while (*data != data_end)
  {
    char c = **data;

    if (c < 0x30 || c > 0x39)
    {
      if (started)
      {
        return current;
      }
    }
    else
    {
      if (!started)
      {
        current = (c - 0x30);
        started = true;
      }
      else
      {
        current *= 10;
        current += (c - 0x30);
      }
    }

    ++(*data);
  }

  printf("Reached end of file while looking for another integer\n");
  exit(EINVAL);
}

u32 next_int_from_c_str(const char *data)
{
  char c;
  u32 i = 0;
  u32 current = 0;
  bool started = false;
  while ((c = data[i++]))
  {
    if (c < 0x30 || c > 0x39)
    {
      exit(EINVAL);
    }
    if (!started)
    {
      current = (c - 0x30);
      started = true;
    }
    else
    {
      current *= 10;
      current += (c - 0x30);
    }
  }
  return current;
}

void init_processes(const char *path,
                    struct process **process_data,
                    u32 *process_size)
{
  int fd = open(path, O_RDONLY);
  if (fd == -1)
  {
    int err = errno;
    perror("open");
    exit(err);
  }

  struct stat st;
  if (fstat(fd, &st) == -1)
  {
    int err = errno;
    perror("stat");
    exit(err);
  }

  u32 size = st.st_size;
  const char *data_start = mmap(NULL, size, PROT_READ, MAP_PRIVATE, fd, 0);
  if (data_start == MAP_FAILED)
  {
    int err = errno;
    perror("mmap");
    exit(err);
  }

  const char *data_end = data_start + size;
  const char *data = data_start;

  *process_size = next_int(&data, data_end);

  *process_data = calloc(sizeof(struct process), *process_size);
  if (*process_data == NULL)
  {
    int err = errno;
    perror("calloc");
    exit(err);
  }

  for (u32 i = 0; i < *process_size; ++i)
  {
    (*process_data)[i].pid = next_int(&data, data_end);
    (*process_data)[i].arrival_time = next_int(&data, data_end);
    (*process_data)[i].burst_time = next_int(&data, data_end);
  }

  munmap((void *)data, size);
  close(fd);
}

// tried many times to do this evaluation directly within the loop in main, would never work so
// moved to helper function
bool processes_complete_helper(struct process *data, u32 size) {
  struct process *curr_proc;
  for (int i = 0; i < size; i++) {
    curr_proc = &data[i];
    if (curr_proc->time_remaining > 0) {
      return false;
    }
  }
  return true;
}

int main(int argc, char *argv[])
{
  if (argc != 3)
  {
    return EINVAL;
  }
  struct process *data;
  u32 size;
  init_processes(argv[1], &data, &size);

  u32 quantum_length = next_int_from_c_str(argv[2]);

  struct process_list list;
  TAILQ_INIT(&list);

  u32 total_waiting_time = 0;
  u32 total_response_time = 0;

  /* Your code here */

  u32 curr_time = data[0].arrival_time;

  struct process *curr_proc;

  // didn't want to modify init_processes, so initialize each arrived_yet bool
  // and set time_remaining
  // also use this loop to figure out earliest arrival time and start curr_time
  // at that
  for (int i = 0; i < size; i++) {
    curr_proc = &data[i];
    curr_proc->arrived_yet = false;
    curr_proc->time_remaining = curr_proc->burst_time;
    if (curr_proc->arrival_time < curr_time) {
      curr_time = curr_proc->arrival_time;
    }
  }
  u32 time_within_slice = 1;
  u32 elapsed_time = curr_time;
  bool processes_complete = false;

  struct process * arriving_proc;
  while (!processes_complete) {
    // add any newly arriving processes to the end of the queue
    for (int i = 0; i < size; i++) {
      arriving_proc = &data[i];
      if (arriving_proc->arrival_time == curr_time) {
        TAILQ_INSERT_TAIL(&list, arriving_proc, pointers);
      }
    }

    // if a process's time slice is expiring, add it to the end of the queue
    if (curr_proc && time_within_slice == quantum_length + 1 && curr_proc->time_remaining > 0) {
      TAILQ_INSERT_TAIL(&list, curr_proc, pointers);
      time_within_slice = 1;
    }

    // if a new time slice has started, the front of the queue is now curr_proc
    if (curr_proc && time_within_slice == 1) {
      if (TAILQ_EMPTY(&list))
        return -1;
      curr_proc = TAILQ_FIRST(&list);
      TAILQ_REMOVE(&list, curr_proc, pointers);
    }

    // if this process is just arriving, calculate its response time and add it to the total
    if (curr_proc && !curr_proc->arrived_yet) {
      curr_proc->arrived_yet = true;
      total_response_time += (elapsed_time - curr_proc->arrival_time);
    }

    // if the time slice is not over, do one time-unit's work on the process
    if (curr_proc && time_within_slice <= quantum_length) {
      // decrease time remaining if not done yet
      if (curr_proc->time_remaining > 0) {
        curr_proc->time_remaining--;
        elapsed_time++;
      }
      // if done, we can calculate wait time
      if (curr_proc->time_remaining == 0) {
        total_waiting_time += (elapsed_time - curr_proc->arrival_time - curr_proc->burst_time);
        time_within_slice = 0;
      }
    }
    time_within_slice++;
    curr_time++;

    // re-evaluate whether processes are complete:
    processes_complete = processes_complete_helper(data, size);
  }
    
  
  /* End of "Your code here" */

  printf("Average waiting time: %.2f\n", (float)total_waiting_time / (float)size);
  printf("Average response time: %.2f\n", (float)total_response_time / (float)size);

  free(data);
  return 0;
}
