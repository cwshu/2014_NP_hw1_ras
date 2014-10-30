#ifndef __PIPE_MANAGER_H__
#define __PIPE_MANAGER_H__

#include <vector>
using namespace std;

/* struct anony_pipe */
const int ANONY_PIPE_NORMAL     =  0;
const int ANONY_PIPE_FD_CLOSED  = -1;
const int ANONY_PIPE_NO_PIPE    = -2;
const int ANONY_PIPE_PIPE_EXIST = -4;

struct anony_pipe {
    bool enable;
    bool fd_is_closed[2];
    int fds[2];

    anony_pipe();
    int read_fd();
    int write_fd();
    int create_pipe();
    void close_read();
    void close_write();
    void close_pipe();
}; 

/* struct pipe_manager */
/*
enum pipe_manager_error_code {
    PIPE_MANAGER_NORMAL         = 0x00,
    PIPE_MANAGER_PIPE_EXIST     = 0x02,
    PIPE_MANAGER_PIPE_UNEXIST   = 0x04,
};*/

struct pipe_manager {
    int cur_cmd_index;
    vector<anony_pipe> cmd_input_pipes;

    pipe_manager();
    bool cmd_has_pipe(int next_n_cmd);
    anony_pipe& get_pipe(int next_n_cmd);
    void next_pipe();
};

#endif
