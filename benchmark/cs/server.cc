/*
 * @Author: your name
 * @Date: 2021-04-08 10:36:18
 * @LastEditTime: 2021-07-22 19:40:35
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: /code/eRPC/hello_world/server.cc
 */
#include "common.h"
erpc::Rpc<erpc::CTransport>* rpc;

void req_handler(erpc::ReqHandle* req_handle, void*)
{
    auto& resp = req_handle->pre_resp_msgbuf;
    rpc->resize_msg_buffer(&resp, kMsgSize);
    sprintf(reinterpret_cast<char*>(resp.buf), "hello");
    rpc->enqueue_response(req_handle, &resp);
}

int main()
{
    std::string server_uri = kServerHostname + ":" + std::to_string(kUDPPort);
    erpc::Nexus nexus(server_uri, 0, 0);
    nexus.register_req_func(kReqType, req_handler);
    rpc = new erpc::Rpc<erpc::CTransport>(&nexus, nullptr, 0, nullptr);
    rpc->run_event_loop(100000);
}