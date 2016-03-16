#ifndef NODE_SQLITE3_PLUS_DATA_H
#define NODE_SQLITE3_PLUS_DATA_H
#include <sqlite3.h>
#include <nan.h>
#include "strlcpy.h"

namespace Data {

// A generic class representing an SQLite3 value.
// When a Value is created, all memory is copied, and managed internally.
// Therefore, you must still manage the memory of the originally passed value.
class Value { public:
	Value() {}
	virtual ~Value() {}
	virtual v8::Local<v8::Value> ToJS() {return Nan::Undefined();}
};

// An SQLite3 integer value.
class Integer : public Data::Value { public:
	Integer(sqlite3_int64 n) : value(n) {}
	Integer(v8::Local<v8::Number> n) : value((sqlite3_int64)n->Value()) {}
	v8::Local<v8::Value> ToJS() {return Nan::New<v8::Number>((double)value);}
	sqlite3_int64 value;
};

// An SQLite3 real/float value.
class Float : public Data::Value { public:
	Float(double n) : value(n) {}
	Float(v8::Local<v8::Number> n) : value(n->Value()) {}
	v8::Local<v8::Value> ToJS() {return Nan::New<v8::Number>(value);}
	double value;
};

// An SQLite3 text value.
// The given constructor argument (const unsigned char*) should be a
// NUL-terminated string, and len should be the number of bytes in the string,
// not including the NUL terminator.
class Text : public Data::Value { public:
	Text(const unsigned char* str, int len) {
		length = len;
		value = new char[len + 1];
		strlcpy(value, (const char*)str, len + 1);
	}
	Text(v8::Local<v8::String> str) {
		Nan::Utf8String utf8(str);
		length = utf8.length();
		value = new char[length + 1];
		strlcpy(value, *utf8, length + 1);
	}
	~Text() {delete[] value;}
	v8::Local<v8::Value> ToJS() {return Nan::New<v8::String>(value, length).ToLocalChecked();}
	char* value; // Is NUL-terminated.
	int length; // Does not include the NUL terminator.
};

// An SQLite3 blob value.
// The given constructor argument (const void*) is a pointer to the raw bytes to
// include. The len argument is the number of bytes. Invoking ToJS() multiple
// times returns Buffers that all point to the same underlying memory.
class Blob : public Data::Value { public:
	Blob(const void* data, int len) {
		transferred = false;
		length = len;
		value = new char[len];
		memcpy(value, data, len);
	}
	Blob(v8::Local<v8::Object> buffer) {
		transferred = false;
		length = node::Buffer::Length(buffer);
		value = new char[length];
		memcpy(value, node::Buffer::Data(buffer), length);
	}
	~Blob() {if (!transferred) {delete[] value;}}
	v8::Local<v8::Value> ToJS() {
		transferred = true;
		return Nan::NewBuffer(value, length).ToLocalChecked();
	}
	char* value;
	int length;
	bool transferred;
};

// An SQLite3 null value.
class Null : public Data::Value { public:
	Null() {}
	v8::Local<v8::Value> ToJS() {return Nan::Null();}
};

// A list of SQLite3 values.
// Values that are added to a row are automatically destroyed when the row is
// destroyed. You should NOT manually destroy values that you add to a row.
// Before adding values, a row must be initialized with Init(n), where n is the
// maximum number of values that can be added to the row. It's okay to add less
// than the maximum, and column_count will always reflect the actual number of
// rows that have been added. However, you must promise never to add more than n
// number of rows, or else the behavior is undefined. It is an error to invoke
// Init() on a row more than once, and the behavior is undefined. The integer
// passed to Init() must never be less than 1.
class Row {
	public:
		Row() : column_count(0), init(false) {}
		~Row() {
			if (init) {
				for (int i=0; i<column_count; i++) {delete values[i];}
				delete[] values;
			}
		}
		
		// Given a row that has not yet been initialized, an sqlite3_stmt
		// handle, and a valid slice of columns for an avilable result row,
		// fills the row with the values given by the sqlite3_stmt.
		static inline void Fill(Row* row, sqlite3_stmt* handle, int i, int len) {
			row->Init(len - i);
			for (; i<len; i++) {
				int type = sqlite3_column_type(handle, i);
				switch (type) {
					case SQLITE_INTEGER:
						row->Add(new Data::Integer(sqlite3_column_int64(handle, i)));
						break;
					case SQLITE_FLOAT:
						row->Add(new Data::Float(sqlite3_column_double(handle, i)));
						break;
					case SQLITE_TEXT:
						row->Add(new Data::Text(sqlite3_column_text(handle, i), sqlite3_column_bytes(handle, i)));
						break;
					case SQLITE_BLOB:
						row->Add(new Data::Blob(sqlite3_column_blob(handle, i), sqlite3_column_bytes(handle, i)));
						break;
					default: // SQLITE_NULL
						row->Add(new Data::Null());
				}
			}
		}
		
		// max_columns must never be less than 1.
		inline void Init(int max_columns) {
			init = true;
			values = new Data::Value* [max_columns];
		}
		
		inline void Add(Data::Value* value) {
			values[column_count++] = value;
		}
		
		int column_count;
		Data::Value** values;
	private:
		bool init;
};

}
#endif