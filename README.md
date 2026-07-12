# minishell

> *"As beautiful as a shell"*  a 42/1337 school project.

**minishell** is a Unix command-line interpreter written in C, re-implementing the core behavior of Bash. It reads a command line, tokenizes and validates it, expands variables and wildcards, then executes it  handling pipelines, redirections, here-documents, logical operators, subshells, and a set of built-in commands, all with faithful exit-status and signal semantics.

## Goal

The goal of the project is to understand how a real shell works under the hood by building one from scratch using only low-level system calls:

- **Process management**  `fork`, `execve`, `waitpid`, exit-status propagation
- **File descriptors**  `pipe`, `dup2`, `open`, `close` for pipelines and redirections
- **Signal handling**  `SIGINT` / `SIGQUIT` behavior in interactive, child, and here-doc contexts
- **Parsing**  turning a raw string into a validated, executable command structure
- **Memory discipline**  every allocation tracked and freed, no leaks on any exit path

The only external convenience allowed is the **readline** library (prompt, line editing, history).

## Features

| Category     | Support                                                                               |
| ------------ | ------------------------------------------------------------------------------------- |
| Operators    | `\|` pipes, `&&` / `\|\|` logical operators, `( )` subshells                     |
| Redirections | `<` input, `>` output, `>>` append, `<<` here-document                        |
| Expansions   | `$VAR`, `$?` (last exit status), `*` wildcard, quote-aware rules                |
| Quoting      | single quotes (literal), double quotes (expansion allowed)                            |
| Builtins     | `echo -n`, `cd`, `pwd`, `export` (incl. `+=`), `unset`, `env`, `exit` |
| Modes        | interactive (readline prompt) and non-interactive (piped stdin)                       |
| Extras       | colored prompt with cwd/user, command history,`cd -`                                |

Not implemented (out of scope): `;` separators, background jobs (`&`), command substitution `$(...)`, `?`/`[]` glob classes, arithmetic expansion.

## Build & Run

Requirements: Linux, `cc`, GNU Make, and the readline development library (`libreadline-dev`).

```bash
make          # build ./minishell (also builds bundled libft)
make clean    # remove object files
make fclean   # remove objects + binary
make re       # full rebuild
```

```bash
./minishell                       # interactive mode
echo 'echo hello' | ./minishell   # non-interactive mode
```

## Architecture

```
minishell/
├── 1-minishell.c            # main loop: read → parse → execute
├── 2-prompt_minishell.c     # colored prompt composition
├── 3-minishell_helper.c     # init, interactive/non-interactive input, cleanup, exit
├── 4-t_env_set_env.c        # environment bootstrap (envp → t_env list)
├── parsing/
│   ├── 1-split_line/        # tokenization into t_cmds + token typing
│   ├── 2-syntax_check/      # operator / parenthesis / redirection grammar
│   ├── 3-here_doc/          # here-doc collection (child process + tmp file)
│   └── 4-extract_t_minishell_cmd/  # t_cmds → t_minishell command graph
├── execution/
│   ├── 1-manage_data_flow/  # pipes, redirections, heredoc expansion, ambiguity
│   ├── 2-extract_clean_args/# quote removal, $ expansion, wildcards → argv
│   ├── 3-extract_path/      # PATH resolution + permission/stat diagnostics
│   ├── 4-extract_execve_env/# t_env list → char** for execve
│   └── 22/23/24-*.c         # executor: fork, pipe wiring, &&/|| logic, wait
├── builtins/                # echo, cd, pwd, export, unset, env, exit
└── libft/                   # utility library (string/memory helpers)
```

The main loop in `1-minishell.c` is deliberately simple:

```c
while (1)
{
    interactive_non_interactive(shelldata);   // read a line
    if (shelldata->line && shelldata->line[0])
    {
        add_history(shelldata->line);
        parse_line(shelldata);                // tokenize → validate → build graph
        execute_cmd(shelldata);               // run it
    }
    free_shelldata(shelldata, F_SHELL | F_LINE, 0);
}
```

## Data Structures

All core structures are defined in `parsing/parsing.h`.

### `t_shelldata`  global shell state (singleton)

Accessed everywhere through `shelldata_memo()`, a function returning a pointer to a single static instance (the project norm allows only one global variable, reserved for the signal flag `g_sigint`).

