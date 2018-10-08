'use strict';

module.exports = new Map([
	['better-sqlite3', async (filename, pragma) => {
		const db = require('../.')(filename);
		db.pragma('journal_mode = WAL');
		db.pragma('synchronous = 1');
		for (const str of pragma) db.pragma(str);
		return db;
	}],
	['node-sqlite3', async (filename, pragma) => {
		const db = await require('sqlite').open(filename);
		await db.run('PRAGMA journal_mode = WAL');
		await db.run('PRAGMA synchronous = 1');
		for (const str of pragma) await db.run(`PRAGMA ${str}`);
		return db;
	}],
]);
