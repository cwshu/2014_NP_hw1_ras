#include <cstring>
#include <cerrno>

#include <unistd.h>

#include "pipe_manager.h"
#include "io_wrapper.h"

/* anony_pipe */
anony_pipe::anony_pipe(){
    enable = false;
    fd_is_closed[0] = false;
    fd_is_closed[1] = false;
    fds[0] = -1;
    fds[1] = -1;
}

int anony_pipe::read_fd(){
    if(!enable) return ANONY_PIPE_NO_PIPE;
    if(fd_is_closed[0]) return ANONY_PIPE_FD_CLOSED;
    return fds[0];
}

int anony_pipe::write_fd(){
    if(!enable) return ANONY_PIPE_NO_PIPE;
    if(fd_is_closed[1]) return ANONY_PIPE_FD_CLOSED;
    return fds[1];
}

int anony_pipe::create_pipe(){
    if(enable) return ANONY_PIPE_PIPE_EXIST;
    int ret = pipe(fds);
    if(ret == -1) perr_and_exit("pipe error: %s\n", strerror(errno));
    enable = true;
    return ANONY_PIPE_NORMAL;
}

void anony_pipe::close_read(){
    if(!enable) return;
    if(fd_is_closed[0]) return;
    int ret = close(fds[0]);
    if(ret == -1) perr_and_exit("close error: %s\n", strerror(errno));
    fd_is_closed[0] = true;
}

void anony_pipe::close_write(){
    if(!enable) return;
    if(fd_is_closed[1]) return;
    int ret = close(fds[1]);
    if(ret == -1) perr_and_exit("close error: %s\n", strerror(errno));
    fd_is_closed[1] = true;
}

void anony_pipe::close_pipe(){
    if(!enable) return;
    close_read();
    close_write();
    enable = false;
}

/* pipe_manager */
pipe_manager::pipe_manager(){
    cur_cmd_index = 0;
    cmd_input_pipes = vector<anony_pipe>(16, anony_pipe());
}

bool pipe_manager::cmd_has_pipe(int next_n_cmd){
    int cmd_index = cur_cmd_index + next_n_cmd;
    if( cmd_index+1 > cmd_input_pipes.size() )
        return false;
    return cmd_input_pipes[cmd_index].enable;
}

anony_pipe& pipe_manager::get_pipe(int next_n_cmd){
    int cmd_index = cur_cmd_index + next_n_cmd;
    return cmd_input_pipes[cmd_index];
}

void pipe_manager::next_pipe(){
    if( cur_cmd_index+1 > cmd_input_pipes.size() ){
        cmd_input_pipes.push_back(anony_pipe());
    }

    get_pipe(0).close_pipe();
    cur_cmd_index += 1;
}
