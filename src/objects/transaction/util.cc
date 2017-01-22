// Used by std::set to organize the pointers it holds.
bool Transaction::Compare::operator() (const Transaction* a, const Transaction* b) const {
	return a->id < b->id;
}

// Builds a JavaScript array that has an object for each sqlite3_stmt handle
// that has bind parameters. Each object maps the handle's parameter names
// to their respective parameter index. After the first invocation, a cached
// version is returned, rather than rebuilding it.
v8::Local<v8::Object> Transaction::GetBindMap() {
	if (state & HAS_BIND_MAP) {
		return v8::Local<v8::Object>::Cast(Nan::GetPrivate(handle(), Nan::EmptyString()).ToLocalChecked());
	}
	v8::Local<v8::Function> cons = Nan::New<v8::Function>(NullFactory);
	v8::Local<v8::Object> array = Nan::New<v8::Object>();
	for (unsigned int h=0; h<handle_count; ++h) {
		sqlite3_stmt* handle = handles[h];
		int param_count = sqlite3_bind_parameter_count(handle);
		if (param_count > 0) {
			v8::Local<v8::Object> namedParams = Nan::NewInstance(cons).ToLocalChecked();
			for (int i=1; i<=param_count; ++i) {
				const char* name = sqlite3_bind_parameter_name(handle, i);
				if (name != NULL) {
					Nan::Set(namedParams, NEW_INTERNAL_STRING8(name + 1), Nan::New<v8::Number>(static_cast<double>(i)));
				}
			}
			Nan::Set(array, h, namedParams);
		}
	}
	if (state & USED_BIND_MAP) {
		Nan::SetPrivate(handle(), Nan::EmptyString(), array);
		state |= HAS_BIND_MAP;
	} else {
		state |= USED_BIND_MAP;
	}
	return array;
}

// .safeIntegers(boolean) -> this
NAN_METHOD(Transaction::SafeIntegers) {
	Transaction* trans = Nan::ObjectWrap::Unwrap<Transaction>(info.This());
	
	if (info.Length() == 0 || info[0]->BooleanValue() == true) {
		trans->state |= SAFE_INTS;
	} else {
		trans->state &= ~SAFE_INTS;
	}
	
	info.GetReturnValue().Set(info.This());
}
