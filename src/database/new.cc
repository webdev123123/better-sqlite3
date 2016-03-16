// new Database(string filename)

NAN_METHOD(Database::New) {
	REQUIRE_ARGUMENT_STRING(0, filename);
	if (!info.IsConstructCall()) {
		v8::Local<v8::Function> cons = Nan::New<v8::Function>(constructor);
		info.GetReturnValue().Set(cons->NewInstance(1, {filename}));
	} else {
		Database* db = new Database();
		db->Wrap(info.This());
		
		db->Ref();
		db->workers += 1;
		Nan::AsyncQueueWorker(new OpenWorker(db, C_STRING(filename)));
		
		info.GetReturnValue().Set(info.This());
	}
}
