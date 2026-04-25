# minishell

A custom shell implementation inspired by Bash, written in C.

This project implements an interactive/non-interactive shell with:
- command parsing
- syntax validation
- pipelines and redirections
- here-doc support
- subshell execution with parenthesis
- logical operators (`&&`, `||`)
- environment variable expansion
- wildcard expansion (`*`)
- builtins (`echo`, `cd`, `pwd`, `export`, `unset`, `env`, `exit`)

## Project Status

The codebase is modular and covers the core minishell workflow end-to-end.
From source analysis, the shell is feature-rich for a 42 minishell project and has clear subsystem boundaries.

## Build

Requirements:
- Linux
- `cc`
- GNU Make
- `readline` development library

Build:

```bash
make
```

Clean object files:

```bash
make clean
```

Remove objects + binary:

```bash
make fclean
```

Rebuild from scratch:

```bash
make re
```

The executable name is:

```bash
./minishell
```

## Quick Usage

Interactive mode:

```bash
./minishell
```

Non-interactive mode (stdin pipe):

```bash
echo 'echo hello' | ./minishell
```

## Supported Syntax and Behavior

### Operators

Implemented token/operator support:
- pipe: `|`
- logical AND: `&&`
- logical OR: `||`
- subshell grouping: `(` `)`
- output redirection: `>`
- append redirection: `>>`
- input redirection: `<`
- here-doc redirection: `<<`

### Expansion

Implemented expansions:
- environment variable expansion: `$VAR`
- last status expansion: `$?`
- wildcard expansion: `*` (directory entries in current working directory)
- quote-aware expansion rules (single vs double quotes)

### Here-doc

Implemented behavior:
- `<< delimiter` creates temporary input data for command stdin
- delimiter quotes are removed for matching
- if delimiter is quoted, here-doc body is not expanded
- if delimiter is unquoted, variable expansion is performed in here-doc body

### Builtins

Implemented builtins:
- `echo` (supports `-n` style options)
- `cd` (`cd`, `cd -`, path changes, PWD/OLDPWD updates)
- `pwd`
- `export` (identifier validation, assignment, append with `+=`)
- `unset`
- `env`
- `exit` (numeric parsing and shell exit semantics)

## What Is Not Implemented (or Not Evident in Source)

From source inspection, the following are not implemented as shell features:
- command separators like `;`
- background jobs (`&`) as execution operator
- advanced wildcard classes (`?`, `[]`) in pathname matching
- command substitution (`$(...)` or backticks)
- arithmetic expansion
- history expansion (`!`)
- full Bash-compatible lexer/parser grammar

## Architecture Overview

Top-level modules:
- `parsing/`: tokenization, syntax checks, here-doc collection, AST/list extraction
- `execution/`: argument cleanup/expansion, redirection/data flow, process execution
- `builtins/`: builtin commands and env mutation
- `libft/`: local utility library
- root files: shell lifecycle, prompt, environment bootstrap

Core data structures:
- `t_shelldata`: global shell runtime state
- `t_env`: linked-list environment store
- `t_cmds`: parsed token list
- `t_minishell`: executable command/operator list (with nested command nodes for subshells)
- `t_args`, `t_redir`: command arguments and redirections

## Execution Pipeline (High-Level)

1. **Read input**
   - interactive: `readline(...)`
   - non-interactive: internal `get_next_line(...)`

2. **Tokenize**
   - split input into token list (`t_cmds`)
   - assign token types (`WORD`, operators, redir types, parens)

3. **Syntax validation**
   - validate operators/parens/redirection grammar
   - collect here-doc content during validation stage

4. **Extract executable structure**
   - convert token list into `t_minishell` linked representation
   - preserve nesting for parenthesized commands

5. **Prepare args and expansions**
   - quote handling
   - env expansion
   - wildcard expansion and sorting
   - ambiguity checks for redirection target expansion

6. **Execute**
   - optimize standalone builtins in parent process when possible
   - otherwise fork/pipe/dup/manage redirections
   - execute nested subshells recursively
   - resolve `&&` / `||` short-circuit behavior

7. **Collect status**
   - wait for children
   - update shell exit code and signal-derived statuses

## Error and Exit Semantics

Observed behaviors:
- syntax errors set shell exit status to `2`
- command-not-found typically maps to `127`
- permission/path execution errors use `126` semantics
- SIGINT handling follows shell-like behavior (`130` in many paths)
- child process signal termination is translated to `128 + signal`

## Prompt and Signal Handling

Prompt system:
- custom colored prompt with cwd and user information
- fallback prompt when composed prompt exceeds fixed buffer threshold

Signals:
- different handlers for interactive shell, child execution, and here-doc child
- interactive SIGINT clears current line and redisplays prompt

## Notes on Design Choices

- Environment is stored as linked list (`t_env`) and converted to `char **` only when needed for `execve`.
- Builtins are executed in the parent process when command context allows stateful effects (for example, `cd`).
- Here-doc data uses temporary files under `/tmp` and explicit cleanup paths.
- Argument expansion is implemented through a staged pattern/token pipeline before final `char **argv` extraction.

## Maintenance Suggestions

If you continue this project, high-value next steps are:
- add automated regression tests for parser, expansion, and exit-code behavior
- add a debug mode to print token list and extracted command graph
- unify/centralize error messages and status-setting conventions
- replace temporary here-doc files with pipe-based buffering where possible

## Author

Project by 42 students (as indicated in file headers):
- makarkao
- melayyad
