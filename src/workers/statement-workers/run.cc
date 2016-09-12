#include <sqlite3.h>
#include <nan.h>
#include "run.h"
#include "../query-worker.h"
#include "../../objects/statement/statement.h"
#include "../../util/macros.h"

RunWorker::RunWorker(Statement* stmt, Nan::Callback* cb)
	: QueryWorker<Statement, Nan::AsyncWorker>(stmt, cb) {}
void RunWorker::Execute() {
	sqlite3* db_handle = obj->db->db_handle;
	LOCK_DB(db_handle);
	
	int total_changes_before = sqlite3_total_changes(db_handle);
	
	sqlite3_step(obj->st_handle);
	if (sqlite3_reset(obj->st_handle) == SQLITE_OK) {
		changes = sqlite3_total_changes(db_handle) == total_changes_before ? 0 : sqlite3_changes(db_handle);
		id = sqlite3_last_insert_rowid(db_handle);
	} else {
		SetErrorMessage(sqlite3_errmsg(db_handle));
	}
	
	UNLOCK_DB(db_handle);
}
void RunWorker::HandleOKCallback() {
	Nan::HandleScope scope;
	
	v8::Local<v8::Object> object = Nan::New<v8::Object>();
	Nan::Set(object, NEW_INTERNAL_STRING_FAST("changes"), Nan::New<v8::Number>((double)changes));
	Nan::Set(object, NEW_INTERNAL_STRING_FAST("lastInsertROWID"), Nan::New<v8::Number>((double)id));
	
	Resolve(object);
}