```c
typedef struct s_shelldata
{
    t_env       *env;          // environment as a linked list
    t_cmds      *cmd;          // token list (parsing stage)
    t_minishell *shell;        // executable command graph (execution stage)
    char        *line;         // raw input line
    char        *cwd;          // cached working directory
    void        *sigint_h;     // saved signal handlers
    void        *sigquit_h;
    int         state;         // error state (<0 → fatal, triggers cleanup/exit)
    int         std_incpy;     // stdin/stdout backups for parent-run builtins
    int         std_outcpy;
    int         lvl;           // subshell nesting depth
    int         exit_status;   // $? value
} t_shelldata;
```

### `t_cmds`  token list (output of the lexer)

A doubly linked list of tokens produced by `split_line`. Each token carries a `t_type` from the enum (`WORD`, `PIPE`, `AND`, `OR`, `O_PARENTHES`, `C_PARENTHES`, `HEREDOC`, `APPEND`, `INFILE`, `OUTFILE`, `*_NAME`, `DELIMITER`, …). Here-doc tokens also store the fd of the already-collected body in `herdocfd`.

```c
typedef struct s_cmds
{
    t_type        type;
    char          *str;        // raw token text (quotes still present)
    int           herdocfd;    // fd of collected here-doc body
    struct s_cmds *next;
    struct s_cmds *previous;
} t_cmds;
```

### `t_minishell`  executable command graph

The token list is compiled into a linked list of **command nodes** and **operator nodes**. A node of type `WORD` is a simple command; a node of type `C_CMD` is a parenthesized subshell whose body hangs off `c_cmd`  making the structure recursive (a flat list of alternating commands/operators, with nested lists for subshells).

```c
typedef struct s_minishell
{
    pid_t               pid;            // child pid after fork
    int                 pipefd[2];      // pipe connecting this node to the previous one
    int                 args_extracted; // expansion already done (builtin fast path)
    int                 exp_split;      // special word-splitting rules for `export`
    char                *path;          // resolved binary path
    char                **args;         // final argv for execve
    t_args              *args_list;     // argument list before extraction
    char                **env;          // env flattened for execve
    t_type              type;           // WORD (command) | C_CMD (subshell) | PIPE/AND/OR
    t_redir             *redir;         // redirections attached to this command
    struct s_minishell  *next;
    struct s_minishell  *c_cmd;         // nested graph for ( ... )
    struct s_minishell  *previous;
} t_minishell;
```

### Supporting structures

```c
typedef struct s_args          // one command argument (pre-argv)
{
    char          *str;
    int           is_cmd;      // true for the first word (the command itself)
    struct s_args *next;
} t_args;

typedef struct s_redir         // one redirection, in source order
{
    t_type         type;       // INFILE_NAME | OUTFILE_NAME | APPEND_NAME | DELIMITER
    char           *redir_name;// target filename / heredoc delimiter
    int            redirfd;    // heredoc: fd of collected body
    struct s_redir *next;
} t_redir;

typedef struct s_env           // one environment variable
{
    char         *variable;    // name
    char         *value;       // value (may be NULL)
    int          ev_hide;      // exported-but-unset (visible to export, not env)
    struct s_env *next;
} t_env;
```

During expansion, arguments are further decomposed into `t_pattern` / `t_pattern_list` (defined in `execution/execution.h`): a token-within-a-word representation that distinguishes literal `WORD` segments from `WILDCARD` and `EXPANSION` segments, which is what makes quote-aware globbing (`"*"` vs `*`) possible.

## Parsing Process

Parsing is a four-stage pipeline driven by `parse_line()` (`parsing/5-parse_line.c`):

### 1. Tokenization  `split_line()` (`parsing/1-split_line/`)

The raw line is scanned character by character and split into a `t_cmds` list:

- whitespace separates tokens;
- operators (`|`, `&&`, `||`, `<`, `>`, `<<`, `>>`, `(`, `)`) become their own tokens, with doubled characters (`&&`, `||`, `<<`, `>>`) collapsed into one token;
- words are consumed with **quote tracking**: an opening `'` or `"` swallows everything up to the matching close, so `echo "a | b"` yields one word token. An unclosed quote is reported immediately as a syntax error (exit status 2).

Each token is then typed by `check_type()`. Context refines types: the word after `<` becomes `INFILE_NAME`, after `>>` becomes `APPEND_NAME`, after `<<` becomes `DELIMITER`, etc.

### 2. Syntax validation  `check_parsing()` (`parsing/2-syntax_check/`)

Grammar checks over the token list, mirroring Bash's error messages:

