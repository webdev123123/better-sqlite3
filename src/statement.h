#ifndef NODE_SQLITE3_PLUS_STATEMENT_H
#define NODE_SQLITE3_PLUS_STATEMENT_H

#include <sqlite3.h>
#include <nan.h>
#include "macros.h"
class Database;

class Statement : public Nan::ObjectWrap {
	public:
		Statement();
		~Statement();
		static void Init();
		class CloseStatementFunction { public:
		    void operator()(Statement* stmt, int i = 0) const {
		        stmt->closed = true;
		    	stmt->FreeHandles();
		    }
		};
		
		friend class Database;
		template <class T> friend class StatementWorker;
		
	private:
		static CONSTRUCTOR(constructor);
		static NAN_METHOD(New);
		static NAN_GETTER(ReadonlyGetter);
		static NAN_METHOD(Cache);
		static NAN_METHOD(Pluck);
		static NAN_METHOD(Run);
		static NAN_METHOD(Get);
		static NAN_METHOD(All);
		static NAN_METHOD(Each);
		void FreeHandles();
		sqlite3_stmt* NewHandle(); // This should only be invoked while db.state == DB_READY
		
		Database* db;
		sqlite3* db_handle;
		char* source_string; // NUL-terminated
		int source_length; // DOES include the NUL terminator
		bool readonly;
		int pluck_column;
		bool closed; // Whether the statement's handles have been freed before garbage collection, by Database::Close()
		
		sqlite3_stmt** handles;
		bool* handle_states;
		int handle_count;
		int next_handle;
		bool config_locked;
		unsigned int requests;
};

#endif