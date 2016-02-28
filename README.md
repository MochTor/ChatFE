**DON'T USE ON A SERVER**

DISCLAIMER <br/>
This project was written for the Operating Systems class as part of the exam. It is focused on didatic purposes.

**I don't accept merge requests or new issues, 'cause I don't work on this project after the exam**.

For any question, write to me.

#ChatFe

That's the final project I discussed as part of the final exam for OS course. The goal was to develop a server-client software for a multiuser chat, using POSIX elements such as threads, processes, mutex...

#Compiling

To compile all necessary files, you need to go into the main directory and run:
- 'make': compiles all executables
- 'make install': moves the executables into the folder src
- 'make clean': removes all executables

#Usage
- './server user_file.txt log_file.txt': runs the server program. It needs a user_file to read/save users, and a log_file to save all logs created during its execution.
- './client username': runs the client program. It needs a username of a already registered user. For full client program usage type -h option.

#License

This is free and unencumbered software released into the public domain.

Anyone is free to copy, modify, publish, use, compile, sell, or
distribute this software, either in source code form or as a compiled
binary, for any purpose, commercial or non-commercial, and by any
means.

In jurisdictions that recognize copyright laws, the author or authors
of this software dedicate any and all copyright interest in the
software to the public domain. We make this dedication for the benefit
of the public at large and to the detriment of our heirs and
successors. We intend this dedication to be an overt act of
relinquishment in perpetuity of all present and future rights to this
software under copyright law.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

For more information, please refer to <http://unlicense.org>
