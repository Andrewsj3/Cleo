# Cleo

A rewrite of [smp](https://github.com/Andrewsj3/smp) using [SFML](https://www.sfml-dev.org/),
written in C++.

Since this project has reached version 1.0, `smp` has officially been deprecated. The functionality
is very similar, although some commands from `smp` have been changed/removed:
Command          |Changed/<br>Removed|Reason
-----------------|-------------------|------
`unpause`        |Removed            |Pause acts as a toggle, so this command is unnecessary.
`config`         |Removed            |This command could only print the current config or generate<br>a default one, which was not useful enough to include here
`exec`           |Changed            |This command was renamed to `run` to be more user-friendly.<br>Also, scripts can no longer run other scripts for security reasons.
`queue`          |Changed            |Renamed to `playlist` to clarify its purpose. However, `queue`<br>is still a valid alias.
`macro`          |Removed            |Macros, while convenient, allowed recursion which could break<br>things. If you want to execute multiple commands at once,<br>use scripts.
`queue swap`     |Removed            |Unfortunately, manipulating a playlist through a command-line<br>can be very difficult, and this command did not make it any easier.
`queue randomize`|Removed            |The functionality of this command has been merged into<br>`playlist shuffle`.
`queue insert`   |Removed            |Like `queue swap`, this command is not a good solution to the<br>problem it tries to solve.

## Improvements over `smp`
* Autocomplete support for commands, songs, files, and help
* Playlists now advance while in help mode - due to a design oversight, this did not work in `smp`
* Exits cleanly when using Ctrl-C or Ctrl-D - this used to cause exceptions
* Greatly improved error handling - `smp` has lots of instances where it fails on invalid inputs
* Reduced CPU and RAM usage
* Better code quality which makes development a lot easier

## Requirements
* SFML: At least v3.0.0, preferably latest version
* A compiler that supports C++23

> [!IMPORTANT]
> This application only works on Linux. Windows support will be added soon, but it will lack some features.
