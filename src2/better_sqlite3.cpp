#include <string>
#include <sqlite3.h>
#include <node.h>
#include <node_object_wrap.h>
#include <nan.h>
#include "objects/int64/int64.cpp"

void RegisterModule(v8::Local<v8::Object> exports, v8::Local<v8::Object> module) {
	v8::HandleScope scope(v8::Isolate::GetCurrent());
	
	Int64::Init(exports);
}
NODE_MODULE(better_sqlite3, RegisterModule);
