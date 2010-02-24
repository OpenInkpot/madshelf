#include "madshelf.h"

#include <err.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include <string.h>

static void
clipboard_clear(madshelf_state_t *state)
{
    free(state->clipboard_path);
    state->clipboard_path = NULL;
    state->clipboard_active = false;
}

static bool
is_path_valid(const char *path)
{
    struct stat st;
    if (stat(path, &st) == -1)
        return false;
    if (!S_ISDIR(st.st_mode) && !S_ISREG(st.st_mode))
        return false;
    return true;
}

bool
is_clipboard_active(madshelf_state_t *state)
{
    if (state->clipboard_active && !is_path_valid(state->clipboard_path))
        clipboard_clear(state);

    return state->clipboard_active;
}

void
clipboard_paste(madshelf_state_t *state, char *dest)
{
    struct stat st;
    if (stat(dest, &st) == -1) {
        // FIXME: show error notice
        return;
    }

    if (!S_ISDIR(st.st_mode)) {
        // FIXME: show error notice
        return;
    }

    if (!is_clipboard_active(state)) {
        // FIXME: show error notice
    } else {
        // FIXME: progress dialog
        pid_t pid = fork();
        if (pid == -1)
            err(1, "clipboard_paste: fork");
        if (pid == 0) {
            /* Child */

            if (state->clipboard_copy) {
                char *const args[] = { "cp", "-Rf",
                                       state->clipboard_path, dest, NULL };
                execv("/bin/cp", args);
            } else {
                char *const args[] = { "mv", "-f",
                                       state->clipboard_path, dest, NULL };
                execv("/bin/mv", args);
            }
            err(1, "clipboard_paste: execv");
        }

        /* Parent */
        int status;
        if(waitpid(pid, &status, 0) == -1)
            err(1, "clipboard_paste: waitpid");
        if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
            // FIXME: show error notice
        }
    }

    clipboard_clear(state);
}

void
clipboard_new(madshelf_state_t *state, char *src, bool copy)
{
    clipboard_clear(state);

    if (is_path_valid(src)) {
        state->clipboard_active = true;
        state->clipboard_copy = copy;
        state->clipboard_path = strdup(src);
    }
}
