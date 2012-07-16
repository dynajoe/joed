#include <stdio.h>
#include "v8/include/v8.h"
#include "libuv/include/uv.h"
#include <string>
#include "HttpWrap.h"

using namespace v8;

static Persistent<Context> context;

Handle<Value> CreateServer(const Arguments& args) 
{
   HandleScope scope;

   HttpWrap* httpWrap = new HttpWrap(args);

   return scope.Close(httpWrap->server);
}

Handle<String> ReadFile(const std::string& fileName)
{
   FILE* file = fopen(fileName.c_str(), "rb");
   
   if (file == NULL) return Handle<String>();

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
   
   httpObject->Set(String::New("createServer"), FunctionTemplate::New(CreateServer));
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
