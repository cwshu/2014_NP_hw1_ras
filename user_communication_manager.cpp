
UserCommunicationManager::UserCommunicationManager(int max_user, const std::string& file_dir){
    this->max_user = max_user;
    file_exist = std::vector<std::vector<bool>>(max_user, std::vector<bool>(max_user, false));
    this->file_dir = file_dir;
}

std::string UserCommunicationManager::gen_file_name(int user_id_from, int user_id_to){
    if( user_id_from < 1 || user_id_from > max_user ){
        return "";
    }
    if( user_id_to < 1 || user_id_to > max_user ){
        return "";
    }
    
    return file_dir + "data_" + std::to_string(user_id_from) + "_to_" std::to_string(user_id_to);
}

bool UserCommunicationManager::is_exist(int user_id_from, int user_id_to){
    return file_exist[user_id_from-1][user_id_to-1];
}

int UserCommunicationManager::prepare_for_pass_in(int* error_code, int user_id_from, int user_id_to){
    // return fd;
    if( is_exist(user_id_from, user_id_to) ){
        if( error_code ){
            *error_code = __USER_CON_FILE_EXIST__;
            return -1;
        }
    }

    if( error_code ) *error_code = __USER_CON_NORMAL__;
    file_exist[user_id_from-1][user_id_to-1] = true;
    int fd = open(gen_file_name, O_WDONLY|O_CREAT|O_TRUNC , 0644)
    return fd;
}

int UserCommunicationManager::prepare_for_pass_out(int* error_code, int user_id_from, int user_id_to){ 
    // return fd;
    if( !is_exist(user_id_from, user_id_to) ){
        if( error_code ){
            *error_code = __USER_CON_FILE_NON_EXIST__;
            return -1;
        }
    }

    if( error_code ) *error_code = __USER_CON_NORMAL__;
    int fd = open(gen_file_name, O_RDONLY);
    return fd;
}

void UserCommunicationManager::clear_file(int user_id_from, user_id_to){
    file_exist[user_id_from-1][user_id_to-1] = false;
}
