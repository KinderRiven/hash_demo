/*
 * @Author: your name
 * @Date: 2021-04-08 10:36:18
 * @LastEditTime: 2021-07-26 11:03:19
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: /HiKV+++/benchmark/cs/client.cc
 */
#include "common.h"

erpc::Rpc<erpc::CTransport>* rpc = nullptr;
erpc::MsgBuffer req;
erpc::MsgBuffer resp;

void cont_func(void* context, void* tag)
{
    printf("cont_func.\n");
}

void sm_handler(int, erpc::SmEventType, erpc::SmErrType, void*) { }

int main()
{
    std::string client_uri = kClientHostname + ":" + std::to_string(kUDPPort);
    printf("ClientHostName:%s\n", client_uri.c_str());

    erpc::Nexus nexus(client_uri, 0, 0);
    rpc = new erpc::Rpc<erpc::CTransport>(&nexus, nullptr, 0, sm_handler);

    std::string server_uri = kServerHostname + ":" + std::to_string(kUDPPort);
    printf("ServerHostName:%s\n", server_uri.c_str());

    int session_num = rpc->create_session(server_uri, 0);
    printf("Session:%d.\n", session_num);

    while (!rpc->is_connected(session_num)) {
        rpc->run_event_loop_once();
    }
    printf("Connect Finished.\n");

    req = rpc->alloc_msg_buffer_or_die(kMsgSize);
    resp = rpc->alloc_msg_buffer_or_die(kMsgSize);

    char* _src = (char*)req.buf;
    *(uint32_t*)_src = 1;
    rpc->enqueue_request(session_num, kInsertType, &req, &resp, cont_func, nullptr);
    rpc->run_event_loop(1000000);
    delete rpc;
}
