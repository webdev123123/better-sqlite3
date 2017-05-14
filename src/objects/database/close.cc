// .close() -> this

NAN_METHOD(Database::Close) {
	Database* db = Nan::ObjectWrap::Unwrap<Database>(info.This());
	
	if (db->open) {
		if (db->busy) {
			return Nan::ThrowTypeError("You cannot close a database while it is executing a query");
		}
		
		db->open = false;
		db->CloseChildHandles();
		if (db->CloseHandles() != SQLITE_OK) {
			return Nan::ThrowError("Failed to successfully close the database connection");
		}
	}
	
	info.GetReturnValue().Set(info.This());
}
