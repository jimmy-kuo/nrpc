
/***********************************************
  File name		: controller.h
  Create date	: 2015-12-02 23:47
  Modified date : 2016-01-05 00:57
  Author		: zmkeil, alibaba.inc
  Express : 
  
 **********************************************/

#ifndef NRPC_CONTROLLER_H
#define NRPC_CONTROLLER_H

extern "C" {
#include <nginx.h>
#include <ngx_core.h>
}
#include <google/protobuf/descriptor.h>
#include <google/protobuf/message.h>
#include "iobuf_zero_copy_stream.h"
#include "service_set.h"
#include "server.h"
#include "protocol.h"
#include "service_context.h"
#include "info_log_context.h"

namespace nrpc
{

enum RPC_SESSION_STATE {
    RPC_SESSION_READING_REQUEST = 0,
    RPC_SESSION_PROCESSING,
    RPC_SESSION_SENDING_RESPONSE,
    RPC_SESSION_LOGING,
    RPC_SESSION_OVER
};

enum RPC_RESULT {
    // inner error, just close connection
    RPC_INNER_ERROR = 0,
    // first two result, just close connection
    // don't send response
    RPC_READ_ERROR,
    RPC_READ_TIMEOUT,
    // set response_meta's error code/text
    RPC_PROCESS_ERROR,
    RPC_PROCESS_TIMEOUT,
    // close connection
    RPC_SEND_ERROR,
    RPC_SEND_TIMEOUT,
    // send all response_payload to TCP_BUFFER,
    // cann't ensure client receive it correctly,
    // in server end, this RPC is ok
    RPC_OK
};

class Controller : public google::protobuf::RpcController
{
public:
    Controller();
    virtual ~Controller();

    // Client-side methods ---------------------------------------------
    // These calls may be made from the client side only.  Their results
    // are undefined on the server side (may crash).

    // Resets the RpcController to its initial state so that it may be reused in
    // a new call.  Must not be called while an RPC is in progress.
    virtual void Reset() {return;}

    // After a call has finished, returns true if the call failed.  The possible
    // reasons for failure depend on the RPC implementation.  Failed() must not
    // be called before a call has finished.  If Failed() returns true, the
    // contents of the response message are undefined.
    virtual bool Failed() const {return true;}

    // If Failed() is true, returns a human-readable description of the error.
    virtual std::string ErrorText() const {
        // before (and in) protocol->process_response (in other words, in rpc frame),
        // return the _result_text; otherwise, the rpc procedure is successly completed,
        // than get error_code and error_text from rpc_meta->response
        // TODO: client two-level ErrorText
        return std::string("OK");
    }

    // Advises the RPC system that the caller desires that the RPC call be
    // canceled.  The RPC system may cancel it immediately, may wait awhile and
    // then cancel it, or may not even cancel the call at all.  If the call is
    // canceled, the "done" callback will still be called and the RpcController
    // will indicate that the call failed at that time.
    virtual void StartCancel() {return;}


    // Server-side methods ---------------------------------------------
    // These calls may be made from the server side only.  Their results
    // are undefined on the client side (may crash).
    // Server-side methods ---------------------------------------------

    // Causes Failed() to return true on the client side.  "reason" will be
    // incorporated into the message returned by ErrorText().  If you find
    // you need to return machine-readable information about failures, you
    // should incorporate it into your response protocol buffer and should
    // NOT call SetFailed().
    // In user_defined service->method, use this api to set the reson into rpc_meta's
    // error_code and error_text; in rpc frame, we usually close the connection when error
    // TODO: server set reason into rpc_meta->response
    virtual void SetFailed(const std::string& reason) {
        (void) reason;
        return;
    }

    // If true, indicates that the client canceled the RPC, so the server may
    // as well give up on replying to it.  The server should still call the
    // final "done" callback.
    virtual bool IsCanceled() const {return true;}

    // Asks that the given callback be called when the RPC is canceled.  The
    // callback will always be called exactly once.  If the RPC completes without
    // being canceled, the callback will be called after completion.  If the RPC
    // has already been canceled when NotifyOnCancel() is called, the callback
    // will be called immediately.
    //
    // NotifyOnCancel() must be called no more than once per request.
    virtual void NotifyOnCancel(google::protobuf::Closure* callback) {
        (void) callback;
        return;
    }

    // server side init, get _server, _service_set from c
    bool server_side_init(ngx_connection_t* c);

    // service_set and ngx_connection
    bool set_service_set(ServiceSet* service_set) {
        _service_set = service_set;
        return true;
    }
    ServiceSet* service_set() {
        return _service_set;
    }
    ngx_connection_t* connection() {
        return _ngx_connection;
    }

    // for server options
    ServiceContext* service_context();
    int server_read_timeout();

    // -------------------------------------------------------------------
    //                      Both-side methods.
    // Following methods can be called from both client and server. But they
    // may have different or opposite semantics.
    // -------------------------------------------------------------------
    // session state and result
    bool set_state(RPC_SESSION_STATE state) {
        _state = state;
        return true;
    }
    bool set_result(RPC_RESULT result) {
        _result = result;
        return true;
    }
    bool set_result_text(const char* result_text) {
        _result_text = result_text;
        return true;
    }
    RPC_SESSION_STATE state() {
        return _state;
    }

    // protocol
    bool set_protocol(unsigned protocol_num);

    Protocol* protocol() {
        return _protocol;
    }
    void* protocol_ctx() {
        return _protocol_ctx;
    }
 
    // rpc data
    void set_request(google::protobuf::Message* request) {
        _request = request;
    }
    google::protobuf::Message* request() {
        return _request;
    }
    void set_response(google::protobuf::Message* response) {
        _response = response;
    }
    google::protobuf::Message* response() {
        return _response;
    }


    // iobuf of the rpc procedure
    bool set_iobuf(ngxplus::IOBuf* iobuf) {
        _iobuf = iobuf;
        return true;
    }
    ngxplus::IOBuf* iobuf() {
        return _iobuf;
    }

    // stastics
    int/*base::EndPoint*/ remote_side() const {
        return _remote_side;
    }
    int/*base::EndPoint*/ local_side() const {
        return _local_side;
    }
    void set_process_start_time(long start_process_us) {
        _start_process_us = start_process_us;
    }
    long process_start_time() {
        return _start_process_us;
    }

    // finalize
    void finalize();

private:
    // rpc_frame describes for both of server and client side;
    // the order of _state is different between server and client side;
    // the _result and _result_text describe the rpc_frame errors,
    // the user_define rpc_service errors are transmited from server to client
    // by rpc_meta->response
    RPC_SESSION_STATE _state;
    RPC_RESULT _result;
    const char* _result_text;

    // protocol for both of server and client side
    Protocol* _protocol;
    void* _protocol_ctx;
    const char* _protocol_name;

    // rpc data for both of server and client side
    // In server: set req and resp in protocol.process_request, then excute user_defined
    // method, and then get resp in default_send_rpc_response and send it to client.
    // In client: set resp in stud->method(Channel->CallMethod), then get in protocol.
    // process_response
    google::protobuf::Message* _request;
    google::protobuf::Message* _response;

    // iobuf for both of server and client side
    // In server: first >> iobuf and then << iobuf, In client: excuted in contrast
    // destory it in finalize()
    ngxplus::IOBuf* _iobuf;

    // stastics for both of server and client side
    long _start_process_us;
    int _remote_side;
    int _local_side;


    // server and service informations for server side
    ServiceSet* _service_set;
    Server* _server;
    ServiceContext* _service_context;
    ngx_connection_t* _ngx_connection;
};

}
#endif
