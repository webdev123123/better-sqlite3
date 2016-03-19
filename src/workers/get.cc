#include <sqlite3.h>
#include <nan.h>
#include "get.h"
#include "statement-worker.h"
#include "../objects/statement/statement.h"
#include "../util/macros.h"
#include "../util/data.h"

GetWorker::GetWorker(Statement* stmt, sqlite3_stmt* handle, int handle_index, int pluck_column)
	: StatementWorker<Nan::AsyncWorker>(stmt, handle, handle_index),
	pluck_column(pluck_column) {}
void GetWorker::Execute() {
	LOCK_DB(db_handle);
	int status = sqlite3_step(handle);
	GET_COLUMN_RANGE(start, end);
	if (status == SQLITE_ROW) {
		Data::Row::Fill(&row, handle, start, end);
	} else if (status != SQLITE_DONE) {
		SetErrorMessage(sqlite3_errmsg(db_handle));
	}
	UNLOCK_DB(db_handle);
}
void GetWorker::HandleOKCallback() {
	Nan::HandleScope scope;
	
	// No error, but no result found.
	if (!row.column_count) {
		return Resolve(Nan::Undefined());
	}
	
	// Resolve with the plucked column.
	if (pluck_column >= 0) {
		return Resolve(row.values[0]->ToJS());
	}
	
	// Resolve with every column.
	v8::Local<v8::Object> obj = Nan::New<v8::Object>();
	for (int i=0; i<row.column_count; i++) {
		Nan::ForceSet(obj,
			Nan::New(sqlite3_column_name(handle, i)).ToLocalChecked(),
			row.values[i]->ToJS());
	}
	
	Resolve(obj);
}
