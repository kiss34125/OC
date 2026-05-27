#define _POSIX_C_SOURCE 200809L

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

extern char **environ;

#define MAX_LINE 4096
#define MAX_TOKENS 256

/* Представление разобранной команды из одной строки ввода. */
typedef struct {
    char *argv[MAX_TOKENS];
    int argc;
    char *input_file;
    char *output_file;
    int append_output;
    int background;
} command_t;

/* Глобальное состояние оболочки, используется в основном цикле. */
static int g_running = 1;
static char g_shell_path[PATH_MAX];
static char g_help_path[PATH_MAX];

/* Удаляет завершающие CR/LF у строки, прочитанной через fgets(). */
static void trim_line(char *line) {
    size_t n = strcspn(line, "\r\n");
    line[n] = '\0';
}

/* Сбрасывает структуру команды перед разбором новой строки. */
static void init_command(command_t *cmd) {
    memset(cmd, 0, sizeof(*cmd));
}

/*
 * Разбирает строку команды в argv и флаги перенаправления/фона.
 * Предположение (по условию лабы): специальные токены отделены пробелами.
 */
static int parse_command(char *line, command_t *cmd) {
    char *saveptr = NULL;
    char *tok = NULL;

    init_command(cmd);
    tok = strtok_r(line, " \t", &saveptr);

    while (tok != NULL) {
        if (strcmp(tok, "<") == 0) {
            /* Перенаправление stdin: < input_file */
            tok = strtok_r(NULL, " \t", &saveptr);
            if (tok == NULL) {
                fprintf(stderr, "myshell: missing input file after '<'\n");
                return 0;
            }
            cmd->input_file = tok;
        } else if (strcmp(tok, ">") == 0) {
            /* Перенаправление stdout с перезаписью: > output_file */
            tok = strtok_r(NULL, " \t", &saveptr);
            if (tok == NULL) {
                fprintf(stderr, "myshell: missing output file after '>'\n");
                return 0;
            }
            cmd->output_file = tok;
            cmd->append_output = 0;
        } else if (strcmp(tok, ">>") == 0) {
            /* Перенаправление stdout с дозаписью: >> output_file */
            tok = strtok_r(NULL, " \t", &saveptr);
            if (tok == NULL) {
                fprintf(stderr, "myshell: missing output file after '>>'\n");
                return 0;
            }
            cmd->output_file = tok;
            cmd->append_output = 1;
        } else if (strcmp(tok, "&") == 0) {
            /* Флаг фонового запуска. */
            cmd->background = 1;
        } else {
            /* Обычный аргумент команды. */
            if (cmd->argc >= MAX_TOKENS - 1) {
                fprintf(stderr, "myshell: too many tokens\n");
                return 0;
            }
            cmd->argv[cmd->argc++] = tok;
        }
        tok = strtok_r(NULL, " \t", &saveptr);
    }

    cmd->argv[cmd->argc] = NULL;
    return cmd->argc > 0;
}

/* Синхронизирует PWD в окружении с текущим каталогом. */
static void update_pwd_env(void) {
    char cwd[PATH_MAX];
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        setenv("PWD", cwd, 1);
    }
}

/* Определяет полный путь к исполняемому shell и выставляет shell=... */
static void detect_shell_path(const char *argv0) {
    ssize_t len = readlink("/proc/self/exe", g_shell_path, sizeof(g_shell_path) - 1);
    if (len > 0) {
        g_shell_path[len] = '\0';
    } else {
        if (realpath(argv0, g_shell_path) == NULL) {
            strncpy(g_shell_path, argv0, sizeof(g_shell_path) - 1);
            g_shell_path[sizeof(g_shell_path) - 1] = '\0';
        }
    }

    setenv("shell", g_shell_path, 1);
}

/* Формирует путь к локальному readme, который читает команда help. */
static void detect_help_path(void) {
    const char *slash = strrchr(g_shell_path, '/');
    if (slash == NULL) {
        strncpy(g_help_path, "readme", sizeof(g_help_path) - 1);
        g_help_path[sizeof(g_help_path) - 1] = '\0';
        return;
    }

    {
        size_t dir_len = (size_t)(slash - g_shell_path);
        if (dir_len + 8 >= sizeof(g_help_path)) {
            strncpy(g_help_path, "readme", sizeof(g_help_path) - 1);
            g_help_path[sizeof(g_help_path) - 1] = '\0';
            return;
        }
        memcpy(g_help_path, g_shell_path, dir_len);
        g_help_path[dir_len] = '\0';
        strcat(g_help_path, "/readme");
    }
}

