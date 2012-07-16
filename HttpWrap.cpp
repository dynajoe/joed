#include "HttpWrap.h"
#include <cstdlib>
#include "v8/include/v8.h"
#include "libuv/include/uv.h"

using namespace v8;

HttpWrap::HttpWrap(const Arguments& args)
{
   Local<ObjectTemplate> serverTemplate = ObjectTemplate::New();
   serverTemplate->SetInternalFieldCount(1);
   serverTemplate->Set(String::New("listen"), FunctionTemplate::New(Listen));

   Local<Object> localServer = serverTemplate->NewInstance();
   localServer->SetInternalField(0, External::New(this));

   String::AsciiValue ip_address(args[0]);
   int port = args[1]->Int32Value();

   handle = (uv_tcp_t*) malloc(sizeof(uv_tcp_t));
   uv_tcp_init(uv_default_loop(), handle);
   handle->data = this;
   uv_tcp_bind(handle, uv_ip4_addr(*ip_address, port));
   
   server = Persistent<Object>::New(localServer);
}

Handle<Value> HttpWrap::Listen(const Arguments& args)
{
   puts("HttpWrap::Listen");

   HandleScope scope;
   
   Local<External> wrap = Local<External>::Cast(args.Holder()->GetInternalField(0));
   HttpWrap* httpWrap = static_cast<HttpWrap*>(wrap->Value());
   httpWrap->server->Set(String::New("onrequest"), args[0]);
   uv_listen((uv_stream_t*) httpWrap->handle, 128, OnConnection);

   return scope.Close(Undefined());
}

void HttpWrap::OnConnection(uv_stream_t* handle, int status)
{
   puts("HttpWrap::OnConnection");
   uv_tcp_t* client = (uv_tcp_t*) malloc(sizeof(uv_tcp_t));
   uv_tcp_init(uv_default_loop(), client);
   client->data = handle->data;
   uv_accept(handle, (uv_stream_t*) client);
   uv_read_start((uv_stream_t*) client, AllocConnection, OnRead);
}

uv_buf_t HttpWrap::AllocConnection(uv_handle_t* handle, size_t suggested_size)
{
   puts("HttpWrap::AllocConnection");
   return uv_buf_init((char*) malloc(suggested_size), suggested_size);
}

void HttpWrap::OnRead(uv_stream_t* server, ssize_t nread, uv_buf_t buf)
{
   HandleScope scope;
   puts("HttpWrap::OnRead");

   HttpWrap* httpWrap = static_cast<HttpWrap*>(server->data);
   Local<String> callbackSym = String::New("onrequest");
   Local<Value> value = httpWrap->server->Get(callbackSym);

   if (!value->IsFunction())
   {
      puts("Value is not a function");
   }
   else 
   {
      puts("Value is a function");
      Local<Function> callback = Local<Function>::Cast(value);
      Local<Value>* argv = new Local<Value>[1];
      argv[0] = String::New("test");
      puts("Invoking callback");
      callback->Call(Context::GetCurrent()->Global(), 1, argv);
      puts("invoked callback");
   }
   
   free(buf.base);
   uv_close((uv_handle_t*) server, OnClose);
}

void HttpWrap::OnClose(uv_handle_t* handle)
{
   puts("HttpWrap::OnClose");
}  
