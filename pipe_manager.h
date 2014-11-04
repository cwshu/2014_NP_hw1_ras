#ifndef __PIPE_MANAGER_H__
#define __PIPE_MANAGER_H__

#include <vector>
using namespace std;

/* struct AnonyPipe */
const int ANONY_PIPE_NORMAL     =  0;
const int ANONY_PIPE_FD_CLOSED  = -1;
const int ANONY_PIPE_NO_PIPE    = -2;
const int ANONY_PIPE_PIPE_EXIST = -4;

struct AnonyPipe{
    bool enable;
    bool fd_is_closed[2];
    int fds[2];

    AnonyPipe();
    int read_fd();
    int write_fd();
    int create_pipe();
    void close_read();
    void close_write();
    void close_pipe();
}; 

/* struct PipeManager */
/*
enum PipeManagerErrorCode {
    PIPE_MANAGER_NORMAL         = 0x00,
    PIPE_MANAGER_PIPE_EXIST     = 0x02,
    PIPE_MANAGER_PIPE_UNEXIST   = 0x04,
};*/

struct PipeManager{
    int cur_cmd_index;
    vector<AnonyPipe> cmd_input_pipes;

    PipeManager();
    bool cmd_has_pipe(int next_n_cmd);
    AnonyPipe& get_pipe(int next_n_cmd);
    void next_pipe();
};

#endif
