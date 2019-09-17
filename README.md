# NewSH
It creates an advanced shell that supports all commands and the built-ins cd, pwd, history, and exit. Moreover, it supports up to 2 pipes per command line and redirection.

The program is set in a way that can handle two different modes, interactive and batch.

In interactive mode, the code takes the commands and options from stdin and executes them in a child processes. To terminate and exit the shell, just type exit

In batch mode, the code reads the command lines from a file specified as a command-line parameter

NOTE: This program was written in such a way that it accounts for all known security vulnerabilities present in other shells such as zsh. Permissions and root privileges attacks are strictly accounted for.
