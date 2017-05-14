// compile with:
// gcc -std=c99 -o test9 -framework OpenCL test9.c 

#include <stdio.h>
#include <ctype.h>
#include <sys/poll.h>
#include <OpenCL/opencl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "EasyOpenCL.h"
#include "lo/lo.h"

lo_server_thread serverthread;
volatile int osc1_var = 1;
volatile int osc2_var = 1;

int cleanup_var = 1;
char *data;
cl_device_id device_id;
cl_context context;
cl_command_queue commands;
cl_program program;
cl_kernel kernel;
size_t global;
size_t local;

unsigned int count; // count for input/outputs and file descriptors
cl_mem *ios; // input/outputs
cl_mem *ios_size; // input/outputs
cl_mem *ios_offset; // input/outputs
FILE **fds; // file descriptors
int *szs; // time delay buffer size
int *ofs; // time delay buffer size
int *vid; // is it a video

unsigned int input_count = 0;
cl_mem* input_ios;
cl_mem* input_ios_size;
cl_mem* input_ios_offset;
FILE** input_fds;
int* input_szs;
int* input_ofs;
int *input_vid;

unsigned int output_count = 0;
cl_mem output_io;
cl_mem output_io_size;
cl_mem output_io_offset;
FILE* output_fd;
int output_sz;
int output_of;

void error(int num, const char *m, const char *path);

int osc1_handler(const char *path, const char *types, lo_arg ** argv,
                int argc, void *data, void *user_data);

int osc2_handler(const char *path, const char *types, lo_arg ** argv,
                int argc, void *data, void *user_data);

int osc3_handler(const char *path, const char *types, lo_arg ** argv,
                int argc, void *data, void *user_data);

void error(int num, const char *msg, const char *path)
{
    printf("liblo server error %d in path %s: %s\n", num, path, msg);
    fflush(stdout);
}

int osc1_handler(const char *path, const char *types, lo_arg ** argv,
                int argc, void *data, void *user_data)
{
    /* example showing pulling the argument values out of the argv array */
  printf("%s <- i:%d\n", path, argv[0]->i);
  fflush(stdout);
  osc1_var = argv[0]->i;
  return 0;
}

int osc2_handler(const char *path, const char *types, lo_arg ** argv,
                int argc, void *data, void *user_data)
{
    /* example showing pulling the argument values out of the argv array */
  printf("%s <- i:%d\n", path, argv[0]->i);
  fflush(stdout);
  osc2_var = argv[0]->i;
  return 0;
}

int osc3_handler(const char *path, const char *types, lo_arg ** argv,
                int argc, void *data, void *user_data)
{
    /* example showing pulling the argument values out of the argv array */
  long datasize = (480 * 360 * 3) / 2;
  long input_loc = argv[0]->i;
  long input_safe = (input_loc/ datasize) * datasize;
  printf("%s <- i:%d\n", path, input_safe);
  fflush(stdout);
  // input_fds[1]
  fseek(input_fds[0], input_safe, SEEK_SET);
  return 0;
}

void* safeMalloc(unsigned int n) {
  char* mem = malloc(n);
  if (mem == NULL) {
    fprintf(stderr, "Error: Failed to allocate memory.\n");
    exit(EXIT_FAILURE);
  }
  return mem;
}

char* slurp(const char* filename) {
  FILE *fd = fopen(filename, "r");
  char *src;
  long len;
  if (fd != 0) {
    fseek(fd, 0, SEEK_END);
    len = ftell(fd);
    fseek(fd, 0, SEEK_SET);
    src = safeMalloc(len);
    fread(src, 1, len, fd);
    fclose(fd);
  }
  else {
    fprintf(stderr, "Error: Failed to open file \"%s\".\n", filename);
    exit(EXIT_FAILURE);
  }
  return src;
}

void cleanup(int signo) {
  cleanup_var = 0;
}

void real_cleanup() {
  for (int i = 0; i < count; i++) {
      clReleaseMemObject(ios[i]);
  }
  for (int i = 0; i < count; i++) {
    pclose(fds[i]);
  }
  free(data);
  free(ios);
  free(ios_size);
  free(fds);
  free(szs);
  free(input_ios);
  free(input_ios_size);
  free(input_fds);
  free(input_szs);
  clReleaseProgram(program);
  clReleaseKernel(kernel);
  clReleaseCommandQueue(commands);
  clReleaseContext(context);
  lo_server_thread_free(serverthread);
  printf("Exited successfully.\n");
}

