#ifndef NODE_SQLITE3_PLUS_WORKER_STATEMENT_WORKER_H
#define NODE_SQLITE3_PLUS_WORKER_STATEMENT_WORKER_H

#include <sqlite3.h>
#include <nan.h>
#include "../../objects/database/database.h"
#include "../../objects/statement/statement.h"
#include "../../util/macros.h"
#include "../../util/handle-manager.h"

template <class T>
class StatementWorker : public T {
	public:
		StatementWorker(Statement* stmt, sqlite3_stmt* handle, int handle_index, Nan::Callback* cb, bool writer)
			: T(cb),
			handle(handle),
			db_handle(stmt->db_handle),
			stmt(stmt),
			handle_index(handle_index),
			writer(writer) {}
			
		void HandleErrorCallback() {
			Nan::HandleScope scope;
			CONCAT2(message, "SQLite: ", T::ErrorMessage());
			Reject(Nan::Error(message));
		}
		
	protected:
		void Resolve(v8::Local<v8::Value> value) {
			FinishRequest();
			v8::Local<v8::Value> args[2] = {Nan::Null(), value};
			T::callback->Call(2, args);
		}
		void Reject(v8::Local<v8::Value> value) {
			FinishRequest();
			v8::Local<v8::Value> args[1] = {value};
			T::callback->Call(1, args);
		}
		inline bool GetPluckColumn() {
			return stmt->pluck_column;
		}
		
		sqlite3_stmt* const handle;
		sqlite3* const db_handle;
		
	private:
		Statement* const stmt;
		int const handle_index;
		bool const writer;
		
		void FinishRequest() {
			stmt->requests -= 1;
			stmt->db->requests -= 1;
			stmt->handles->Release(handle_index, handle);
			if (stmt->requests == 0) {
				stmt->Unref();
				if (stmt->db->state == DB_DONE && stmt->db->requests == 0) {
					stmt->db->ActuallyClose();
				}
			}
			if (writer && --stmt->db->pending_write_statements == 0 && stmt->db->write_lock == 1) {
				stmt->db->write_lock = 2;
				Nan::AsyncQueueWorker(stmt->db->write_queue.Shift());
			}
		}
};

#endif