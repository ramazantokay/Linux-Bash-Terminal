#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <iostream>
#include <vector>
#include "parser.h"
#include "eshell.h"

using namespace std;

#define DEBUG_PRINT 0
#define DEBUG_COMMAND 1

int main(int argc, char *argv[])
{
    // parsing variables
    parsed_input input;
    string command;
    int num_cmd = 0;

    while (true)
    {
        cout << "/> ";
        // take input from user
        getline(cin, command);

        // parse the input
        if (parse_line(command.data(), &input))
        {
            // get the number of inputs
            num_cmd = input.num_inputs;

            // check if the command is quit
            if (input.inputs[0].type == INPUT_TYPE_COMMAND)
            {
                if (strcmp(input.inputs[0].data.cmd.args[0], "quit") == 0)
                {
                    if (DEBUG_PRINT)
                        cout << "Exiting" << endl;

                    exit(0);
                }
            }
            if (DEBUG_PRINT)
                cerr << "number of inputs in the command " << num_cmd << endl;

            if (num_cmd)
            {
                // check the commad is single pipeline or sequential or parallel command or mixed

                int num_subshells = 0;
                for (int i = 0; i < num_cmd; i++)
                {
                    if (input.inputs[i].type == INPUT_TYPE_SUBSHELL)
                    {
                        num_subshells++;
                    }
                }
                if (DEBUG_PRINT)
                    cerr << "number of subshells " << num_subshells << endl;

               
                    if (DEBUG_COMMAND)
                    {

                        // SINGLE COMMAND --> ls -l
                        if (input.separator == SEPARATOR_NONE)
                        {
                            // single pipeline
                            if (DEBUG_PRINT)
                                cerr << "SINGLE COMMAND" << endl;
                            single_command(input);
                        }
                        // PIPELINE --> ls -l | tr /a-z/ /A-Z/
                        if (input.separator == SEPARATOR_PIPE)
                        {
                            if (DEBUG_PRINT)
                                cerr << "PIPE" << endl;
                            pipe_command(num_cmd, input);
                        }
                        // SEQUENTIAL --> ls -l ; tr /a-z/ /A-Z/ ; echo "Done." --> A | B ; C | D | E ; F
                        if (input.separator == SEPARATOR_SEQ)
                        {
                            if (DEBUG_PRINT)
                                cerr << "SEQ" << endl;

                            for (int i = 0; i < num_cmd; i++)
                            {
                                single_input *current_input = &input.inputs[i];

                                if (current_input->type == INPUT_TYPE_COMMAND)
                                {
                                    single_command(*current_input);
                                }
                                else if (current_input->type == INPUT_TYPE_PIPELINE)
                                {
                                    // pipe_command(current_input->data.pline.num_commands, input);
                                    pipe_command(current_input->data.pline.num_commands, *current_input);
                                }
                            }
                        }
                        // PARALLEL --> ls -l , tr /a-z/ /A-Z/ , echo "Done."  --> A | B , C | D | E , F
                        if (input.separator == SEPARATOR_PARA)
                        {
                            if (DEBUG_PRINT)
                            {
                                cerr << "PARALLEL" << endl;
                                cerr << "number of commands " << num_cmd << endl;
                            }

                            int num_fork = 0;
                            for (int i = 0; i < num_cmd; i++)
                            {
                                single_input *current_input = &input.inputs[i];
                                if (current_input->type == INPUT_TYPE_COMMAND)
                                {
                                    single_command_wo_wait(*current_input);
                                    num_fork++;
                                }
                                else if (current_input->type == INPUT_TYPE_PIPELINE)
                                {
                                    pipe_command_wo_wait(current_input->data.pline.num_commands, *current_input);
                                    num_fork += current_input->data.pline.num_commands;
                                }
                            }
                            if (DEBUG_PRINT)
                                cerr << "number of fork " << num_fork << endl;

                            // wait for pipe commands
                            for (int i = 0; i < num_fork; i++)
                            {
                                int status;
                                if (DEBUG_PRINT)
                                    cerr << "waiting for the child process to finish " << i << endl;
                                wait(NULL);
                                if (WIFEXITED(status))
                                {
                                    if (DEBUG_PRINT)
                                        cerr << "Child process exited with status: " << WEXITSTATUS(status) << std::endl;
                                }
                                else if (WIFSIGNALED(status))
                                {
                                    if (DEBUG_PRINT)
                                        cerr << "Child process terminated by signal: " << WTERMSIG(status) << std::endl;
                                }
                            }
                        }

                        // SINGLE SUBSHELL ( A | B | C | D ) or ( A ; B ; C ; D ) or ( A , B , C , D )
                        // ( A | B ; C | D | E ; F ; G | H )
                        // ( A | B , C | D , E ; F , G | H )
                        if (input.inputs[0].type == INPUT_TYPE_SUBSHELL)
                        {
                            if (DEBUG_PRINT)
                                cerr << "SINGLE SUBSHELL" << endl;
                            // parse again the subshell
                            parsed_input subshell_input;
                            if (parse_line(input.inputs[0].data.subshell, &subshell_input))
                            {
                                // pretty_print(&subshell_input);
                                // single subshell with pipe ( A | B | C | D )
                                if (subshell_input.separator == SEPARATOR_PIPE)
                                {
                                    if (DEBUG_PRINT)
                                    {
                                        cerr << "SINGLE SUBSHELL PIPE" << endl;
                                        cerr << "number of commands in the subshell " << subshell_input.num_inputs << endl;
                                    }
                                    pipe_command(subshell_input.num_inputs, subshell_input);

                                    // Subshell pipeline with mixed types(sequential-pipeline or parallel pipeline) without the need for repeaters:
                                    // ( A | B , C ) | ( D | E ) | ( F ; G ; H | I )

                                    // @TODO: implement repeaters for subshell
                                }
                                // single subshell with seq ( A ; B ; C ; D )
                                if (subshell_input.separator == SEPARATOR_SEQ)
                                {
                                    if (DEBUG_PRINT)
                                    {
                                        cerr << "SINGLE SUBSHELL SEQ" << endl;
                                        cerr << "number of commands in the subshell " << subshell_input.num_inputs << endl;
                                    }

                                    for (int i = 0; i < subshell_input.num_inputs; i++)
                                    {
                                        single_input *current_input = &subshell_input.inputs[i];

                                        if (current_input->type == INPUT_TYPE_COMMAND)
                                        {
                                            single_command(*current_input);
                                        }
                                        else if (current_input->type == INPUT_TYPE_PIPELINE)
                                        {
                                            pipe_command(current_input->data.pline.num_commands, *current_input);
                                        }
                                    }
                                }
                                // single subshell with parallel ( A , B , C , D )
                                if (subshell_input.separator == SEPARATOR_PARA)
                                {
                                    if (DEBUG_PRINT)
                                    {
                                        cerr << "SINGLE SUBSHELL PARA" << endl;
                                        cerr << "number of commands in the subshell " << subshell_input.num_inputs << endl;
                                    }
                                    int num_fork = 0;
                                    for (int i = 0; i < subshell_input.num_inputs; i++)
                                    {
                                        single_input *current_input = &subshell_input.inputs[i];
                                        if (current_input->type == INPUT_TYPE_COMMAND)
                                        {
                                            single_command_wo_wait(*current_input);
                                            num_fork++;
                                        }
                                        else if (current_input->type == INPUT_TYPE_PIPELINE)
                                        {
                                            pipe_command_wo_wait(current_input->data.pline.num_commands, *current_input);
                                            num_fork += current_input->data.pline.num_commands;
                                        }
                                    }
                                    if (DEBUG_PRINT)
                                        cerr << "number of fork " << num_fork << endl;

                                    // wait for pipe commands
                                    for (int i = 0; i < num_fork; i++)
                                    {
                                        int status;
                                        if (DEBUG_PRINT)
                                            cerr << "waiting for the child process to finish " << i << endl;
                                        wait(NULL);
                                        if (WIFEXITED(status))
                                        {
                                            if (DEBUG_PRINT)
                                                cerr << "Child process exited with status: " << WEXITSTATUS(status) << std::endl;
                                        }
                                        else if (WIFSIGNALED(status))
                                        {
                                            if (DEBUG_PRINT)
                                                cerr << "Child process terminated by signal: " << WTERMSIG(status) << std::endl;
                                        }
                                    }
                                }
                            }

                            free_parsed_input(&subshell_input);
                        }
                    }
            }

            if (DEBUG_PRINT)
                pretty_print(&input);

            free_parsed_input(&input);
        }
        else
        {

            // may be add some error message and quit
            cerr << "Error with parsing" << endl;

        }
    }

    return 0;
}
void pipe_command(int num_cmd, parsed_input &input)
{
    int num_pipes = num_cmd - 1;
    int proc_fd[num_pipes][2];
    pid_t proc_pids[num_cmd];

    // create pipes
    for (int i = 0; i < num_pipes; i++)
    {
        if (pipe(proc_fd[i]) < 0)
        {
            perror("pipe errorrr!!!!!");
            exit(1);
        }

        if (DEBUG_PRINT)
            cerr << i << ".th pipe created" << endl;
    }

    // fork the processes
    for (int i = 0; i < num_cmd; i++)
    {
        if (DEBUG_PRINT)
            cerr << "parent process " << getpid() << endl;

        // @TODO: check if the fork is successful
        if ((proc_pids[i] = fork()) == 0)
        {
            if (DEBUG_PRINT)
                cerr << "child process " << getpid() << endl;
            // first command

            if (i < num_pipes)
            {    
                dup2(proc_fd[i][1], 1);
            }

            if (i > 0)
            {
                dup2(proc_fd[i - 1][0], 0);
            }
            for (int j = 0; j < num_pipes; j++)
            {
                close(proc_fd[j][0]);
                close(proc_fd[j][1]);
            }

            if (DEBUG_PRINT)
            {
                cerr << "executing the command: " << input.inputs[i].data.cmd.args[0] << endl;
            }
            execvp(input.inputs[i].data.cmd.args[0], input.inputs[i].data.cmd.args);
            perror("execvp error");
        }
    }

    for (int i = 0; i < num_pipes; i++)
    {
        close(proc_fd[i][0]);
        close(proc_fd[i][1]);
    }

    for (int i = 0; i < num_cmd; i++)
    {
        int status;
        if (DEBUG_PRINT)
            cerr << "waiting for the child process to finish " << i << endl;
        waitpid(proc_pids[i], &status, 0);
        if (WIFEXITED(status))
        {
            if (DEBUG_PRINT)
                cerr << "Child process exited with status: " << WEXITSTATUS(status) << std::endl;
        }
        else if (WIFSIGNALED(status))
        {
            if (DEBUG_PRINT)
                cerr << "Child process terminated by signal: " << WTERMSIG(status) << std::endl;
        }
    }
}

