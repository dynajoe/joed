#include <stdio.h>
#include "v8/include/v8.h"
#include "libuv/include/uv.h"
#include <string>

using namespace v8;

static Persistent<Context> context;

Handle<Value> WriteToResponse(const Arguments& args) 
{
   HandleScope scope;

   String::AsciiValue response(args[0]);

   puts(*response);

   return Undefined();
}


uv_buf_t AllocConnection(uv_handle_t* handle, size_t suggested_size)
{
   return uv_buf_init((char*) malloc(suggested_size), suggested_size);
}

void OnClose(uv_handle_t* handle)
{
   puts("Closed");
}

void OnRead(uv_stream_t* server, ssize_t nread, uv_buf_t buf)
{
   HandleScope scope;
   Context::Scope context_scope(context);
   
   Persistent<Function> callback = *((Persistent<Function>*) (server->data));
   
   Handle<ObjectTemplate> response = ObjectTemplate::New();
  
   response->Set(String::New("respond"), FunctionTemplate::New(WriteToResponse));
 
   Local<Object> responseObj = response->NewInstance();
   
   callback->Call(context->Global(), 1, (Local<Value>*) &responseObj);
   
   uv_close((uv_handle_t*) server, OnClose);

   free(buf.base);
}

void OnConnection(uv_stream_t* server, int status)
{
   uv_tcp_t* stream = (uv_tcp_t*) malloc(sizeof(uv_tcp_t));

   int r = uv_tcp_init(uv_default_loop(), stream);

   if (r)
   {

   }

   stream->data = server->data;

   r = uv_accept(server, (uv_stream_t*) stream);

   r = uv_read_start((uv_stream_t*) stream, AllocConnection, OnRead);
}

Handle<Value> InitializeHttpResponder(const Arguments& args) 
{
   if (args.Length() < 1) 
   {
      return v8::Undefined();
   }   

   uv_tcp_t* server = (uv_tcp_t*) malloc(sizeof(uv_tcp_t));

   uv_tcp_init(uv_default_loop(), server);

   Local<Function> callback = Local<Function>::Cast(args[0]);

   Persistent<Function> pCallback = Persistent<Function>::New(callback);

   server->data = &pCallback;

   int r = uv_tcp_bind(server, uv_ip4_addr("0.0.0.0", 5000));

   r = uv_listen((uv_stream_t*) server, 128, OnConnection);

   return Undefined();
}

Handle<String> ReadFile(const std::string& fileName)
{
   FILE* file = fopen(fileName.c_str(), "rb");
   
   if(file == NULL) return Handle<String>();

   //Get file length
   fseek(file, 0, SEEK_END);
   int size = ftell(file);
   rewind(file);

   char* contents = new char[size + 1];
   
   for (int i = 0; i < size;) 
   {
      int bytesRead = static_cast<int>(fread(&contents[i], 1, size - i, file));
      i += bytesRead;
   }
   
   fclose(file);

   Handle<String> result = String::New(contents, size);
   
   delete[] contents;

   return result;
}

void SetupGlobal(int argc, char* argv[]) 
{
   HandleScope handle_scope;

   Handle<ObjectTemplate> global = ObjectTemplate::New();
   Handle<ObjectTemplate> httpObject = ObjectTemplate::New();
   
   httpObject->Set(String::New("respond"), FunctionTemplate::New(InitializeHttpResponder));
   global->Set(String::New("http"), httpObject);

   context = Context::New(NULL, global);

   Context::Scope context_scope(context);
   
   Handle<String> source = ReadFile(argv[1]);
   Handle<Script> script = Script::Compile(source);
   script->Run();
   
   context.Dispose();
}

int main(int argc, char* argv[]) 
{		
	if (argc != 2) 
   {
      puts("Specify a file to load");
      return -1;
   }

   SetupGlobal(argc, argv);

   uv_run(uv_default_loop());
   
   puts("shutting down");

   return 0;
}
