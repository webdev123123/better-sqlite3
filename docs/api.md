# API

## class *Database*

- [new Database()](#new-databasepath-options)
- [Database#prepare()](#preparestring---statement) (see [`Statement`](#class-statement))
- [Database#transaction()](#transactionmode-function---function)
- [Database#pragma()](#pragmastring-simplify---results)
- [Database#checkpoint()](#checkpointdatabasename---this)
- [Database#register()](#registeroptions-function---this)
- [Database#loadExtension()](#loadextensionpath---this)
- [Database#exec()](#execstring---this)
- [Database#close()](#close---this)
- [Properties](#properties)

### new Database(*path*, [*options*])

Creates a new database connection. If the database file does not exist, it is created. This happens synchronously, which means you can start executing queries right away.

- If `options.memory` is `true`, an in-memory database will be created, rather than a disk-bound one.

- If `options.readonly` is `true`, the database connection will be opened in readonly mode.

- If `options.fileMustExist` is `true` and the database does not exist, an `Error` will be thrown instead of creating a new file. This option does not affect in-memory or readonly database connections.

> All options listed above default to `false`.

```js
const Database = require('better-sqlite3');
const db = new Database('foobar.db', { readonly: true });
```

### .prepare(*string*) -> *Statement*

Creates a new prepared [`Statement`](#class-statement) from the given SQL string.

```js
const stmt = db.prepare('SELECT name, age FROM cats');
const cats = stmt.all();
```

### .transaction(*function*) -> *function*

Creates a function that always runs inside a [transaction](https://sqlite.org/lang_transaction.html). When the function is invoked, it will begin a new transaction. When the function returns, the transaction will be committed. If an exception is thrown, the transaction will be rolled back (the exception will still propagate—it does not get suppressed).

```js
const insert = db.prepare('INSERT INTO cats (name, age) VALUES (@name, @age)');

const insertMany = db.transaction((cats) => {
  for (const cat of cats) insert.run(cat);
});

insertMany([
  { name: 'Joey', age: 2 },
  { name: 'Sally', age: 4 },
  { name: 'Junior', age: 1 },
]);
```

Nested transactions ([savepoints](https://www.sqlite.org/lang_savepoint.html)) are supported automatically. Transaction functions can be called from inside other transaction functions.

```js
const newExpense = db.prepare('INSERT INTO expenses (note, dollars) VALUES (?, ?)');

const adopt = db.transaction((cats) => {
  newExpense('adoption fees', 20);
  insertMany(cats); // nested transaction
});
```

Transaction functions also come with `deferred`, `immediate`, and `exclusive` versions.

```js
insertMany(cats); // uses "BEGIN"
insertMany.deferred(cats); // uses "BEGIN DEFERRED"
insertMany.immediate(cats); // uses "BEGIN IMMEDIATE"
insertMany.exclusive(cats); // uses "BEGIN EXCLUSIVE"
```

#### Caveats

If you'd like to manage transactions manually, you're free to do so with regular [prepared statements](#preparestring---statement) (using `BEGIN`, `COMMIT`, etc.). However, the usage of manually managed transactions should not be mixed with the automatic transaction handling offered by the `.transaction()` method. In other words, using raw `COMMIT` or `ROLLBACK` statements inside a transaction function is not supported.

It's important to know that SQLite3 may sometimes rollback a transaction without us asking it to. This can happen either because of an [`ON CONFLICT`](https://sqlite.org/lang_conflict.html) clause, the [`RAISE()`](https://www.sqlite.org/lang_createtrigger.html) trigger function, or certain errors such as `SQLITE_FULL` or `SQLITE_BUSY`. When this occurs, transaction functions will automatically detect the situation and handle it appropriately. However, if you catch one of these errors with a try-catch statement, you become responsible for handling the case. In other words, all catch statements within transaction functions should look like this:

```js
try {
  ...
} catch (err) {
  if (!db.inTransaction) throw err; // transaction was forcefully rolled back
  ...
}
```

This situation generally only arises when checking for partial failures inside a nesting transaction.

### .pragma(*string*, [*options*]) -> *results*

TOOD: options object
TODO: new docs

Executes the given PRAGMA and returns its result. By default, the return value will be an array of result rows. Each row is represented by an object whose keys correspond to column names.

Since most PRAGMA statements return a single value, the `simple` option is provided to make things easier. When `simple` is `true`, only the first column of the first row will be returned.

```js
db.pragma('cache_size = 32000');
const cacheSize = db.pragma('cache_size', { simple: true }); // returns the number 32000
```

If execution of the PRAGMA fails, an `Error` is thrown.

It's better to use this method instead of normal [prepared statements](#preparestring---statement) when executing PRAGMA, because this method normalizes some odd behavior that may otherwise be experienced. The documentation on SQLite3 PRAGMA can be found [here](https://www.sqlite.org/pragma.html).

### .checkpoint([*databaseName*]) -> *this*

Runs a [WAL mode checkpoint](https://www.sqlite.org/wal.html) on all attached databases (including the `main` database).

Unlike [automatic checkpoints](https://www.sqlite.org/wal.html#automatic_checkpoint), this method executes a checkpoint in "RESTART" mode, which ensures a complete checkpoint operation even if other processes are using the database at the same time. You only need to use this method if you are accessing the database from multiple processes at the same time.

If `databaseName` is provided, it should be the name of an attached database (or `"main"`). This causes only that database to be checkpointed.

If the checkpoint fails, an `Error` is thrown.

```js
setInterval(() => db.checkpoint(), 30000).unref();
```

### .register([*options*], *function*) -> *this*

TODO: support window functions
TODO: options at the end?
TODO: new docs

TODO: implement custom collating sequences
TODO: implement online backup API

Registers the given `function` so that it can be used by SQL statements.

```js
db.register(function add2(a, b) { return a + b; });
db.prepare('SELECT add2(?, ?)').get(12, 4); // => 16
db.prepare('SELECT add2(?, ?)').get('foo', 'bar'); // => 'foobar'
db.prepare('SELECT add2(?, ?, ?)').get(12, 4, 18); // => Error: wrong number of arguments
```

By default, registered functions have a strict number of arguments (determined by `function.length`). You can register multiple functions of the same name, each with a different number of arguments, causing SQLite3 to execute a different function depending on how many arguments were passed to it. If you register two functions with same name and the same number of arguments, the second registration will erase the first one.

If `options.name` is given, the function will be registered under that name (instead of defaulting to `function.name`).

If `options.varargs` is `true`, the registered function can accept any number of arguments.

If your function is [deterministic](https://en.wikipedia.org/wiki/Deterministic_algorithm), you can set `options.deterministic` to `true`, which may improve performance under some circumstances.

```js
db.register({ name: "void", deterministic: true, varargs: true }, function () {});
db.prepare("SELECT void()").get(); // => null
db.prepare("SELECT void(?, ?)").get(55, 19); // => null
```

You can create custom [aggregates](https://sqlite.org/lang_aggfunc.html) by using generator functions. Your generator function must `yield` a regular function that will be invoked for each row passed to the aggregate.

```js
db.register(function* addAll() {
  let total = 0;
  yield function (rowValue) { total += rowValue; };
  return total;
});
const totalTreasure = db.prepare('SELECT addAll(treasure) FROM dragons').pluck().get();
```

### .loadExtension(*path*) -> *this*

TODO: show code example with useful extension

Loads a compiled [SQLite3 extension](https://sqlite.org/loadext.html) and applies it to the current database connection.

It's your responsibility to make sure the extensions you load are compiled/linked against a version of [SQLite3](https://www.sqlite.org/) that is compatible with `better-sqlite3`. Keep in mind that new versions of `better-sqlite3` will periodically use newer versions of [SQLite3](https://www.sqlite.org/). You can see which version is being used [here](../deps).

### .exec(*string*) -> *this*

Executes the given SQL string. Unlike [prepared statements](#preparestring---statement), this can execute strings that contain multiple SQL statements. This function performs worse and is less safe than using [prepared statements](#preparestring---statement). You should only use this method when you need to execute SQL from an external source (usually a file). If an error occurs, execution stops and further statements are not executed. You must rollback changes manually.

```js
const migration = fs.readFileSync('migrate-schema.sql', 'utf8');

db.exec(migration);
```

### .close() -> *this*

Closes the database connection. After invoking this method, no statements can be created or executed.

TODO: use AtExit c++ hook

```js
process.on('exit', () => {
  db.close();
});
```

## Properties

**.open -> _boolean_** - Whether the database connection is currently open.

**.inTransaction -> _boolean_** - Whether the database connection is currently in an open transaction.

**.name -> _string_** - The string that was used to open the database connection.

**.memory -> _boolean_** - Whether the database is an in-memory database.

**.readonly -> _boolean_** - Whether the database connection was created in readonly mode.

# class *Statement*

An object representing a single SQL statement.

- [Statement#run()](#runbindparameters---object)
- [Statement#get()](#getbindparameters---row)
- [Statement#all()](#allbindparameters---array-of-rows)
- [Statement#iterate()](#iteratebindparameters---iterator)
- [Statement#pluck()](#plucktogglestate---this)
- [Statement#bind()](#bindbindparameters---this)
- [Properties](#properties-1)

### .run([*...bindParameters*]) -> *object*

**(only on statements that do not return data)*

Executes the prepared statement. When execution completes it returns an `info` object describing any changes made. The `info` object has two properties:

- `info.changes`: The total number of rows that were inserted, updated, or deleted by this operation. Changes made by [foreign key actions](https://www.sqlite.org/foreignkeys.html#fk_actions) or [trigger programs](https://www.sqlite.org/lang_createtrigger.html) do not count.
- `info.lastInsertRowid`: The [rowid](https://www.sqlite.org/lang_createtable.html#rowid) of the last row inserted into the database (ignoring those caused by [trigger programs](https://www.sqlite.org/lang_createtrigger.html)). If the current statement did not insert any rows into the database, this number should be completely ignored.

If execution of the statement fails, an `Error` is thrown.

You can specify [bind parameters](#binding-parameters), which are only bound for the given execution.

```js
const stmt = db.prepare('INSERT INTO cats (name, age) VALUES (?, ?)');

const info = stmt.run('Joey', 2);
console.log(info.changes); // => 1
```

### .get([*...bindParameters*]) -> *row*

**(only on statements that return data)*

Executes the prepared statement. When execution completes it returns an object that represents the first row retrieved by the query. The object's keys represent column names.

If the statement was successful but found no data, `undefined` is returned. If execution of the statement fails, an `Error` is thrown.

You can specify [bind parameters](#binding-parameters), which are only bound for the given execution.

```js
const stmt = db.prepare('SELECT age FROM cats WHERE name = ?');

const cat = stmt.get('Joey');
console.log(cat.age); // => 2
```

### .all([*...bindParameters*]) -> *array of rows*

**(only on statements that return data)*

Similar to [`.get()`](#getbindparameters---row), but instead of only retrieving one row all matching rows will be retrieved. The return value is an array of row objects.

If no rows are found, the array will be empty. If execution of the statement fails, an `Error` is thrown.

You can specify [bind parameters](#binding-parameters), which are only bound for the given execution.

```js
const stmt = db.prepare('SELECT * FROM cats WHERE name = ?');

const cats = stmt.all('Joey');
console.log(cats.length); // => 1
```

### .iterate([*...bindParameters*]) -> *iterator*

**(only on statements that return data)*

Similar to [`.all()`](#allbindparameters---array-of-rows), but instead of returning every row together, an [iterator](https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Iteration_protocols) is returned so you can retrieve the rows one by one. If you plan on retrieving every row anyways, [`.all()`](#allbindparameters---array-of-rows) will perform slightly better.

If execution of the statement fails, an `Error` is thrown and the iterator is closed.

You can specify [bind parameters](#binding-parameters), which are only bound for the given execution.

```js
const stmt = db.prepare('SELECT * FROM cats');

for (const cat of stmt.iterate()) {
  if (cat.name === 'Joey') {
    console.log('found him!');
    break;
  }
}
```

### .pluck([toggleState]) -> *this*

**(only on statements that return data)*

Causes the prepared statement to only return the value of the first column of any rows that it retrieves, rather than the entire row object.

You can toggle this on/off as you please:

TODO: implement setting for columns namespaced by table

```js
const stmt = db.prepare(SQL);

stmt.pluck(); // plucking ON
stmt.pluck(true); // plucking ON
stmt.pluck(false); // plucking OFF
```

### .bind([*...bindParameters*]) -> *this*

[Binds the given parameters](#binding-parameters) to the statement *permanently*. Unlike binding parameters upon execution, these parameters will stay bound to the prepared statement for its entire life.

After a statement's parameters are bound this way, you may no longer provide it with execution-specific (temporary) bound parameters.

This method is primarily used as a performance optimization when you need to execute the same prepared statement many times with the same bound parameters.

```js
const stmt = db.prepare('SELECT * FROM cats WHERE name = ?').bind('Joey');

const cat = stmt.get();
console.log(cat.name); // => "Joey"
```

## Properties

**.source -> _string_** - The source string that was used to create the prepared statement.

**.returnsData -> _boolean_** - Whether the prepared statement returns data.

# Binding Parameters

This section refers to anywhere in the documentation that specifies the optional argument [*`...bindParameters`*].

There are many ways to bind parameters to a prepared statement. The simplest way is with anonymous parameters:

```js
const stmt = db.prepare('INSERT INTO people VALUES (?, ?, ?)');

// The following are equivalent.
stmt.run('John', 'Smith', 45);
stmt.run(['John', 'Smith', 45]);
stmt.run(['John'], ['Smith', 45]);
```

You can also use named parameters. SQLite3 provides [3 different syntaxes for named parameters](https://www.sqlite.org/lang_expr.html) (`@foo`, `:foo`, and `$foo`), all of which are supported by `better-sqlite3`.

```js
// The following are equivalent.
const stmt = db.prepare('INSERT INTO people VALUES (@firstName, @lastName, @age)');
const stmt = db.prepare('INSERT INTO people VALUES (:firstName, :lastName, :age)');
const stmt = db.prepare('INSERT INTO people VALUES ($firstName, $lastName, $age)');
const stmt = db.prepare('INSERT INTO people VALUES (@firstName, :lastName, $age)');

stmt.run({
  firstName: 'John',
  lastName: 'Smith',
  age: 45
});
```

Below is an example of mixing anonymous parameters with named parameters.

```js
const stmt = db.prepare('INSERT INTO people VALUES (@name, @name, ?)');
stmt.run(45, { name: 'Henry' });
```