/* В интерактивном режиме приглашение показывает текущий каталог. */
static void print_prompt(void) {
    char cwd[PATH_MAX];
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        printf("%s> ", cwd);
    } else {
        printf("myshell> ");
    }
    fflush(stdout);
}

/* Проверяет, является ли команда встроенной (обрабатывается в myshell). */
static int is_builtin(const char *name) {
    return strcmp(name, "cd") == 0 || strcmp(name, "clr") == 0 || strcmp(name, "dir") == 0 ||
           strcmp(name, "environ") == 0 || strcmp(name, "echo") == 0 || strcmp(name, "help") == 0 ||
           strcmp(name, "pause") == 0 || strcmp(name, "quit") == 0;
}

/* Встроенная cd: без аргументов печатает cwd, иначе меняет каталог. */
static void run_cd(const command_t *cmd, FILE *out) {
    char cwd[PATH_MAX];

    if (cmd->argc == 1) {
        if (getcwd(cwd, sizeof(cwd)) != NULL) {
            fprintf(out, "%s\n", cwd);
        } else {
            perror("cd");
        }
        return;
    }

    if (cmd->argc > 2) {
        fprintf(stderr, "cd: too many arguments\n");
        return;
    }

    if (chdir(cmd->argv[1]) != 0) {
        perror("cd");
        return;
    }

    update_pwd_env();
}

/* Встроенная clr: очищает экран терминала (ANSI-последовательность). */
static void run_clr(FILE *out) {
    fputs("\033[H\033[J", out);
}

/* Встроенная dir: выводит содержимое каталога через POSIX dir API. */
static void run_dir(const command_t *cmd, FILE *out) {
    const char *target = ".";
    DIR *dir = NULL;
    struct dirent *entry = NULL;

    if (cmd->argc >= 2) {
        target = cmd->argv[1];
    }
    if (cmd->argc > 2) {
        fprintf(stderr, "dir: too many arguments\n");
        return;
    }

    dir = opendir(target);
    if (dir == NULL) {
        perror("dir");
        return;
    }

    while ((entry = readdir(dir)) != NULL) {
        fprintf(out, "%s\n", entry->d_name);
    }

    closedir(dir);
}

/* Встроенная environ: печатает все переменные окружения. */
static void run_environ(FILE *out) {
    char **env = environ;
    while (env != NULL && *env != NULL) {
        fprintf(out, "%s\n", *env);
        env++;
    }
}

/* Встроенная echo: печатает аргументы, разделяя их одним пробелом. */
static void run_echo(const command_t *cmd, FILE *out) {
    int i = 0;
    for (i = 1; i < cmd->argc; i++) {
        if (i > 1) {
            fputc(' ', out);
        }
        fputs(cmd->argv[i], out);
    }
    fputc('\n', out);
}

/* Встроенная help: выводит readme из каталога оболочки. */
static void run_help(FILE *out) {
    FILE *f = fopen(g_help_path, "r");
    int ch = 0;

    if (f == NULL) {
        fprintf(out, "myshell builtins: cd clr dir environ echo help pause quit\n");
        fprintf(out, "No readme file found near myshell executable.\n");
        return;
    }

    while ((ch = fgetc(f)) != EOF) {
        fputc(ch, out);
    }
    fclose(f);
}

/* Встроенная pause: ждёт нажатия Enter в управляющем терминале. */
static void run_pause(void) {
    FILE *tty_in = fopen("/dev/tty", "r");
    FILE *tty_out = fopen("/dev/tty", "w");
    int ch = 0;

    if (tty_out != NULL) {
        fprintf(tty_out, "Press Enter to continue...");
        fflush(tty_out);
    } else {
        printf("Press Enter to continue...");
        fflush(stdout);
    }

    if (tty_in != NULL) {
        while ((ch = fgetc(tty_in)) != '\n' && ch != EOF) {
        }
    } else {
        while ((ch = getchar()) != '\n' && ch != EOF) {
        }
    }

    if (tty_out != NULL) {
        fputc('\n', tty_out);
        fclose(tty_out);
    } else {
        fputc('\n', stdout);
    }

    if (tty_in != NULL) {
        fclose(tty_in);
    }
}

/* Вызывает обработчик нужной встроенной команды по имени. */
static void run_builtin(const command_t *cmd, FILE *out) {
    const char *name = cmd->argv[0];

    if (strcmp(name, "cd") == 0) {
        run_cd(cmd, out);
    } else if (strcmp(name, "clr") == 0) {
        run_clr(out);
    } else if (strcmp(name, "dir") == 0) {
        run_dir(cmd, out);
    } else if (strcmp(name, "environ") == 0) {
        run_environ(out);
    } else if (strcmp(name, "echo") == 0) {
        run_echo(cmd, out);
    } else if (strcmp(name, "help") == 0) {
        run_help(out);
    } else if (strcmp(name, "pause") == 0) {
        run_pause();
    } else if (strcmp(name, "quit") == 0) {
        g_running = 0;
    }
}