void setios(char* script[][4], unsigned int script_count) {
  ios =      (cl_mem*) safeMalloc(sizeof(cl_mem) * script_count);
  ios_size = (cl_mem*) safeMalloc(sizeof(cl_mem) * script_count);
  ios_offset = (cl_mem*) safeMalloc(sizeof(cl_mem) * script_count);
  szs = (int*) safeMalloc(sizeof(int) * script_count);
  ofs = (int*) safeMalloc(sizeof(int) * script_count);
  fds = (FILE**) safeMalloc(sizeof(FILE*) * script_count);
  vid = (int*) safeMalloc(sizeof(int*) * script_count);
  
  for (int i = 0; i < script_count; i++) {
    int t_size = atoi(script[i][3]);
    if (t_size > 0) {
      ios[i] = getMemory(script[i][2], context, global * t_size * sizeof(char));
      ios_size[i] = getMemory(script[i][2], context, sizeof(int));
      ios_offset[i] = getMemory(script[i][2], context, sizeof(int));

      if (script[i][2][0] == 'r') {
	fds[i] = fopen(script[i][0], script[i][2]); // inputs
      }
      else
      {
	fds[i] = popen(script[i][0], script[i][2]); // outputs
      }

      szs[i] = t_size;
      ofs[i] = 0;

      if (script[i][1][0] == 'v' && script[i][1][1] == 0) {
	vid[i] = 1;
      }
      else if (script[i][1][0] == 'i' && script[i][1][1] == 0) {
	vid[i] = 0;
      }
      else {
	fprintf(stderr, "Error: \"%s\" is not a valid type.\n", script[i][1]);
	exit(EXIT_FAILURE);
      }
      
      setKernelArg(kernel, i * 3, sizeof(cl_mem), &ios[i]);
      setKernelArg(kernel, (i * 3) + 1, sizeof(cl_mem), &ios_offset[i]);
      setKernelArg(kernel, (i * 3) + 2, sizeof(cl_mem), &ios_size[i]);
    }
    else {
      fprintf(stderr, "Error: Invalid time delay value: %s\n", script[i][3]);
      exit(EXIT_FAILURE);
    }
  }
}

void startup(size_t datasize, char* filename, char* script[][4], unsigned int script_count) {
  char *source;
  source = slurp(filename);
  device_id = getDeviceID();
  context = getContext(&device_id);
  commands = getCommands(context, device_id);
  program = getProgram(context, device_id, &source);
  kernel = getKernel(program, "cl_main");
  global = getGlobal(datasize);
  local  = getLocal(kernel, device_id);
  setios(script, script_count);
  count = script_count;
  free(source);
  signal(SIGPIPE, cleanup);
}

unsigned int countLines(char* sourcecode) {
  char c;
  unsigned int lines = 1;
  for (int i = 0; (c = sourcecode[i]) != 0; i++) {
    if (c == 10) {
      sourcecode[i] = 0;
      lines++;
    }
  }
  return lines;
}

unsigned int nextLine(char* str, unsigned int offset) {
  unsigned int i = 0;
  while (1) {
    char c = str[offset + i];
    if ((c == 10) || c == 0) break;
    i++;
  }
  return offset + i + 1; // new offset
}

unsigned int nextWord(char* str, unsigned int offset) {
  unsigned int i = 0;
  while (1) {
    char c = str[offset + i];
    if ((c == 32) || (c == 10) || (c == 0)) break;
    i++;
  }
  return offset + i + 1; // new offset
}

void eval(char* source) {
  printf("line: %s\n",  source);
}

void config(char* filename) {
  char *source = slurp(filename);
  unsigned int lines = countLines(source);
  for (int i = 0; i < lines; i++) {
    int offset = (i == 0) ? 0 : nextLine(source, offset);
    eval(source + offset);
  }
  free(source);
}

