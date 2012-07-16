#include <stdio.h>
#include "v8/include/v8.h"
#include "libuv/include/uv.h"
#include <string>
#include "HttpWrap.h"

using namespace v8;

static Persistent<Context> context;

Handle<Value> Log(const Arguments& args)
{
   HandleScope scope;

   String::Utf8Value value(args[0]);
   fprintf(stdout, "%s", *value);  
   
   return Undefined();
}

Handle<Value> CreateServer(const Arguments& args) 
{
   HandleScope scope;
   HttpWrap* httpWrap = new HttpWrap(context, args);
 
   return scope.Close(httpWrap->server);
}

Handle<String> ReadFile(const std::string& fileName)
{
   FILE* file = fopen(fileName.c_str(), "rb");
   
   if (file == NULL) return Handle<String>();

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

Handle<ObjectTemplate> GetGlobalObject()
{
   Handle<ObjectTemplate> httpObject = ObjectTemplate::New();
   httpObject->Set(String::New("createServer"), FunctionTemplate::New(CreateServer));
   
   Handle<ObjectTemplate> global = ObjectTemplate::New();
   global->Set(String::New("http"), httpObject);
   global->Set(String::New("log"), FunctionTemplate::New(Log));

   return global;
}

void RunScript(const char* fileName)
{
   Handle<String> source = ReadFile(fileName);

   Handle<Script> compiledScript = Script::Compile(source);

   compiledScript->Run();
}

int main(int argc, char* argv[]) 
{		
   HandleScope scope;

   Handle<ObjectTemplate> global = GetGlobalObject();

   context = Context::New(NULL, global);

   Context::Scope context_scope(context);
   
   RunScript(argv[1]);

   uv_run(uv_default_loop());

   context.Dispose();

   return 0;
}
