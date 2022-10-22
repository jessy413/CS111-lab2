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

struct process {
  u32 pid;
  u32 arrival_time;
  u32 burst_time;

  /* Additional fields here*/
  u32 remaining_time;
  u32 start_exec_time;
  u32 waiting_time;
  u32 response_time;
  bool start;
  /* End of "Addtional fields here" */

  TAILQ_ENTRY(process) pointers;

};

TAILQ_HEAD(process_list, process);

u32 next_int(const char **data, const char *data_end) {
  u32 current = 0;
  bool started = false;
  while (*data != data_end) {
    char c = **data;

    if (c < 0x30 || c > 0x39) {
      if (started) {
	return current;
      }
    }
    else {
      if (!started) {
	current = (c - 0x30);
	started = true;
      }
      else {
	current *= 10;
	current += (c - 0x30);
      }
    }

    ++(*data);
  }

  printf("Reached end of file while looking for another integer\n");
  exit(EINVAL);
}

u32 next_int_from_c_str(const char *data) {
  char c;
  u32 i = 0;
  u32 current = 0;
  bool started = false;
  while ((c = data[i++])) {
    if (c < 0x30 || c > 0x39) {
      exit(EINVAL);
    }
    if (!started) {
      current = (c - 0x30);
      started = true;
    }
    else {
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
  if (fd == -1) {
    int err = errno;
    perror("open");
    exit(err);
  }

  struct stat st;
  if (fstat(fd, &st) == -1) {
    int err = errno;
    perror("stat");
    exit(err);
  }

  u32 size = st.st_size;
  const char *data_start = mmap(NULL, size, PROT_READ, MAP_PRIVATE, fd, 0);
  if (data_start == MAP_FAILED) {
    int err = errno;
    perror("mmap");
    exit(err);
  }

  const char *data_end = data_start + size;
  const char *data = data_start;
  

  *process_size = next_int(&data, data_end);

  *process_data = calloc(sizeof(struct process), *process_size);
  if (*process_data == NULL) {
    int err = errno;
    perror("calloc");
    exit(err);
  }

  for (u32 i = 0; i < *process_size; ++i) {
    (*process_data)[i].pid = next_int(&data, data_end);
    (*process_data)[i].arrival_time = next_int(&data, data_end);
    (*process_data)[i].burst_time = next_int(&data, data_end);
  }
  
  munmap((void *)data, size);
  close(fd);
}

int main(int argc, char *argv[])
{
  if (argc != 3) {
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

  // if no processes, print and return
  if(size==0)
  {
	  printf("Average waiting time: 0.00\n");
	  printf("Average response time: 0.00\n");
	  return 0;
  }
  
  struct process *current;
  u32 t = data[0].arrival_time;
  u32 done = 0;
  //find the earliest arrival time
  //set remaining time to burst time
  for(u32 i = 0; i<size; i++)
  {
	if(data[i].arrival_time<t)
		t = data[i].arrival_time;
	data[i].remaining_time = data[i].burst_time;
  }
  for(u32 i = 0; i<size; i++)
  {
	if(data[i].arrival_time==t)
		TAILQ_INSERT_TAIL(&list,&data[i],pointers);
  }
  while(done<size)
  {
	while(TAILQ_EMPTY(&list))
	{
		t++;
		for(u32 i = 0; i<size; i++)
		{
			if(data[i].arrival_time == t)
				TAILQ_INSERT_TAIL(&list,&data[i],pointers);
		}
	}
	current = TAILQ_FIRST(&list);
	TAILQ_REMOVE(&list,current,pointers);
	if(current->remaining_time == 0)
	{
		TAILQ_INSERT_TAIL(&list,current,pointers);
		continue;
	}

	if(current->start==false)
	{
		current->start_exec_time = t;
		current->response_time = t-current->arrival_time;
		current->start = true;
	}
	u32 time = quantum_length;
	if(current->remaining_time < quantum_length)
		time = current->remaining_time;
	while(time>0)
	{
		current->remaining_time--;
		time--;
		t++;
		for(u32 i = 0; i<size;i++)
		{
			if(data[i].arrival_time==t)
				TAILQ_INSERT_TAIL(&list,&data[i],pointers);
		}
	}
	if(current->remaining_time==0)
	{
		current->waiting_time = t-current->arrival_time-current->burst_time;
		done++;
	}

	TAILQ_INSERT_TAIL(&list, current,pointers);

  }

  
  // for each process
  // add waiting_time and response time to total
  struct process *current_process;
  TAILQ_FOREACH(current_process,&list,pointers)
  {	
	  
	  total_waiting_time += current_process->waiting_time;
	  total_response_time += current_process->response_time;
  }
  
 /* End of "Your code here" */

 printf("Average waiting time: %.2f\n", (float) total_waiting_time / (float) size);
  printf("Average response time: %.2f\n", (float) total_response_time / (float) size);

  free(data);
  return 0;
}
