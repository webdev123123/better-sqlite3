#ifndef NODE_SQLITE3_PLUS_MACROS_H
#define NODE_SQLITE3_PLUS_MACROS_H

#include <cstring>
#include <cstdlib>
#include <nan.h>

inline char* RAW_STRING(v8::Handle<v8::String> val) {
    Nan::Utf8String utf8(val);
    
    int len = utf8.length() + 1;
    char* str = (char*) calloc(sizeof(char), len);
    strncpy(str, *utf8, len);
    
    return str;
}

#define REQUIRE_ARGUMENTS(n)                                                   \
    if (info.Length() < (n)) {                                                 \
        return Nan::ThrowTypeError("Expected " #n " arguments.");              \
    }

#define REQUIRE_ARGUMENT_FUNCTION(i, var)                                      \
    if (info.Length() <= (i) || !info[i]->IsFunction()) {                      \
        return Nan::ThrowTypeError("Argument " #i " must be a function.");     \
    }                                                                          \
    v8::Local<v8::Function> var = v8::Local<v8::Function>::Cast(info[i]);

#define REQUIRE_ARGUMENT_STRING(i, var)                                        \
    if (info.Length() <= (i) || !info[i]->IsString()) {                        \
        return Nan::ThrowTypeError("Argument " #i " must be a string.");       \
    }                                                                          \
    v8::Local<v8::String> var = v8::Local<v8::String>::Cast(info[i]);

#define OPTIONAL_ARGUMENT_FUNCTION(i, var)                                     \
    v8::Local<v8::Function> var;                                               \
    if (info.Length() > i && !info[i]->IsUndefined()) {                        \
        if (!info[i]->IsFunction()) {                                          \
            return Nan::ThrowTypeError("Argument " #i " must be a function."); \
        }                                                                      \
        var = v8::Local<v8::Function>::Cast(info[i]);                          \
    }

#define OPTIONAL_ARGUMENT_STRING(i, var)                                       \
    v8::Local<v8::String> var;                                                 \
    if (info.Length() > i && !info[i]->IsUndefined()) {                        \
        if (!info[i]->IsString()) {                                            \
            return Nan::ThrowTypeError("Argument " #i " must be a string.");   \
        }                                                                      \
        var = v8::Local<v8::String>::Cast(info[i]);                            \
    }

#define CONCAT2(var, a, b)                                                     \
    v8::Local<v8::String> var = v8::String::Concat(                            \
        Nan::New(a).ToLocalChecked(),                                          \
        Nan::New(b).ToLocalChecked()                                           \
    );

#define CONCAT3(var, a, b, c)                                                  \
    v8::Local<v8::String> var = v8::String::Concat(                            \
        v8::String::Concat(                                                    \
            Nan::New(a).ToLocalChecked(),                                      \
            Nan::New(b).ToLocalChecked()                                       \
        ),                                                                     \
        Nan::New(c).ToLocalChecked()                                           \
    );

#define CONCAT4(var, a, b, c, d)                                               \
    v8::Local<v8::String> var = v8::String::Concat(                            \
        v8::String::Concat(                                                    \
            Nan::New(a).ToLocalChecked(),                                      \
            Nan::New(b).ToLocalChecked()                                       \
        ),                                                                     \
        v8::String::Concat(                                                    \
            Nan::New(c).ToLocalChecked(),                                      \
            Nan::New(d).ToLocalChecked()                                       \
        )                                                                      \
    );

#define INVOKE_METHOD(var, obj, methodName, argc, argv)                        \
    Nan::MaybeLocal<v8::Value> _maybeMethod =                                  \
        Nan::Get(obj, Nan::New(methodName).ToLocalChecked());                  \
    if (_maybeMethod.IsEmpty()) {return;}                                      \
    v8::Local<v8::Value> _method = _maybeMethod.ToLocalChecked();              \
    if (!_method->IsFunction()) {                                              \
        return Nan::ThrowTypeError(                                            \
            "" #obj "[" #methodName "]() is not a function");                  \
    }                                                                          \
    Nan::MaybeLocal<v8::Value> _maybeValue =                                   \
        Nan::Call(v8::Local<v8::Function>::Cast(_method), obj, argc, argv);    \
    if (_maybeValue.IsEmpty()) {return;}                                       \
    v8::Local<v8::Value> var = _maybeValue.ToLocalChecked();

#define EMIT_EVENT(obj, argc, argv)                                            \
    Nan::MakeCallback((obj),                                                   \
        Nan::Get(obj, Nan::New("emit").ToLocalChecked())                       \
            .ToLocalChecked().As<v8::Function>(),                              \
        argc, argv                                                             \
    );

#define CONSTRUCTOR(name)                                                      \
    Nan::Persistent<v8::Function> name;

#endif