- operators cannot start a line, follow another operator, or end a line (`echo |` → ``syntax error near unexpected token `newline'``);
- parentheses must be balanced and non-empty, and content adjacent to them must be valid;
- every redirection must be followed by a filename/delimiter token.

On any violation the shell prints `minishell: syntax error near unexpected token '...'`, sets `$? = 2`, and returns to the prompt without executing anything.

### 3. Here-doc collection (`parsing/3-here_doc/`)

Interleaved with validation: as soon as a valid `<< DELIMITER` is confirmed, its body is read **immediately** (before execution, exactly like Bash). A child process reads lines with `readline("> ")` until the delimiter matches, writing them into an unlinked temporary file under `/tmp` (the file is deleted from the filesystem instantly; only the open fd keeps it alive). That read fd is stored on the token. Quotes in the delimiter are removed and remembered: a quoted delimiter (`<< "EOF"`) disables variable expansion in the body. `Ctrl-C` inside a here-doc aborts the whole command with status 130.

### 4. Command-graph extraction  `extract_minishell_cmd()` (`parsing/4-extract_t_minishell_cmd/`)

The flat token list is compiled into the executable `t_minishell` graph:

- consecutive `WORD` tokens are collected into an `t_args` list, redirection tokens into a `t_redir` list (in source order), and together they form a command node;
- each operator (`PIPE`, `AND`, `OR`) becomes its own node between commands;
- an `O_PARENTHES` triggers a **recursive** extraction: everything up to the matching `)` becomes a nested graph attached to `c_cmd`, and the node is typed `C_CMD`.

After extraction the token list is freed; execution works exclusively on the graph.

> Note: expansion (`$VAR`, wildcards, quote removal) is deliberately **not** done during parsing  it is deferred to execution time, per command, right before the command actually runs.

## Execution Process

Execution is driven by `execute_cmd()` → `execute_and_or()` (`execution/22-execution.c`).

### 1. Logical-operator walk  `execute_and_or()`

The executor walks the top-level graph, one **pipeline segment** at a time:

- run the segment, collect its exit status;
- on status `0`, skip forward past an `OR` to the next `AND` branch; on non-zero, skip past an `AND` to the next `OR` branch  classic short-circuit evaluation;
- a lone builtin with no pipe (`check_buitin_sample_cond`) is executed **in the parent process** so that `cd`, `export`, `unset` and `exit` can actually mutate shell state. stdin/stdout are backed up with `dup`, redirections applied, the builtin run, then the originals restored.

### 2. Pipeline execution  `execute_subshell()` (`execution/23-execution_subshell.c`)

For each command in a pipeline:

1. if the next node is a `PIPE`, create the pipe (`pipe(pipefd)`);
2. `fork()`; the parent closes its pipe ends and moves to the next command, so **all pipeline stages run concurrently**;
3. the child wires fds (`manage_data_flow`): `dup2` the previous pipe's read end onto stdin, the next pipe's write end onto stdout, then applies the command's redirections in order (redirections override pipes, as in Bash);
4. after wiring, the child dispatches on node type:
   - **`WORD`**  expand arguments (below), then either run a builtin and exit with its status, or resolve the binary via `extract_path()` (PATH search with `stat` checks producing `command not found` / `Permission denied` / `Is a directory` diagnostics), flatten the env, and `execve`;
   - **`C_CMD`**  recursively call `execute_and_or()` on the nested graph (`lvl` is incremented), so subshells inherit pipes/redirections but keep state changes isolated;
5. the parent `waitpid`s for all children; the pipeline's status is the last command's status, and a signal-killed child maps to `128 + signal` (e.g. `SIGINT` → 130, with the newline/`Quit` message handling Bash uses).

### 3. Argument expansion  `extract_clean_args()` (`execution/2-extract_clean_args/`)

Performed lazily per command, in the child (or in the parent for parent-run builtins). For each argument:

- **fast path**: if the word contains no unquoted `$` or `*` (checked by `check_wild_exp_out_quotes`), only in-quote expansion and quote removal are needed (`expand_inside_dquotes`): `$VAR`/`$?` are substituted inside double quotes, left literal inside single quotes, then quotes are stripped;
- **full path**: the word is tokenized into a `t_pattern` list of `WORD` / `EXPANSION` / `WILDCARD` segments:
  1. `$VAR` segments are substituted from `t_env` (`$?` from `exit_status`); an unquoted expansion result is **field-split** on whitespace, possibly producing several arguments (with a special exemption for `export VAR=$X`, which must not split);
  2. adjacent segments are re-joined into candidate words;
  3. words containing an unquoted `*` are matched against the current directory entries (`opendir`/`readdir`, hidden files excluded) by `check_pattern_new`, which anchors the pattern's first/last literal segments and searches middle segments in order; matches are inserted **sorted**, and a pattern with no match is kept literally;
- the final `t_args` list is flattened into the `char **argv` handed to `execve`.

Redirection targets go through the same expansion with one extra rule: if a target expands to zero or multiple words, the command fails with `minishell: $VAR: ambiguous redirect` (`27-handle_ambiguous_redirection.c`).

Here-doc bodies with unquoted delimiters are expanded at this stage too (`here_doc_redirection/`): the stored fd is read line by line with an internal `get_next_line`, `$` expansion applied, and the result rewritten to a fresh unlinked tmp file which becomes the command's stdin.

### 4. Status collection and signals

The parent shell ignores `SIGINT` while children run, then translates child termination into `$?`. Signal handling uses three handlers: interactive prompt (clear line, redisplay), child execution, and here-doc (abort and propagate 130), coordinated through the single global `volatile sig_atomic_t g_sigint`.

### Exit-status conventions

| Situation                                           | `$?`                              |
| --------------------------------------------------- | ----------------------------------- |
| success                                             | 0                                   |
| generic failure (builtin errors, dup/open failures) | 1                                   |
| syntax error / unclosed quote                       | 2                                   |
| command found but not executable / is a directory   | 126                                 |
| command not found                                   | 127                                 |
| terminated by signal*N*                           | 128 +*N* (e.g. `Ctrl-C` → 130) |

## Test Cases

All examples below were run against the built binary. In interactive mode just type the command; in non-interactive mode pipe it in (`echo '<cmd>' | ./minishell`).

### Basics & pipes

```bash
$ echo hello world
hello world

