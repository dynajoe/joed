#include "HttpWrap.h"
#include <cstdlib>
#include "v8/include/v8.h"
#include "libuv/include/uv.h"

using namespace v8;

HttpWrap::HttpWrap(Handle<Context> context, const Arguments& args)
{
   Local<ObjectTemplate> serverTemplate = ObjectTemplate::New();
   serverTemplate->SetInternalFieldCount(1);
   serverTemplate->Set(String::New("listen"), FunctionTemplate::New(Listen));

   Local<Object> localServer = serverTemplate->NewInstance();
   localServer->SetInternalField(0, External::New(this));

   String::AsciiValue ip_address(args[0]);
   int port = args[1]->Int32Value();

   handle = (uv_tcp_t*) malloc(sizeof(uv_tcp_t));
   handle->data = this;

   uv_tcp_init(uv_default_loop(), handle);
   uv_tcp_bind(handle, uv_ip4_addr(*ip_address, port));
   
   server = Persistent<Object>::New(localServer);
}

HttpWrap::~HttpWrap()
{
   free(handle);
}

Handle<Value> HttpWrap::Listen(const Arguments& args)
{
   HandleScope scope;
   
   Local<External> wrap = Local<External>::Cast(args.Holder()->GetInternalField(0));
   HttpWrap* httpWrap = static_cast<HttpWrap*>(wrap->Value());
   Local<Function> cb = Local<Function>::Cast(args[0]);
   httpWrap->callback = Persistent<Function>::New(cb);
   
   uv_listen((uv_stream_t*) httpWrap->handle, 128, OnConnection);

   return scope.Close(Undefined());
}

void HttpWrap::OnConnection(uv_stream_t* handle, int status)
{
   uv_tcp_t* client = (uv_tcp_t*) malloc(sizeof(uv_tcp_t));
   uv_tcp_init(uv_default_loop(), client);
   client->data = handle->data;
   uv_accept(handle, (uv_stream_t*) client);
   uv_read_start((uv_stream_t*) client, AllocConnection, OnRead);
}

uv_buf_t HttpWrap::AllocConnection(uv_handle_t* handle, size_t suggested_size)
{
   return uv_buf_init((char*) malloc(suggested_size), suggested_size);
}

void HttpWrap::OnRead(uv_stream_t* server, ssize_t nread, uv_buf_t buf)
{
   HandleScope scope;

   HttpWrap* httpWrap = static_cast<HttpWrap*>(server->data);
   Local<Value>* argv = new Local<Value>[1];
   argv[0] = String::New(buf.base);
   
   Local<Value> response = httpWrap->callback->Call(Context::GetCurrent()->Global(), 1, argv);
   
   String::AsciiValue data(response->ToString());
   
   uv_buf_t resbuf;
   resbuf.base = *data;
   resbuf.len = data.length();

   uv_write_t writeType;

   uv_write(&writeType, server, &resbuf, 1, 0);
   
   uv_close((uv_handle_t *) server, 0);

   free(buf.base);
}