int main(int argc, char** argv) {
  // OSC stuff
  // start a new server on port 7770
  serverthread = lo_server_thread_new("7772", error);

  // add method that will match the path /foo/bar, with two numbers, coerced
  // to float and int
  lo_server_thread_add_method(serverthread, "/osc1", "i", osc1_handler, NULL);
  lo_server_thread_add_method(serverthread, "/osc2", "i", osc2_handler, NULL);
  lo_server_thread_add_method(serverthread, "/osc3", "i", osc3_handler, NULL);
  
  // start
  lo_server_thread_start(serverthread);


  // Configure (get this information from a configuration script)
  //
  char* ProgramName = (argc > 1) ? argv[1] : "";
  config(ProgramName);
  
  size_t x_size = 480;
  size_t y_size = 360;
  char* sourcefilename = "test.cl";
  unsigned int script_count = 2;
  char* script[][4] = {{"IntimateAddictions.yuv", "v", "r", "90"},
		       {"ffplay -f rawvideo -vcodec rawvideo -pixel_format yuv420p -video_size 480x360 -i pipe:0", "v", "w", "60"}};
  
  size_t resolution = x_size * y_size;
  size_t datasize = (resolution * 3) / 2;
  startup(datasize, sourcefilename, script, script_count);

  // add more OSC args here
  // (char* permissions, cl_context context, size_t size)
  cl_mem osc1 = getMemory("r", context, sizeof(int*));
  cl_mem osc2 = getMemory("r", context, sizeof(int*));
  setKernelArg(kernel, (script_count * 3), sizeof(cl_mem), &osc1);
  setKernelArg(kernel, (script_count * 3) + 1, sizeof(cl_mem), &osc2);
  
  input_ios        = (cl_mem*) safeMalloc(sizeof(cl_mem) * script_count);
  input_ios_size   = (cl_mem*) safeMalloc(sizeof(cl_mem) * script_count);
  input_ios_offset = (cl_mem*) safeMalloc(sizeof(cl_mem) * script_count);
  input_fds        = (FILE**)  safeMalloc(sizeof(FILE*)  * script_count);
  input_szs        = (int*)    safeMalloc(sizeof(int)    * script_count);
  input_ofs        = (int*)    safeMalloc(sizeof(int)    * script_count);
  input_vid        = (int*)    safeMalloc(sizeof(int)    * script_count);
  
  for (int i = 0; i < script_count; i++) {
    if (script[i][2][0] == 'r') {
      input_ios[input_count] = ios[i];
      input_ios_size[input_count] = ios_size[i];
      input_ios_offset[input_count] = ios_offset[i];
      input_fds[input_count] = fds[i];
      input_szs[input_count] = szs[i];
      input_ofs[input_count] = ofs[i];
      input_vid[input_count] = vid[i];
      input_count++;
    }
    else if (script[i][2][0] == 'w') {
      output_io = ios[i];
      output_io_size = ios_size[i];
      output_io_offset = ios_offset[i];
      output_fd = fds[i];
      output_sz = szs[i];
      output_of = ofs[i];
      output_count++;
    }
  }	
  
  if (output_count > 1) {
    fprintf(stderr, "Error: More than one (%d) outputs.\n", output_count);
    exit(EXIT_FAILURE);
  }

  data = (char*) safeMalloc(sizeof(char) * datasize);

  // set delay buffer sizes beforehand
  writeBuffer(commands, sizeof(int), output_io_size, &output_sz);
  for (int i = 0; i < input_count; i++) {
    writeBuffer(commands, sizeof(int), input_ios_size[i], &input_szs[i]);
  }

  int first_run = 1;
  
  while (cleanup_var) {

    int err;
    for (int i = 0; i < input_count; i++) {
      cl_buffer_region region;
      region.origin = input_ofs[i] * datasize;
      region.size = datasize;
      cl_mem sub = clCreateSubBuffer(input_ios[i], CL_MEM_READ_ONLY, CL_BUFFER_CREATE_TYPE_REGION, &region, &err);

      if (err != CL_SUCCESS) {
	    fprintf(stderr, "Error: Failed to create sub-buffer.\n");
	    exit(EXIT_FAILURE);
      }

      if (first_run || input_vid[i]) {
	fread(data, datasize, 1, input_fds[i]);
	writeBuffer(commands, datasize, sub, data);
	input_ofs[i] = (input_ofs[i] + 1) % input_szs[i];
	writeBuffer(commands, sizeof(int), input_ios_offset[i], &input_ofs[i]);
      }
    }

    // write OSC vars here
    writeBuffer(commands, sizeof(int), osc1, &osc1_var);
    writeBuffer(commands, sizeof(int), osc2, &osc2_var);

    executeKernel(commands, kernel, &global, &local);

    cl_buffer_region region_out;
    region_out.origin = output_of * datasize;
    region_out.size = datasize;
    cl_mem sub_out = clCreateSubBuffer(output_io, CL_MEM_WRITE_ONLY, CL_BUFFER_CREATE_TYPE_REGION, &region_out, &err);

    readBuffer(commands, datasize, sub_out, data);
    fwrite(data, datasize, 1, output_fd);
    output_of = (output_of + 1) % output_sz;
    writeBuffer(commands, sizeof(int), output_io_offset, &output_of);
    fflush(output_fd);
    first_run = 0;
  }
  real_cleanup();
  return 0;
}
