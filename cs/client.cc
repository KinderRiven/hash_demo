/*
 * @Author: your name
 * @Date: 2021-04-08 10:36:18
 * @LastEditTime: 2021-07-26 18:49:10
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: /HiKV+++/benchmark/cs/client.cc
 */
#include "common.h"

struct RequestContext {
public:
    int type;
    int complete;
    size_t start_time;
    size_t end_time;
};

struct ResultContext {
public:
    uint64_t num_insert_ok;
    uint64_t num_insert_error;
    uint64_t num_search_ok;
    uint64_t num_search_error;
};

struct ClientContext {
public:
    int thread_id;
    uint64_t base;
    uint64_t num_kv;

    std::string client_uri;
    std::string server_uri;

    erpc::Nexus* nexus;
    erpc::Rpc<erpc::CTransport>* rpc;
    erpc::MsgBuffer req;
    erpc::MsgBuffer resp;

    RequestContext request;
    ResultContext result;

public:
    ClientContext()
        : rpc(nullptr)
    {
        memset(&result, 0, sizeof(result));
        memset(&request, 0, sizeof(request));
    }
};

void sm_handler(int, erpc::SmEventType, erpc::SmErrType, void*) { }

void kv_cont_func(void* context, void* tag)
{
    ClientContext* _context = (ClientContext*)context;
    _context->request.end_time = erpc::rdtsc();
    _context->request.complete = 1;
    _context->result.num_insert_ok++;
}

static void run_client_thread(ClientContext* context)
{
    int _thread_id = context->thread_id;
    erpc::Nexus* _nexus = context->nexus;

    context->rpc = new erpc::Rpc<erpc::CTransport>(_nexus, (void*)context, _thread_id, sm_handler);
    erpc::Rpc<erpc::CTransport>* _rpc = context->rpc;
    int _session_num = _rpc->create_session(context->server_uri, _thread_id % kNumServerThread);
    printf("[%d][Session:%d]\n", _thread_id, _session_num);

    while (!_rpc->is_connected(_session_num)) {
        _rpc->run_event_loop_once();
    }
    printf("[%d][Connect Finished]\n", _thread_id);
    context->req = _rpc->alloc_msg_buffer_or_die(kMsgSize);
    context->resp = _rpc->alloc_msg_buffer_or_die(kMsgSize);

    uint64_t _base = context->base;
    size_t _start_time = erpc::rdtsc();
    for (uint64_t i = 1; i <= context->num_kv; i++) {
        context->request.complete = 0;
        // +++++++++++++++++++++++++++++++++++++++
        char* __dest = (char*)context->req.buf;
        *(uint64_t*)__dest = 1;
        __dest += kHeadSize;
        *(uint64_t*)__dest = _base;
        __dest += kKeySize;
        *(uint64_t*)__dest = _base;
        context->request.start_time = erpc::rdtsc();
        // +++++++++++++++++++++++++++++++++++++++
        _rpc->enqueue_request(_session_num, kInsertType, &context->req, &context->resp, kv_cont_func, nullptr);
        while (!context->request.complete) {
            _rpc->run_event_loop_once();
        }
        _base++;
        // +++++++++++++++++++++++++++++++++++++++
    }
    size_t _end_time = erpc::rdtsc();
    double _lat = erpc::to_usec(_end_time - _start_time, _rpc->get_freq_ghz());
    printf("[%d][%llu][time:%.2fseconds][iops:%.2f]\n", _thread_id, context->result.num_insert_ok, _lat / 1000000.0, 1000000.0 / _lat);
}

int main()
{
    std::string _client_uri = kClientHostname + ":" + std::to_string(kUDPPort);
    erpc::Nexus* _nexus = new erpc::Nexus(_client_uri, 0, 0);
    printf("ClientHostName:%s\n", _client_uri.c_str());

    std::thread _thread[128];
    std::string _server_uri = kServerHostname + ":" + std::to_string(kUDPPort);
    printf("ServerHostName:%s\n", _server_uri.c_str());

    for (int i = 0; i < kNumClientThread; i++) {
        ClientContext* __context = new ClientContext();
        __context->thread_id = i;
        __context->nexus = _nexus;
        __context->client_uri = _client_uri;
        __context->server_uri = _server_uri;
        __context->base = i * (kNumOpt / kNumClientThread);
        __context->num_kv = (kNumOpt / kNumClientThread);
        _thread[i] = std::thread(run_client_thread, __context);
    }
    for (int i = 0; i < kNumClientThread; i++) {
        _thread[i].join();
    }
}
