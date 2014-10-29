#include <unistd.h>

#include "pipe_manager.h"

/* anony_pipe */
anony_pipe::anony_pipe(){
    enable = false;
    read_fd = -1;
    write_fd = -1;
}

void anony_pipe::set_pipe(int read_fd, int write_fd){
    this->enable = true;
    this->read_fd = read_fd;
    this->write_fd = write_fd;
}

void anony_pipe::disable(){
    this->enable = false;
}

/* pipe_manager */
pipe_manager::pipe_manager(){
    cur_cmd_index = 0;
    pipe_of_unexecuted_cmds = vector<anony_pipe>(16, anony_pipe());
}

bool pipe_manager::cmd_has_pipe(int next_n_cmd){
    int cmd_index = cur_cmd_index + next_n_cmd;
    if( cmd_index+1 > pipe_of_unexecuted_cmds.size() )
        return false;
    return pipe_of_unexecuted_cmds[cmd_index].enable;
}

pipe_manager_error_code pipe_manager::create_pipe(int next_n_cmd){
    if( cmd_has_pipe(next_n_cmd) )
        return __PIPE_MANAGER_PIPE_EXIST__;

    int pipe_num[2];
    pipe(pipe_num);

    int cmd_index = cur_cmd_index + next_n_cmd;
    if( cmd_index+1 > pipe_of_unexecuted_cmds.size() )
        pipe_of_unexecuted_cmds.resize(cmd_index+1);

    pipe_of_unexecuted_cmds[cmd_index].set_pipe(pipe_num[0], pipe_num[1]);
    return __PIPE_MANAGER_NORMAL__;
}

pipe_manager_error_code pipe_manager::close_pipe(int next_n_cmd){
    if( !cmd_has_pipe(next_n_cmd) )
        return __PIPE_MANAGER_PIPE_UNEXIST__;

    int cmd_index = cur_cmd_index + next_n_cmd;
    int read_fd = pipe_of_unexecuted_cmds[cmd_index].read_fd;
    int write_fd = pipe_of_unexecuted_cmds[cmd_index].write_fd;
    pipe_of_unexecuted_cmds[cmd_index].disable();
    close(read_fd);
    close(write_fd);

    return __PIPE_MANAGER_NORMAL__;
}

void pipe_manager::next_pipe(){
    if( cur_cmd_index+1 > pipe_of_unexecuted_cmds.size() ){
        pipe_of_unexecuted_cmds.push_back(anony_pipe());
    }

    close_pipe(0);
    cur_cmd_index += 1;
}

