2014 NCTU Network Programming HW1 - Remote Access System (ras)
==============================================================

In this homework, you are asked to design rsh-like access systems,
called remote access systems, including both
client and server.  In this system, the server designates a directory,
say ``ras/``; then, clients can only run executable programs inside the
directory, ``ras/``.

The following is a scenario of using the system.

login::
    
    csh> telnet myserver.nctu.edu.tw 7000 # the server port number
    **************************************************************
    ** Welcome to the information server, myserver.nctu.edu.tw. **
    **************************************************************
    ** You are in the directory, /home/studentA/ras/.
    ** This directory will be under "/", in this system.
    ** This directory includes the following executable programs.
    **
    **    bin/
    **    test.html    (test file)
    **
    ** The directory bin/ includes:
    **    cat
    **    ls
    **    removetag         (Remove HTML tags.)
    **    removetag0        (Remove HTML tags with error message.)
    **    number            (Add a number in each line.)
    **
    ** In addition, the following two commands are supported by ras.
    **    setenv
    **    printenv
    **

bulitin commands: printenv and setenv and (exit)::

    % printenv PATH                       # Initial PATH is bin/ and ./
    PATH=bin:.
    % setenv PATH bin                     # Set to bin/ only
    % printenv PATH
    PATH=bin

commands: ls, cat, removetag, removetag0, number::

    % ls
    bin/        test.html
    % ls bin
    ls        cat        removetag     removetag0    number
    % cat test.html > test1.txt
    % cat test1.txt
    <!test.html>
    <TITLE>Test<TITLE>
    <BODY>This is a <b>test</b> program
    for ras.
    </BODY>
    % removetag test.html

    Test
    This is a test program
    for ras.

    % removetag test.html > test2.txt
    % cat test2.txt

    Test
    This is a test program
    for ras.

    % removetag0 test.html
    Error: illegal tag "!test.html"

    Test
    This is a test program
    for ras.

    % removetag0 test.html > test2.txt
    Error: illegal tag "!test.html"
    % cat test2.txt

    Test
    This is a test program
    for ras.

pipe::

    % removetag test.html | number
      1
      2 Test
      3 This is a test program
      4 for ras.
      5
    % removetag test.html |1 number > test3.txt        # |1 does the same thing as normal pipe
    % cat test3.txt
      1
      2 Test
      3 This is a test program
      4 for ras.
      5
    % removetag test.html |3 removetag test.html | number |1 number        # |3 skips two processes.
      1
      2    Test
      3    This is a test program
      4    for ras.
      5
      1   1
      2   2    Test
      3   3    This is a test program
      4   4    for ras.
      5   5
    % ls |2 ls | cat		# in this case, two ls are forked and running concurrently and output to the same pipe.
      bin/                  # the output from two ls commands might mix together(due to the CPU scheduling of different processes)
      test.html
      bin/
      test1.txt
      test.html
      test2.txt
      test1.txt
      test2.txt
    % ls |2 removetag test.html        # ls pipe to next command

      Test
      This is a test program
      for ras.

    % cat                              # ls pipe to this command
      bin/
      test.html
      test1.txt
      test2.txt
    % ls |2                            # only pipe to second next legal process, doesn’t output
    % asdasdas                         # illegal command will not be counted
      Unknown command: [asdasdas].
    % removetag test.html | cat        # cat is second next legal process of ls
      bin/
      test.html
      test1.txt
      test2.txt

      Test
      This is a test program
      for ras.

use system commands(program), like date::

    % date
      Unknown Command: [date].
    # Let TA do this "cp /bin/date bin"  in your csh directory
    % date
      Wed Oct  1 00:41:50 CST 2003

exit::

    % exit
    csh>

Requirements and Hints
----------------------

1. All data to stdout and stderr from server programs return to
   clients.

2. The remote directory in the server at least needs to include
   "removetag" and "number" and a test html file.

3. The programs removetag and number are not important in this project.
   TAs will provide you with these two programs.

4. You MUST use "exec" to run "ls", etc.  You MUST NOT use functions
   like "system()" or some other functions (in lib) to do the job.
   That is, you cannot use a function which will include "exec".

5. Pipe "|" behave the same as that in Unix shell. However, pipe "\|n"
   pipes the stdout S1 to the stdin S2 of the n'th next legal process.

6. For commands that are empty or have errors, the pipe to the command is closed
   subsequently.

Additional comments:

1. All arguments MUST NOT INCLUDE "/" for security.
   You should print out error message instead.

2. You can still let stderr be the console output for your debugging messages.

3. In addition to demo, you also need to prepare a simple report.

Specifications
--------------
Input spec
++++++++++

1. The length of a single-line input will not exceed 10000 characters.
   There may be huge number of commands in a single-line input.
   Each command will not exceed 256 characters.

2. There must be one or more spaces between commands and symbols (or arguments.),
   but no spaces between pipe and numbers.

   e.g.
   ::

        cat hello.txt | number
        cat hello.txt |1 number

3. There will not be any '/' character in demo input.

4. Pipe ("|") will not come with "printenv" and "setenv."

5. Use '% ' as the command line prompt.

About server
------------

1. The welcome message MUST been shown as follows::

        ****************************************
        ** Welcome to the information server. **
        ****************************************

2. Close the connection between the server and the client immediately when the server receive "exit".

3. Note that the forked process of server MUST be killed when the connection to the client is closed.
   Otherwise, there may be lots zombie processes.

About parsing
-------------

1. If there is command not found, print as follows::

        Unknown command: [command].

   e.g.
   ::
        
        % ctt
        Unknown command: [ctt].

About a numbered-pipe
---------------------

1. \|N means the stdout of last command should be piped to next Nth legal process, where 1 <= N <= 1000.

2. If there is any error in a input line, the line number still counts.

e.g.
::

    % ls |1
    % ctt               <= unknown command, process number is not counted
    Unknown command: [ctt].
    % number
    1    bin/
    2    test.html

e.g.
::

    % ls |1 ctt | cat   <= if find any process illegal, process will stop immediately
    Unknown command: [ctt].
    % cat               <= this command is first legal process after "ls |1"
    bin/
    test.html

Other proposed
--------------

1. There must be ``ls``, ``cat``, ``removetag``, ``removetag0``, ``number`` in ``bin/`` of ``ras/``.

2. You have to execute the files in ``bin/`` with an ``exec()``-based function.(e.g. ``execvp()`` or ``execlp()`` ...)
