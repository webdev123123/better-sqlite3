// .begin() -> Transaction

NAN_METHOD(Database::Begin) {
	v8::Local<v8::Function> cons = Nan::New<v8::Function>(Transaction::constructor);
	
	CONSTRUCTING_PRIVILEGES = true;
	v8::Local<v8::Object> transaction = cons->NewInstance(0, NULL);
	CONSTRUCTING_PRIVILEGES = false;
	
	transaction->SetHiddenValue(Nan::New("database").ToLocalChecked(), info.This());
	
	info.GetReturnValue().Set(transaction);
}