/* Открывает поток вывода для редиректа встроенных команд (> или >>). */
static FILE *open_builtin_output(const command_t *cmd) {
    if (cmd->output_file == NULL) {
        return stdout;
    }

    if (cmd->append_output) {
        return fopen(cmd->output_file, "a");
    }
    return fopen(cmd->output_file, "w");
}

/*
 * Выполняет внешнюю программу:
 * - создаёт дочерний процесс (fork)
 * - настраивает редиректы ввода/вывода в дочернем
 * - запускает программу через execvp
 * - в родителе ждёт завершения, если не запрошен фон
 */
static void execute_external(const command_t *cmd) {
    pid_t pid = fork();

    if (pid < 0) {
        perror("fork");
        return;
    }

    if (pid == 0) {
        int fd = -1;

        /* Передаёт дочерним программам путь к родительской оболочке. */
        setenv("parent", g_shell_path, 1);

        if (cmd->input_file != NULL) {
            /* Перенаправляет stdin из файла. */
            fd = open(cmd->input_file, O_RDONLY);
            if (fd < 0) {
                perror("input redirection");
                _exit(1);
            }
            if (dup2(fd, STDIN_FILENO) < 0) {
                perror("dup2 stdin");
                close(fd);
                _exit(1);
            }
            close(fd);
        }

        if (cmd->output_file != NULL) {
            /* Перенаправляет stdout в файл (перезапись или дозапись). */
            int flags = O_CREAT | O_WRONLY;
            mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;

            if (cmd->append_output) {
                flags |= O_APPEND;
            } else {
                flags |= O_TRUNC;
            }

            fd = open(cmd->output_file, flags, mode);
            if (fd < 0) {
                perror("output redirection");
                _exit(1);
            }
            if (dup2(fd, STDOUT_FILENO) < 0) {
                perror("dup2 stdout");
                close(fd);
                _exit(1);
            }
            close(fd);
        }

        execvp(cmd->argv[0], cmd->argv);
        perror("execvp");
        _exit(127);
    }

    if (cmd->background) {
        /* Для фонового процесса не ждём завершения. */
        printf("[background pid %d]\n", (int)pid);
        fflush(stdout);
        return;
    }

    /* Для foreground-режима ждём завершения дочернего процесса. */
    while (waitpid(pid, NULL, 0) < 0) {
        if (errno != EINTR) {
            perror("waitpid");
            break;
        }
    }
}

/* Подбирает завершившиеся фоновые процессы, чтобы не копились zombie. */
static void reap_background_children(void) {
    while (waitpid(-1, NULL, WNOHANG) > 0) {
    }
}

/* Основной цикл REPL: чтение, разбор и запуск команды. */
int main(int argc, char *argv[]) {
    FILE *input = stdin;
    int interactive = 1;
    char line[MAX_LINE];

    if (argc > 2) {
        fprintf(stderr, "Usage: %s [batchfile]\n", argv[0]);
        return 1;
    }

    if (argc == 2) {
        /* Batch-режим: команды читаются из файла вместо интерактивного ввода. */
        input = fopen(argv[1], "r");
        if (input == NULL) {
            perror("batch file");
            return 1;
        }
        interactive = 0;
    }

    detect_shell_path(argv[0]);
    detect_help_path();
    update_pwd_env();

    while (g_running) {
        command_t cmd;
        FILE *out = stdout;

        /* Периодически очищает завершившиеся фоновые задачи. */
        reap_background_children();

        if (interactive) {
            print_prompt();
        }

        if (fgets(line, sizeof(line), input) == NULL) {
            break;
        }

        trim_line(line);

        if (line[0] == '\0') {
            continue;
        }

        if (!parse_command(line, &cmd)) {
            continue;
        }

        if (is_builtin(cmd.argv[0])) {
            /* Встроенные команды выполняются в процессе оболочки (без fork). */
            out = open_builtin_output(&cmd);
            if (out == NULL) {
                perror("output redirection");
                continue;
            }

            run_builtin(&cmd, out);

            if (out != stdout) {
                fclose(out);
            }
            continue;
        }

        /* Невстроенные команды запускаются как внешние программы. */
        execute_external(&cmd);
    }

    if (input != stdin) {
        fclose(input);
    }

    return 0;
}
