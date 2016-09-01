#include <sqlite3.h>
#include <nan.h>
#include "multi-binder.h"
#include "../binder/binder.h"

#include "next-anon-index.cc"
#include "bind-object.cc"
#include "bind.cc"

MultiBinder::MultiBinder(sqlite3_stmt** handles, unsigned int handle_count)
	: Binder(handles[0])
	, handles(handles)
	, handle_count(handle_count)
	, handle_index(0)
	, param_count_sum(param_count) {}