void pipe_command(int num_cmd, single_input &input)
{
    int num_pipes = num_cmd - 1;
    int proc_fd[num_pipes][2];
    pid_t proc_pids[num_cmd];

    // create pipes
    for (int i = 0; i < num_pipes; i++)
    {
        if (pipe(proc_fd[i]) < 0)
        {
            perror("pipe errorrr!!!!!");
            exit(1);
        }

        if (DEBUG_PRINT)
            cerr << i << ".th pipe created" << endl;
    }

    // fork the processes
    for (int i = 0; i < num_cmd; i++)
    {
        if (DEBUG_PRINT)
            cerr << "parent process " << getpid() << endl;

        // @TODO: check if the fork is successful
        if ((proc_pids[i] = fork()) == 0)
        {
            if (DEBUG_PRINT)
                cerr << "child process " << getpid() << endl;
            // first command

            if (i < num_pipes)
            {

                // @TODO: will add error check
                dup2(proc_fd[i][1], 1);
            }

            if (i > 0)
            {
                // @TODO: will add error check
                dup2(proc_fd[i - 1][0], 0);
            }
            for (int j = 0; j < num_pipes; j++)
            {
                close(proc_fd[j][0]);
                close(proc_fd[j][1]);
            }

            if (DEBUG_PRINT)
            {
                cerr << "executing the command: " << input.data.pline.commands[i].args << endl;
            }
            execvp(input.data.pline.commands[i].args[0], input.data.pline.commands[i].args);
            perror("execvp error");
        }
    }

    for (int i = 0; i < num_pipes; i++)
    {
        close(proc_fd[i][0]);
        close(proc_fd[i][1]);
    }

    for (int i = 0; i < num_cmd; i++)
    {
        int status;
        if (DEBUG_PRINT)
            cerr << "waiting for the child process to finish " << i << endl;
        waitpid(proc_pids[i], &status, 0);
        if (WIFEXITED(status))
        {
            if (DEBUG_PRINT)
                cerr << "Child process exited with status: " << WEXITSTATUS(status) << std::endl;
        }
        else if (WIFSIGNALED(status))
        {
            if (DEBUG_PRINT)
                cerr << "Child process terminated by signal: " << WTERMSIG(status) << std::endl;
        }
    }
}

