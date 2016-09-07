#ifndef BETTER_SQLITE3_MULTIBINDER_H
#define BETTER_SQLITE3_MULTIBINDER_H

#include <sqlite3.h>
#include <nan.h>
#include "../binder/binder.h"

class MultiBinder : public Binder {
	public:
		MultiBinder(sqlite3_stmt**, unsigned int, v8::Local<v8::Object>);
		void Bind(Nan::NAN_METHOD_ARGS_TYPE, int, v8::Local<v8::Object>);
		
	protected:
		int NextAnonIndex();
		int BindObject(v8::Local<v8::Object>, v8::Local<v8::Object>); // This should only be invoked once per handle
		
		sqlite3_stmt** const handles;
		unsigned int const handle_count;
		unsigned int handle_index;
		int param_count_sum;
};

#endif