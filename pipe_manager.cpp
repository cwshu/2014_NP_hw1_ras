#include <cstring>
#include <cerrno>

#include <unistd.h>

#include "pipe_manager.h"
#include "io_wrapper.h"

/* AnonyPipe */
AnonyPipe::AnonyPipe(){
    enable = false;
    fd_is_closed[0] = false;
    fd_is_closed[1] = false;
    fds[0] = -1;
    fds[1] = -1;
}

int AnonyPipe::read_fd(){
    if(!enable) return ANONY_PIPE_NO_PIPE;
    if(fd_is_closed[0]) return ANONY_PIPE_FD_CLOSED;
    return fds[0];
}

int AnonyPipe::write_fd(){
    if(!enable) return ANONY_PIPE_NO_PIPE;
    if(fd_is_closed[1]) return ANONY_PIPE_FD_CLOSED;
    return fds[1];
}

int AnonyPipe::create_pipe(){
    if(enable) return ANONY_PIPE_PIPE_EXIST;
    int ret = pipe(fds);
    if(ret == -1) perror_and_exit("pipe error");
    enable = true;
    fd_is_closed[0] = false;
    fd_is_closed[1] = false;
    return ANONY_PIPE_NORMAL;
}

void AnonyPipe::close_read(){
    if(!enable) return;
    if(fd_is_closed[0]) return;
    int ret = close(fds[0]);
    if(ret == -1) perror_and_exit("close error");
    fd_is_closed[0] = true;
}

void AnonyPipe::close_write(){
    if(!enable) return;
    if(fd_is_closed[1]) return;
    int ret = close(fds[1]);
    if(ret == -1) perror_and_exit("close error");
    fd_is_closed[1] = true;
}

void AnonyPipe::close_pipe(){
    if(!enable) return;
    close_read();
    close_write();
    enable = false;
}

/* PipeManager */
PipeManager::PipeManager(){
    cur_cmd_index = 0;
    cmd_input_pipes = vector<AnonyPipe>(16, AnonyPipe());
}

bool PipeManager::cmd_has_pipe(int next_n_cmd){
    int cmd_index = cur_cmd_index + next_n_cmd;
    if( cmd_index+1 > cmd_input_pipes.size() )
        return false;

    return cmd_input_pipes[cmd_index].enable;
}

AnonyPipe& PipeManager::get_pipe(int next_n_cmd){
    int cmd_index = cur_cmd_index + next_n_cmd;
    if( cmd_index+1 > cmd_input_pipes.size() )
        cmd_input_pipes.resize(cmd_index+1);

    return cmd_input_pipes[cmd_index];
}

void PipeManager::next_pipe(){
    if( cur_cmd_index+1 > cmd_input_pipes.size() ){
        cmd_input_pipes.push_back(AnonyPipe());
    }

    get_pipe(0).close_pipe();
    cur_cmd_index += 1;
}
