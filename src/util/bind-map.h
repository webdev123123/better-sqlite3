#ifndef BETTER_SQLITE3_BIND_MAP_H
#define BETTER_SQLITE3_BIND_MAP_H

#include <string>

const int parameter_bits = 10;
const int parameter_mask = (1 << parameter_bits) - 1;
const int max_transaction_length = (1 << (32 - parameter_bits)) - 1;

typedef struct BindPair {
	std::string name;
	int index;
} BindPair;

class BindMap { public:
	explicit BindMap() : pairs(NULL), length(0) {}
	~BindMap() {delete[] pairs;}
	static inline int GetParameterIndex(int index) {
		return index & parameter_mask;
	}
	static inline int GetTransactionIndex(int index) {
		return index >> parameter_bits;
	}
	void Add(std::string name, int index) {
		pairs[length].name = name;
		pairs[length].index = index;
		length += 1;
	}
	void Add(std::string name, int parameter_index, int transaction_index) {
		pairs[length].name = name;
		pairs[length].index = parameter_index | (transaction_index << parameter_bits);
		length += 1;
	}
	void Grow(int* capacity) {
		int new_capacity = (*capacity << 1) | 2;
		BindPair* new_pairs = new BindPair[new_capacity];
		for (int i=0; i<length; ++i) {
			new_pairs[i] = pairs[i];
		}
		delete[] pairs;
		pairs = new_pairs;
		*capacity = new_capacity;
	}
	BindPair* pairs;
	int length;
};

#endif