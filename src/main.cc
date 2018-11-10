#define _GNU_SOURCE 1
#include <string>
#include <iostream>
#include <stdio.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <sys/wait.h>
#include <sys/user.h>
#include <unistd.h>
#include <sys/ptrace.h>
#include <stdlib.h>
#include <syscall.h>
#include <stdint.h>
#include "memory_mapping.h"
#include "debugger-dwarf.h"
#include "parser.h"

static int exec_and_trace(int argc, char **argv)
{
    ptrace(PTRACE_TRACEME, 0, NULL, NULL);

    if (argc == 2)
        return execvp(argv[1], nullptr);
    else
        return execvp(argv[1], argv + 2);
}

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        std::cerr <<  "too few args" << std::endl;
        return 1;
    }

    int pid;

    if (!(pid = fork()))
    {
        if (exec_and_trace(argc, argv) == -1)
        {
            perror("Could not exec the program");
            return 1;
        }
    }
    else
    {
        int status;
        waitpid(pid, &status, 0);
        if (WIFEXITED(status))
            return 1;
        ptrace(PTRACE_SINGLESTEP, pid, NULL, NULL);
        
        if (contains_debug_info(argv[1]))
        {
            DebuggerDwarf dbg(pid, argv[1]);
            Parser parser(dbg);
            std::cout << "Reading debug info" << std::endl;
            auto res = dbg.find_source_file("main").value();
            std::cout << "printer" << res.first << res.second << std::endl;
            parser.input_loop();
        }
        else
        {
            Parser parser(Debugger(pid, argv[1]));
            parser.input_loop();
        }
    }
    return 0;
}
