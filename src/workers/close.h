#ifndef BETTER_SQLITE3_WORKER_CLOSE_H
#define BETTER_SQLITE3_WORKER_CLOSE_H

#include <nan.h>
class Database;

class CloseWorker : public Nan::AsyncWorker {
	public:
		explicit CloseWorker(Database*);
		void Execute();
		void HandleOKCallback();
		void HandleErrorCallback();
	private:
		Database* const db;
};

#endif