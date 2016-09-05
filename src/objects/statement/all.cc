// .all(Function callback) -> this

NAN_METHOD(Statement::All) {
	Statement* stmt = Nan::ObjectWrap::Unwrap<Statement>(info.This());
	if (!stmt->readonly) {
		return Nan::ThrowTypeError("This statement is not read-only. Use run() instead.");
	}
	REQUIRE_LAST_ARGUMENT_FUNCTION(func_index, func);
	WORKER_START(stmt, info, func_index, STATEMENT_BIND, statement);
	AllWorker* worker = new AllWorker(stmt, new Nan::Callback(func));
	WORKER_END(stmt, worker);
}
