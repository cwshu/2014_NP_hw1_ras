#ifndef __USER_COMMUNICATION_MANAGER_H__
#define __USER_COMMUNICATION_MANAGER_H__

#include <vector>

const int __USER_CON_NORMAL__         = 0x00;
const int __USER_CON_FILE_EXIST__     = 0x01;
const int __USER_CON_FILE_NON_EXIST__ = 0x02;

struct UserCommunicationManager{
    int max_user;
    std::vector<std::vector<bool>> file_exist;
    std::string file_dir
    // id >= 1, <= max_user
    
    user_communication_manager(int max_user, const std::string& file_dir);
    std::string gen_file_name(int user_id_from, int user_id_to);
    bool is_exist(int user_id_from, int user_id_to);
    int prepare_for_pass_in(int* error_code, int user_id_from, int user_id_to);
    int prepare_for_pass_out(int* error_code, int user_id_from, int user_id_to);
    void clear_file(int user_id_from, user_id_to);
}
#endif /* end of include guard: __USER_COMMUNICATION_MANAGER_H__ */
