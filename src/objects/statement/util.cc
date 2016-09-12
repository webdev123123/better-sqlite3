// Used by std::set to organize the pointers it holds.
bool Statement::Compare::operator() (const Statement* a, const Statement* b) {
	return a->id < b->id;
}

// Builds a JavaScript object that maps the statement's parameter names with
// the parameter index of each one. After the first invocation, a cached version
// is returned, rather than rebuilding it.
v8::Local<v8::Object> Statement::GetBindMap() {
	if (state & HAS_BIND_MAP) {
		return v8::Local<v8::Object>::Cast(handle()->GetHiddenValue(Nan::EmptyString()));
	}
	int param_count = sqlite3_bind_parameter_count(st_handle);
	v8::Local<v8::Function> cons = v8::Local<v8::Function>::Cast(db->handle()->GetHiddenValue(NEW_INTERNAL_STRING_FAST("NF")));
	v8::Local<v8::Object> namedParams = Nan::NewInstance(cons).ToLocalChecked();
	for (int i=1; i<=param_count; ++i) {
		const char* name = sqlite3_bind_parameter_name(st_handle, i);
		if (name != NULL) {
			Nan::Set(namedParams, NEW_INTERNAL_STRING8(name + 1), Nan::New<v8::Number>((double)i));
		}
	}
	handle()->SetHiddenValue(Nan::EmptyString(), namedParams);
	state |= HAS_BIND_MAP;
	return namedParams;
}

// get .readonly -> boolean
NAN_GETTER(Statement::Readonly) {
	info.GetReturnValue().Set(Nan::ObjectWrap::Unwrap<Statement>(info.This())->column_count != 0);
}
