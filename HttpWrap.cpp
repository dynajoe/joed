#include "HttpWrap.h"
#include <cstdlib>
#include "v8/include/v8.h"
#include "libuv/include/uv.h"
#include <string.h>

bool isAtCR = false;

const char CR = '\r';
const char LF = '\n';

using namespace v8;

typedef struct 
{
   HttpWrap* wrapper;
   uv_tcp_t handle;
   char* data;
   int max;
   int length;
   bool isAtCR;
} Client;

HttpWrap::HttpWrap(Handle<Context> context, const Arguments& args)
{
   Local<ObjectTemplate> serverTemplate = ObjectTemplate::New();
   serverTemplate->SetInternalFieldCount(1);
   serverTemplate->Set(String::New("listen"), FunctionTemplate::New(Listen));

   Local<Object> localServer = serverTemplate->NewInstance();
   localServer->SetInternalField(0, External::New(this));

   String::Utf8Value ip_address(args[0]);
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
   Client* client = (Client*) malloc(sizeof(Client));
   uv_tcp_init(uv_default_loop(), &client->handle);

   client->max = 0;
   client->length = 0;
   client->wrapper = (HttpWrap*) handle->data;
   client->handle.data = client;

   uv_accept(handle, (uv_stream_t*) &client->handle);
   uv_read_start((uv_stream_t*) &client->handle, AllocConnection, OnRead);
}

uv_buf_t HttpWrap::AllocConnection(uv_handle_t* handle, size_t suggested_size)
{
   return uv_buf_init((char*) malloc(suggested_size), suggested_size);
}

uv_buf_t GetResponse(Client* client)
{
   HandleScope scope;

   Handle<Value> argv[1] = { String::New(client->data, client->length) };

   Local<Value> response = client->wrapper->callback->Call(
      Context::GetCurrent()->Global(), 1, argv);

   String::Utf8Value data(response->ToString());

   uv_buf_t resbuf = uv_buf_init((char*) malloc(data.length()), data.length());
   
   memcpy(resbuf.base, *data, data.length());

   return resbuf;
}

void ProcessData(Client* client, uv_stream_t* server, ssize_t nread, uv_buf_t buf)
{
   int offset = 0;

   if (client->max == 0)
   {
      client->data = (char*) malloc(nread);   
      client->max = nread;
   }
   else if (client->length + nread > client->max)
   {
      int newMax = client->max * 2 + nread;
      char* newBuffer = (char*) malloc(newMax);
      memcpy(newBuffer, buf.base, nread);
      
      offset = client->length;
      client->data = newBuffer;
      client->max = newMax;
   }
   else
   {
      offset = client->length;
   }
   
   bool headersComplete = false;

   for (int i = 0; i < nread; i++)
   {
      char current = buf.base[i];

      client->data[client->length++] = current;

      if (current == CR && client->isAtCR)
      {
         headersComplete = true;
         break;
      }

      if (current == CR) 
      {
         client->isAtCR = true;
      }
      else if (buf.base[i] != LF) 
      { 
         client->isAtCR = false; 
      }
   }

   if (headersComplete)
   {
      uv_buf_t resbuf = GetResponse(client);

      uv_write_t writeType;

      uv_write(&writeType, (uv_stream_t*) &client->handle, &resbuf, 1, 0); 

      //uv_close((uv_handle_t*) server, HttpWrap::OnClose);
   }
}

void HttpWrap::OnRead(uv_stream_t* server, ssize_t nread, uv_buf_t buf)
{
   puts("onread");

   if (nread < 0)
   {
      if (buf.base) 
      {
         free(buf.base);
      }

      if (uv_last_error(uv_default_loop()).code == UV_EOF)
      { 
         uv_close((uv_handle_t*) server, OnClose);
      }

      return;
   }

   if (nread > 0)
   {
      ProcessData((Client*) server->data, server, nread, buf);
      puts("Processed data");
   }

   puts("freeing at end");
   free(buf.base);
}

void HttpWrap::OnClose(uv_handle_t* handle)
{
   puts ("onclose");
   //Client* client = (Client*) handle->data;
   puts("after client");
 //  free(client->data);
}