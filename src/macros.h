#ifndef NODE_SQLITE3_PLUS_MACROS_H
#define NODE_SQLITE3_PLUS_MACROS_H

#include <nan.h>
#include "sqlite3_plus.h"

#define REQUIRE_ARGUMENTS(n)                                                   \
    if (info.Length() < (n)) {                                                 \
        return Nan::ThrowTypeError("Expected " #n "arguments.");               \
    }

#define REQUIRE_ARGUMENT_FUNCTION(i, var)                                      \
    if (info.Length() <= (i) || !info[i]->IsFunction()) {                      \
        return Nan::ThrowTypeError("Argument " #i " must be a function.");     \
    }                                                                          \
    Local<v8::Function> var = v8::Local<v8::Function>::Cast(info[i]);

#define REQUIRE_ARGUMENT_STRING(i, var)                                        \
    if (info.Length() <= (i) || !info[i]->IsString()) {                        \
        return Nan::ThrowTypeError("Argument " #i " must be a string.");       \
    }                                                                          \
    Nan::Utf8String var(info[i]);

#define OPTIONAL_ARGUMENT_FUNCTION(i, var)                                     \
    v8::Local<v8::Function> var;                                               \
    if (info.Length() > i && !info[i]->IsUndefined()) {                        \
        if (!info[i]->IsFunction()) {                                          \
            return Nan::ThrowTypeError("Argument " #i " must be a function."); \
        }                                                                      \
        var = v8::Local<v8::Function>::Cast(info[i]);                          \
    }

#define EXCEPTION(msg, errno, name)                                            \
    v8::Local<v8::Value> name = v8::Exception::Error(                          \
        v8::String::Concat(                                                    \
            v8::String::Concat(                                                \
                Nan::New(SQLITE3_PLUS::sqlite_code_string(errno))              \
                    .ToLocalChecked(),                                         \
                Nan::New(": ").ToLocalChecked()                                \
            ),                                                                 \
            (msg)                                                              \
        )                                                                      \
    );                                                                         \
    v8::Local<v8::Object> name = name.As<v8::Object>();

#define EMIT_EVENT(obj, argc, argv)                                            \
    Nan::MakeCallback((obj),                                                   \
        Nan::Get(obj, Nan::New("emit").ToLocalChecked())                       \
            .ToLocalChecked().As<v8::Function>(),                              \
        argc, argv                                                             \
    );

#define CONSTRUCTOR(name)                                                      \
    Nan::Persistent<v8::Function> name;

#endif