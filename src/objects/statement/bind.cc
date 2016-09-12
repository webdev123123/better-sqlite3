// .bind(...any boundValues) -> this

NAN_METHOD(Statement::Bind) {
	Statement* stmt = Nan::ObjectWrap::Unwrap<Statement>(info.This());
	if (stmt->state & CONFIG_LOCKED) {
		return Nan::ThrowTypeError("Cannot bind parameters after the statement has been executed.");
	}
	if (stmt->state & BOUND) {
		return Nan::ThrowTypeError("The bind() method can only be invoked once per statement object.");
	}
	if (stmt->db->state != DB_READY) {
		return Nan::ThrowError("The associated database connection is closed.");
	}
	
	STATEMENT_BIND(stmt, info, info.Length(), SQLITE_TRANSIENT);
	
	stmt->state |= BOUND;
	info.GetReturnValue().Set(info.This());
}
