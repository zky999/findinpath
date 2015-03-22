#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MAX_PATH_LEN	1024
#define MAX_FIND_PAR_LEN	1024
#define MAX_PATH_NUM	64
#define MAX_FIND_CMD_STR	2048
#define MAX_READ_BUF	512

#define ERR_GET_SYS_VAR	-1
#define ERR_MEM_ALLOC	-2
#define ERR_PAR_INVALID	-3
#define ERR_VAR_NOT_INIT	-4
#define ERR_CONST_TOO_SMALL	-5
#define ERR_GEN_FIND_CMD_STR	-6
#define ERR_EXE_FIND_CMD	-7

int get_sys_paths (char * sys_paths[]);
int get_find_par (char * find_par, int argc, char * argv[]);

int main (int argc, char * argv[])
{
  // Allocate memory for single paths
  char * sys_paths[MAX_PATH_NUM];
  int i;
  for (i = 0; i < MAX_PATH_NUM; i++) {
    sys_paths[i] = (char *) malloc (sizeof(char) * MAX_PATH_LEN);
    if (! sys_paths[i]) {
      for (i--; i > 0; i--) {
	if (sys_paths[i]) free(sys_paths[i]);
      }
      fprintf (stderr, "Error: %d: Memory allocation failed.\n", ERR_MEM_ALLOC);
      exit(ERR_MEM_ALLOC);
    }
  }

  // Get system paths.
  int result_code;
  result_code = get_sys_paths(sys_paths);
  if ( result_code < 0 ) {
    fprintf (stderr, "Error: %d: Get system path failed.\n", result_code);
    exit(result_code);
  }
  int path_num = result_code;
  // Ready to use sys_paths, path_num

  // Check path's existence
  int is_path_exist[path_num];
  int access_result;
  for (i=0; i<path_num; i++) {
    access_result = access(sys_paths[i], F_OK);
    if (access_result == 0) {
      is_path_exist[i] = 1;
    } else {
      is_path_exist[i] = 0;
    }
  }

  // Get find parameters
  char * find_par = (char *) malloc (sizeof(char) * MAX_FIND_PAR_LEN);    
  if (! find_par) {
    fprintf(stderr, "Error: %d: Memory allocation failed.\n", ERR_MEM_ALLOC);
    exit(ERR_MEM_ALLOC);
  }

  result_code = get_find_par(find_par, argc, argv);
  if ( result_code < 0 ) {
    fprintf(stderr, "Error: %d: Get find parameters failed.\n", result_code);
    exit(result_code);
  }
  // Ready to use find_par

  /* Main loop:
   * + Generate find command string
   * + Execute find command
   * + Read find command's results
   * + Print out find coomand's results
   */
  char * find_cmd_str = (char *) malloc (sizeof(char) * MAX_FIND_CMD_STR);
  if (! find_cmd_str) {
    fprintf(stderr, "Error: %d: Memory allocation failed.\n", ERR_MEM_ALLOC);
    exit(ERR_MEM_ALLOC);
  }

  // Main loop
  for (i = 0; i < path_num; i++) {  
    // skip the path if it is not exist
    if ( is_path_exist[i] == 0 ) {
      continue;
    }

    // Generate find command string
    if ( ! strcpy(find_cmd_str, "find \"")
	 || ! strcat(find_cmd_str, sys_paths[i])
	 || ! strcat(find_cmd_str, "\" ")
	 || ! strcat(find_cmd_str, find_par)) {
      fprintf(stderr, "Error: %d: Generate find command string failed.\n", ERR_GEN_FIND_CMD_STR);
      exit(ERR_GEN_FIND_CMD_STR);
    }

    // Exexute find command
    FILE * find_cmd_pipe = popen(find_cmd_str, "r");
    if (! find_cmd_pipe) {
      fprintf(stderr, "Error: %d: Execute find command failed.\n", ERR_EXE_FIND_CMD);
      exit(ERR_EXE_FIND_CMD);
    }
    
    // Read find command results
    char * read_buf = (char *) malloc (sizeof(char) * MAX_READ_BUF);
    if (! find_cmd_str) {
      fprintf(stderr, "Error: %d: Memory allocation failed.\n", ERR_MEM_ALLOC);
      exit(ERR_MEM_ALLOC);
    }
   
    // Print out find command's results
    while (fgets(read_buf, MAX_READ_BUF, find_cmd_pipe)) {
      fputs(read_buf, stdout);
    }

    // Finish current find command
    if (find_cmd_pipe) pclose(find_cmd_pipe);
  }

  // Cleaning
  if (find_cmd_str) free(find_cmd_str);
  if (find_par) free(find_par);
  for (i = 0; i < MAX_PATH_NUM; i++) {
    if (sys_paths[i]) free(sys_paths[i]);
  }

  return 0;
}

/* Get every single path of system environment variable: PATH
 * which are separated by colons, and save them in sys_paths[].
 */
int get_sys_paths (char * sys_paths[])
{
  // Make sure we have the space to store paths
  if (! sys_paths) {
    return ERR_PAR_INVALID;
  }

  // Get system PATHs.
  char * sys_paths_ptr = getenv("PATH");
  if (! sys_paths_ptr) {
    return ERR_GET_SYS_VAR;
  }

  // Get single paths
  int i = 0;
  char * path = sys_paths[i];
  while (*sys_paths_ptr != '\0') {
    if (*sys_paths_ptr != ':') {
      // Still in the process of copying one single path
      *path++ = *sys_paths_ptr++;	// Copy
    } else {				// Done with one single path
      *path++ = '\0';			// "Close" the single path just copied
      sys_paths_ptr++;			// Skip the ':' in the "source"

      // Get ready for storing another single path
      i++;
      if (i > MAX_PATH_NUM - 1)	return ERR_CONST_TOO_SMALL;
      path = sys_paths[i];
      // Make sure we have space to store a single path
      if (! path) return ERR_VAR_NOT_INIT;
    }
  }
  *path = '\0';				// "Close" the last single path
  return i+1;				// Return the number of paths
}

 /* Generate find parameter string from main()'s argv.
 * find_par: argv[1] argv[2]...
 */
int get_find_par (char * find_par, int argc, char * argv[])
{
  // Make sure we have the space to store the parameters.
  if (! find_par) {
    return ERR_VAR_NOT_INIT;
  }
  char * find_par_ptr = find_par;

  // Initialize path's first char
  *find_par_ptr = '\0';

  // For each part in argv
  int i;
  char const * arg_ptr;
  for (i = 1; i < argc; i++) {
    arg_ptr = argv[i];
    
    // For each char in one part
    // Copy one character one time, and do some escaping when nessaccery.
    while (*arg_ptr != '\0') {
      // If a escape-needed char found, then insert a '\' before copying it.
      if (*arg_ptr == '*'
	  || *arg_ptr == '('
	  || *arg_ptr == ')'
	  || *arg_ptr == '$'
	  || *arg_ptr == '\\')
	*find_par_ptr++ = '\\';
      *find_par_ptr++ = *arg_ptr++;
    }
    
    // Add a space or a NULL to the end of a part.
    // If this part is not the last one of argv parts, add a space, otherwise add a NULL.
    if (i != argc-1) {
      *find_par_ptr++ = ' ';
    } else {
      *find_par_ptr = '\0';
    }
  }
  return 0;
}
