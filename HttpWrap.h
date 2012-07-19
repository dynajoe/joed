#ifndef HTTP_WRAP_H_
#define HTTP_WRAP_H_

#include "v8/include/v8.h"
#include "libuv/include/uv.h"

using namespace v8;

class HttpWrap
{
   uv_tcp_t* handle;
   
   public: 
      Persistent<Object> server;
      Persistent<Function> callback;
      HttpWrap(Handle<Context> context, const Arguments& args);
      ~HttpWrap();
      static void OnClose(uv_handle_t* handle);
      
   private: 
      static Handle<Value> Listen(const Arguments& args); 
      static void OnConnection(uv_stream_t* handle, int status);
      static uv_buf_t AllocConnection(uv_handle_t* handle, size_t suggested_size);
      static void OnRead(uv_stream_t* server, ssize_t nread, uv_buf_t buf);

};

#endif
