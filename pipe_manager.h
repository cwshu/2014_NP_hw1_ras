#ifndef __PIPE_MANAGER_H__
#define __PIPE_MANAGER_H__

#include <vector>
using namespace std;

enum pipe_manager_error_code {
    __PIPE_MANAGER_NORMAL__       = 0x00,
    __PIPE_MANAGER_PIPE_EXIST__   = 0x02,
    __PIPE_MANAGER_PIPE_UNEXIST__ = 0x04,
};

struct anony_pipe {
    bool enable;
    int read_fd;
    int write_fd;

    anony_pipe();
    void set_pipe(int read_fd, int write_fd);
    void disable();
}; 

struct pipe_manager {
    int cur_cmd_index;
    vector<anony_pipe> pipe_of_unexecuted_cmds;

    pipe_manager();
    bool cmd_has_pipe(int next_n_cmd);
    pipe_manager_error_code create_pipe(int next_n_cmd);
    pipe_manager_error_code close_pipe(int next_n_cmd);
    void next_pipe();

    int read_pipe_fd(int next_n_cmd);
    int write_pipe_fd(int next_n_cmd);
};

#endif
