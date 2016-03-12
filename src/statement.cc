#include <cstdlib>
#include <cmath>
#include <sqlite3.h>
#include <nan.h>
#include "macros.h"
#include "database.h"
#include "statement.h"

class RunWorker : public Nan::AsyncWorker {
	public:
		RunWorker(Statement*, sqlite3_stmt*, int);
		~RunWorker();
		void Execute();
		void HandleOKCallback();
		void HandleErrorCallback();
		void FinishRequest();
	private:
		Statement* stmt;
		sqlite3_stmt* handle;
		int handle_index;
		int changes;
		sqlite3_int64 id;
};





Statement::Statement() : Nan::ObjectWrap(),
	db(NULL),
	source_string(NULL),
	closed(false),
	handles(NULL),
	handle_states(NULL),
	handle_count(0),
	next_handle(0),
	cache_locked(false),
	requests(0) {}
Statement::~Statement() {
	if (!closed) {
		if (db) {db->stmts.Remove(this);}
		CloseStatement(this);
	}
	free(source_string);
}
void Statement::Init() {
	Nan::HandleScope scope;
	
	v8::Local<v8::FunctionTemplate> t = Nan::New<v8::FunctionTemplate>(New);
	t->InstanceTemplate()->SetInternalFieldCount(1);
	t->SetClassName(Nan::New("Statement").ToLocalChecked());
	
	Nan::SetPrototypeMethod(t, "cache", Cache);
	Nan::SetPrototypeMethod(t, "run", Run);
	Nan::SetAccessor(t->InstanceTemplate(), Nan::New("readonly").ToLocalChecked(), ReadonlyGetter);
	
	constructor.Reset(Nan::GetFunction(t).ToLocalChecked());
}
CONSTRUCTOR(Statement::constructor);
NAN_METHOD(Statement::New) {
	if (!CONSTRUCTING_PRIVILEGES) {
		return Nan::ThrowTypeError("Statements can only be constructed by the db.prepare() method.");
	}
	Statement* statement = new Statement();
	statement->Wrap(info.This());
	info.GetReturnValue().Set(info.This());
}
NAN_GETTER(Statement::ReadonlyGetter) {
	Statement* stmt = Nan::ObjectWrap::Unwrap<Statement>(info.This());
	info.GetReturnValue().Set(stmt->readonly == true);
}
NAN_METHOD(Statement::Cache) {
	REQUIRE_ARGUMENT_NUMBER(0, number);
	Statement* stmt = Nan::ObjectWrap::Unwrap<Statement>(info.This());
	if (stmt->db->state != DB_READY) {
		return Nan::ThrowError("The associated database connection is closed.");
	}
	if (stmt->cache_locked) {
		return Nan::ThrowError("A Statement's cache cannot be altered after it has been executed.");
	}
	
	double numberValue = number->Value();
	if (!std::isfinite(numberValue) || numberValue < 1) {
		return Nan::ThrowError("Argument 0 must be a positive, finite number.");
	}
	
	int len = (int)numberValue;
	sqlite3_stmt** handles = new sqlite3_stmt* [len];
	
	for (int i=0; i<len; i++) {
		handles[i] = stmt->NewHandle();
		if (handles[i] == NULL) {
			CONCAT2(message, "SQLite: ", sqlite3_errmsg(stmt->db_handle));
			for (; i>=0; i--) {
				sqlite3_finalize(handles[i]);
			}
			delete[] handles;
			return Nan::ThrowError(message);
		}
	}
	
	stmt->FreeHandles();
	stmt->handles = handles;
	stmt->handle_states = new bool [len]();
	stmt->handle_count = len;
	
	info.GetReturnValue().Set(info.This());
}
NAN_METHOD(Statement::Run) {
	Statement* stmt = Nan::ObjectWrap::Unwrap<Statement>(info.This());
	if (stmt->readonly) {
		return Nan::ThrowTypeError("This Statement is read-only. Use get(), all(), or each() instead.");
	}
	STATEMENT_START(stmt, RunWorker);
}
void Statement::CloseStatement(Statement* stmt) {
	stmt->closed = true;
	stmt->FreeHandles();
}
void Statement::FreeHandles() {
	int len = handle_count;
	for (int i=0; i<len; i++) {
		sqlite3_finalize(handles[i]);
	}
	delete[] handles;
	delete[] handle_states;
}
sqlite3_stmt* Statement::NewHandle() {
	sqlite3_stmt* handle;
	sqlite3_prepare_v2(db_handle, source_string, source_length, &handle, NULL);
	return handle;
}





RunWorker::RunWorker(Statement* stmt, sqlite3_stmt* handle, int handle_index)
	: Nan::AsyncWorker(NULL), stmt(stmt), handle(handle), handle_index(handle_index) {}
RunWorker::~RunWorker() {}
void RunWorker::Execute() {
	int status = sqlite3_step(handle);
	if (status == SQLITE_DONE || status == SQLITE_ROW) {
		changes = sqlite3_changes(stmt->db_handle);
		id = sqlite3_last_insert_rowid(stmt->db_handle);
	} else {
		SetErrorMessage(sqlite3_errmsg(stmt->db_handle));
	}
}
void RunWorker::FinishRequest() {
	stmt->requests -= 1;
	stmt->db->requests -= 1;
	if (handle_index == -1) {
		sqlite3_finalize(handle);
	} else {
		stmt->handle_states[handle_index] = false;
		sqlite3_reset(handle);
	}
	if (stmt->requests == 0) {
		stmt->Unref();
		if (stmt->db->state == DB_DONE && stmt->db->requests == 0) {
			stmt->db->ActuallyClose();
		}
	}
}
void RunWorker::HandleOKCallback() {
	Nan::HandleScope scope;
	FinishRequest();
	
	v8::Local<v8::Object> obj = Nan::New<v8::Object>();
	Nan::ForceSet(obj, Nan::New("changes").ToLocalChecked(), Nan::New<v8::Number>((double)changes));
	Nan::ForceSet(obj, Nan::New("id").ToLocalChecked(), Nan::New<v8::Number>((double)id));
	
	v8::Local<v8::Promise::Resolver> resolver = v8::Local<v8::Promise::Resolver>::Cast(GetFromPersistent((uint32_t)0));
	resolver->Resolve(Nan::GetCurrentContext(), obj);
}
void RunWorker::HandleErrorCallback() {
	Nan::HandleScope scope;
	FinishRequest();
	
	CONCAT2(message, "SQLite: ", ErrorMessage());
	
	v8::Local<v8::Promise::Resolver> resolver = v8::Local<v8::Promise::Resolver>::Cast(GetFromPersistent((uint32_t)0));
	resolver->Reject(Nan::GetCurrentContext(), v8::Exception::Error(message));
}
