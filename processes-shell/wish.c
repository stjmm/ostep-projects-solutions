#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/wait.h>

#define MAX_SEGS 8
#define MAX_ARGS 8
#define MAX_PATHS 8

typedef struct {
    char *argv[MAX_ARGS];
    int argc;
    char *outfile;
} Command;

char *search_paths[MAX_PATHS];
int npath = 0;

void print_error()
{
    char error_message[30] = "An error has occurred\n";
    write(STDERR_FILENO, &error_message, strlen(error_message));
}

/* Removes leading and trailing whitespace chars */
char *trim(char *s)
{
    // Trim leading whitespaces
    while (isspace(*s)) s++;

    if (*s == '\0') return s;

    // Trim trailing whistpaces
    char *end = s + strlen(s) - 1;
    while (end > s && isspace(*end)) *end-- = '\0';

    return s;
}

// --- Path ---------------------------------------

void init_path()
{
    search_paths[npath++] = strdup("/bin");
}

// Adds and resets old paths
void set_path(Command *cmd)
{
    for (int i = 0; i < npath; i++) {
        free(search_paths[i]);
        search_paths[i] = NULL;
    }
    npath = 0;

    for (int i = 1; i < cmd->argc; i++) {
        search_paths[npath++] = strdup(cmd->argv[i]);
    }
}

/* Returns malloc'd string if found and NULL if not 
 * Resolves executable from any path in search_paths */
char *resolve_path(char *cmd)
{
    // Absolute paths skip searching in search_paths
    if (cmd[0] == '/') {
        if (access(cmd, X_OK) == 0) return strdup(cmd);
        return NULL;
    }

    char full[128];
    for (int i = 0; i < npath; i++) {
        snprintf(full, sizeof(full), "%s/%s", search_paths[i], cmd);
        if (access(full, X_OK) == 0) return strdup(full);
    }
    return NULL;
}

// --- Parsing ---------------------------------------

int handle_builtins(Command *cmd)
{
    if (strcmp(cmd->argv[0], "exit") == 0) {
        if (cmd->argc > 1) {
            print_error();
            return 1;
        }
        exit(0);
    } else if (strcmp(cmd->argv[0], "cd") == 0) {
        if (cmd->argc != 2) {
            print_error();
            return 1;
        }
        if (chdir(cmd->argv[1]) != 0) print_error();
        return 1;
    } else if (strcmp(cmd->argv[0], "path") == 0) {
        set_path(cmd);
        return 1;
    }
    return 0;
}

void process_segment(char *seg, Command *cmd)
{
    cmd->argc = 0;
    cmd->outfile = NULL;

    char *redir = strchr(seg, '>');
    if (redir) {
        *redir = '\0';
        char *file = trim(redir + 1);

        // Error if nothing after '>' or multiple words after '>'
        if (strlen(file) == 0 || strchr(file, ' ')) {
            print_error();
            cmd->argc = 0;
            return;
        }
        cmd->outfile = file;
    }

    char *tok;
    while((tok = strsep(&seg, " \t")) != NULL &&
            cmd->argc < MAX_ARGS - 1) {
        if (*tok == '\0') continue;
        cmd->argv[cmd->argc++] = tok;
    }
    cmd->argv[cmd->argc] = NULL;

    // If only '> output' with no command
    if (cmd->argc == 0 && cmd->outfile != NULL) {
        print_error();
        return;
    }
}

void process_line(char *line)
{
    // Strip newline
    line[strcspn(line, "\n")] = '\0';

    // Skip empty lines
    if (strlen(trim(line)) == 0) return;

    // Split on '&' into segments (parallel commands)
    char *copy = strdup(line);
    char *segments[MAX_SEGS];
    int nseg = 0;

    char *tok;
    while ((tok = strsep(&copy, "&")) != NULL) {
        segments[nseg++] = trim(tok);
    }

    pid_t pids[MAX_SEGS];
    int npid = 0;

    for (int i = 0; i < nseg; i++) {
        Command cmd;
        process_segment(segments[i], &cmd);

        if (cmd.argc == 0) continue;

        if (handle_builtins(&cmd)) continue;

        char *fullpath = resolve_path(cmd.argv[0]);
        if (!fullpath) {
            print_error();
            continue;
        }

        pid_t pid = fork();
        pids[npid++] = pid;
        if (pid == 0) {
            if (cmd.outfile) {
                int fd = open(cmd.outfile, O_WRONLY|O_CREAT|O_TRUNC, 0644);
                if (fd < 0) { print_error(); exit(1); }
                dup2(fd, STDOUT_FILENO);
                dup2(fd, STDERR_FILENO);
                close(fd);
            }
            execv(fullpath, cmd.argv);
            // If execv correct it doesnt return
            print_error();
            exit(1);
        } else if (pid < 0) {
            print_error();
        }

        free(fullpath);
    }

    for (int i = 0; i < npid; i++) {
        if (pids[i] > 0) waitpid(pids[i], NULL, 0);
    }

    free(copy);
}

int main(int argc, char **argv)
{
    if (argc > 2) {
        print_error();
        exit(1);
    }

    FILE *input = NULL;
    int interactive_mode = 0;
    
    if (argc == 1) {
        input = stdin;
        interactive_mode = 1;
    } else {
        input = fopen(argv[1], "r");
        if (!input) {
            print_error();
            exit(1);
        }
    }

    char *line = NULL;
    size_t line_size = 0;

    init_path();

    while (1) {
        if (interactive_mode) printf("wish> ");
        fflush(stdout);
        if (getline(&line, &line_size, input) == -1) exit(0);
        process_line(line);
    }

    fclose(input);
    free(line);

    return 0;
}