void pipe_command_wo_wait(int num_cmd, single_input &input)
{
    int num_pipes = num_cmd - 1;
    int proc_fd[num_pipes][2];
    pid_t proc_pids[num_cmd];

    // create pipes
    for (int i = 0; i < num_pipes; i++)
    {
        if (pipe(proc_fd[i]) < 0)
        {
            perror("pipe errorrr!!!!!");
            exit(1);
        }

        if (DEBUG_PRINT)
            cerr << i << ".th pipe created" << endl;
    }

    // fork the processes
    for (int i = 0; i < num_cmd; i++)
    {
        if (DEBUG_PRINT)
            cerr << "parent process " << getpid() << endl;

        // @TODO: check if the fork is successful
        if ((proc_pids[i] = fork()) == 0)
        {
            if (DEBUG_PRINT)
                cerr << "child process " << getpid() << endl;
            // first command

            if (i < num_pipes)
            {

                // @TODO: will add error check
                dup2(proc_fd[i][1], 1);
            }

            if (i > 0)
            {
                // @TODO: will add error check
                dup2(proc_fd[i - 1][0], 0);
            }
            for (int j = 0; j < num_pipes; j++)
            {
                close(proc_fd[j][0]);
                close(proc_fd[j][1]);
            }

            if (DEBUG_PRINT)
            {
                cerr << "executing the command: " << input.data.pline.commands[i].args << endl;
            }
            execvp(input.data.pline.commands[i].args[0], input.data.pline.commands[i].args);
            perror("execvp error");
        }
    }

    for (int i = 0; i < num_pipes; i++)
    {
        close(proc_fd[i][0]);
        close(proc_fd[i][1]);
    }
}

void single_command(parsed_input &input)
{
    if (input.inputs[0].type == INPUT_TYPE_COMMAND)
    {
        pid_t pid;
        if (pid = fork()) // parent
        {
            // wait for the child process to finish
            wait(NULL);
        }
        else // child
        {
            // execute the command
            execvp(input.inputs[0].data.cmd.args[0], input.inputs[0].data.cmd.args);
        }
    }
}

void single_command(single_input &input)
{
    if (input.type == INPUT_TYPE_COMMAND)
    {
        pid_t pid;
        if (pid = fork()) // parent
        {
            // wait for the child process to finish
            wait(NULL);
        }
        else // child
        {
            // execute the command
            execvp(input.data.cmd.args[0], input.data.cmd.args);
        }
    }
}

void single_command_wo_wait(single_input &input)
{
    if (input.type == INPUT_TYPE_COMMAND)
    {
        pid_t pid;
        if ((pid = fork()) == 0)
        {
            // execute the command
            execvp(input.data.cmd.args[0], input.data.cmd.args);
        }
    }
}
