#include <stdint.h>
#include <sqlite3.h>
#include <nan.h>
#include "../objects/int64/int64.h"
#include "../util/macros.h"

#define JS_VALUE_TO_SQLITE(to, value, errorStatements, errorValue, ...)        \
	if (value->IsNumber()) {                                                   \
		return sqlite3_##to##_double(__VA_ARGS__,                              \
			v8::Local<v8::Number>::Cast(value)->Value()                        \
		);                                                                     \
	} else if (value->IsString()) {                                            \
		v8::String::Utf8Value utf8(v8::Local<v8::String>::Cast(value));        \
		return sqlite3_##to##_text(__VA_ARGS__,                                \
			*utf8,                                                             \
			utf8.length(),                                                     \
			SQLITE_TRANSIENT                                                   \
		);                                                                     \
	} else if (value->IsNull() || value->IsUndefined()) {                      \
		return sqlite3_##to##_null(__VA_ARGS__);                               \
	} else if (node::Buffer::HasInstance(value)) {                             \
		return sqlite3_##to##_blob(__VA_ARGS__,                                \
			node::Buffer::Data(value),                                         \
			node::Buffer::Length(value),                                       \
			SQLITE_TRANSIENT                                                   \
		);                                                                     \
	} else {                                                                   \
		v8::Local<v8::FunctionTemplate> Int64Template =                        \
			Nan::New<v8::FunctionTemplate>(Int64::constructorTemplate);        \
		if (Int64Template->HasInstance(value)) {                               \
			return sqlite3_##to##_int64(__VA_ARGS__,                           \
				Nan::ObjectWrap::Unwrap<Int64>(                                \
					v8::Local<v8::Object>::Cast(value)                         \
				)->GetValue()                                                  \
			);                                                                 \
		}                                                                      \
		errorStatements;                                                       \
		return errorValue;                                                     \
	}

#define SQLITE_VALUE_TO_JS(from, ...)                                          \
	int type = sqlite3_##from##_type(__VA_ARGS__);                             \
	switch (type) {                                                            \
	case SQLITE_INTEGER:                                                       \
		return Int64::NewProperInteger(sqlite3_##from##_int64(__VA_ARGS__));   \
	case SQLITE_FLOAT:                                                         \
		return Nan::New<v8::Number>(sqlite3_##from##_double(__VA_ARGS__));     \
	case SQLITE_TEXT:                                                          \
		return v8::String::NewFromUtf8(                                        \
			v8::Isolate::GetCurrent(),                                         \
			reinterpret_cast<const char*>(sqlite3_##from##_text(__VA_ARGS__)), \
			v8::NewStringType::kNormal,                                        \
			sqlite3_##from##_bytes(__VA_ARGS__)                                \
		).ToLocalChecked();                                                    \
	case SQLITE_BLOB:                                                          \
		return Nan::CopyBuffer(                                                \
			static_cast<const char*>(sqlite3_##from##_blob(__VA_ARGS__)),      \
			sqlite3_##from##_bytes(__VA_ARGS__)                                \
		).ToLocalChecked();                                                    \
	default: /* SQLITE_NULL */                                                 \
		return Nan::Null();                                                    \
	}

namespace Data {

v8::Local<v8::Value> GetValueJS(sqlite3_stmt* handle, int column) {
	SQLITE_VALUE_TO_JS(column, handle, column);
}

v8::Local<v8::Value> GetValueJS(sqlite3_value* value) {
	SQLITE_VALUE_TO_JS(value, value);
}

v8::Local<v8::Value> GetRowJS(sqlite3_stmt* handle, int column_count) {
	v8::Local<v8::Object> row = Nan::New<v8::Object>();
	for (int i=0; i<column_count; ++i) {
		Nan::Set(row, NEW_INTERNAL_STRING8(sqlite3_column_name(handle, i)), Data::GetValueJS(handle, i));
	}
	return row;
}

v8::Local<v8::Value>* GetArgumentsJS(sqlite3_value** values, int argument_count, bool safe_integers) {
	bool previous_safe_integers = SAFE_INTEGERS;
	SAFE_INTEGERS = safe_integers;
	v8::Local<v8::Value>* args = new v8::Local<v8::Value>[argument_count];
	for (int i=0; i<argument_count; ++i) {
		args[i] = Data::GetValueJS(values[i]);
	}
	SAFE_INTEGERS = previous_safe_integers;
	return args;
}

int BindValueFromJS(sqlite3_stmt* handle, int index, v8::Local<v8::Value> value) {
	JS_VALUE_TO_SQLITE(bind, value,, -1, handle, index);
}

void ResultValueFromJS(sqlite3_context* ctx, v8::Local<v8::Value> value, const char* function_name) {
	JS_VALUE_TO_SQLITE(result, value, {
		CONCAT3(message, "Custom SQL function \"", function_name, "\" returned an invalid value.");
		sqlite3_result_error(ctx, message.c_str(), -1);
	},, ctx);
}

}
