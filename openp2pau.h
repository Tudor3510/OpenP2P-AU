#ifndef OPENP2PAU_H
#define OPENP2PAU_H


const char* get_error_message_AU(int error_code);
int connect_AU(const char* ip, const char* port, const char* client_name);

#endif