$ echo hello | tr a-z A-Z
HELLO

$ ls -l | grep minishell | wc -l        # multi-stage pipeline
```

### Logical operators & subshells

```bash
$ false && echo no || echo yes
yes

$ (echo a && echo b) | tr a-z A-Z
A
B

$ (cd /tmp && pwd) && pwd               # subshell cd does not leak
/tmp
/home/user/minishell
```

### Expansion & quoting

```bash
$ echo $USER $HOME
makarkao /home/makarkao

$ echo '$USER' "$USER"                  # single quotes literal, double expand
$USER makarkao

$ ls /nonexistent
ls: cannot access '/nonexistent': No such file or directory
$ echo $?
2
```

### Redirections

```bash
$ echo data > out.txt
$ cat < out.txt
data
$ echo more >> out.txt
$ cat out.txt
data
more

$ unset X
$ echo hi > $X                          # ambiguous redirect
minishell: $X: ambiguous redirect
```

### Wildcards

```bash
$ touch a.txt b.txt c.md
$ echo *.txt
a.txt b.txt
$ echo "*.txt"                          # quoted → no globbing
*.txt
$ echo *.nomatch                        # no match → literal
*.nomatch
```

### Here-documents

```bash
$ cat << EOF
> hello $USER
> EOF
hello makarkao

$ cat << "EOF"                          # quoted delimiter → no expansion
> hello $USER
> EOF
hello $USER
```

### Builtins & environment

```bash
$ export MYVAR=42
$ echo $MYVAR
42
$ export MYVAR+=0                       # append syntax
$ echo $MYVAR
420
$ unset MYVAR
$ echo [$MYVAR]
[]

$ cd /tmp && pwd
/tmp
$ cd - > /dev/null && pwd               # cd - returns to OLDPWD
/home/user/minishell

$ echo -n no newline
no newline$
```

### Error handling & exit codes

```bash
$ echo hello |
minishell: syntax error near unexpected token `|'
$ echo $?
2

$ nosuchcmd
nosuchcmd: command not found
$ echo $?
127

$ ./minishell                           # a directory is not executable
minishell: ./somedir: Is a directory    → $? = 126

$ exit 42                               # shell exits with status 42
```

### Signals (interactive)

- `Ctrl-C` at the prompt: prints a newline, redraws the prompt, sets `$?` to 130  does not quit the shell.
- `Ctrl-D` at an empty prompt: prints `exit` and quits with the last status.
- `Ctrl-C` during a here-doc: aborts the whole command with status 130.
- `Ctrl-\` at the prompt: ignored; sent to a running child, prints `Quit (core dumped)` and sets `$?` to 131.

## Authors

42/1337 students (see file headers):

- **makarkao**  parsing, expansion, builtins, shell lifecycle
- **melayyad**  execution engine, data flow, builtins
