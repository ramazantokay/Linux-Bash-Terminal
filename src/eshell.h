#pragma once
void single_command(parsed_input &input);
void single_command(single_input &input);
void single_command_wo_wait(single_input &input);

void pipe_command(int num_cmd, parsed_input &input);
void pipe_command(int num_cmd, single_input &input);
void pipe_command_wo_wait(int num_cmd, single_input &input);
