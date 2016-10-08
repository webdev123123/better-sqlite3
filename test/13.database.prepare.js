var expect = require('chai').expect;
var Database = require('../.');
var util = (function () {
	var path = require('path');
	var dbId = 0;
	var obj;
	return obj = {
		current: function () {
			return 'temp/' + path.basename(__filename).split('.')[0] + '.' + dbId + '.db';
		},
		next: function () {++dbId; return obj.current();}
	};
}());

describe('Database#prepare()', function () {
	it('should throw an exception if a string is not provided', function (done) {
		var db = new Database(util.next());
		db.on('open', function () {
			expect(function () {db.prepare(123);}).to.throw(TypeError);
			expect(function () {db.prepare(0);}).to.throw(TypeError);
			expect(function () {db.prepare(null);}).to.throw(TypeError);
			expect(function () {db.prepare();}).to.throw(TypeError);
			expect(function () {db.prepare(new String('CREATE TABLE people (name TEXT)'));}).to.throw(TypeError);
			done();
		});
	});
	it('should throw an exception if invalid SQL is provided', function (done) {
		var db = new Database(util.next());
		db.on('open', function () {
			expect(function () {db.prepare('CREATE TABLE people (name TEXT');}).to.throw(Error);
			expect(function () {db.prepare('INSERT INTO people VALUES (?)');}).to.throw(Error);
			done();
		});
	});
	it('should throw an exception if no statements are provided', function (done) {
		var db = new Database(util.next());
		db.on('open', function () {
			expect(function () {db.prepare('');}).to.throw(TypeError);
			expect(function () {db.prepare(';');}).to.throw(TypeError);
			done();
		});
	});
	it('should throw an exception if more than one statement is provided', function (done) {
		var db = new Database(util.next());
		db.on('open', function () {
			expect(function () {db.prepare('CREATE TABLE people (name TEXT);CREATE TABLE animals (name TEXT)');}).to.throw(TypeError);
			expect(function () {db.prepare('CREATE TABLE people (name TEXT); ');}).to.throw(TypeError);
			expect(function () {db.prepare('CREATE TABLE people (name TEXT);;');}).to.throw(TypeError);
			done();
		});
	});
	it('should create a prepared Statement object', function (done) {
		function assertStmt(stmt, source) {
			expect(stmt.source).to.equal(source);
			expect(stmt.constructor.name).to.equal('Statement');
			expect(stmt.database).to.equal(db);
			expect(stmt.returnsData).to.equal(false);
			expect(function () {
				new stmt.constructor(source);
			}).to.throw(TypeError);
		}
		var db = new Database(util.next());
		db.on('open', function () {
			var stmt1 = db.prepare('CREATE TABLE people (name TEXT)');
			var stmt2 = db.prepare('CREATE TABLE people (name TEXT);');
			assertStmt(stmt1, 'CREATE TABLE people (name TEXT)');
			assertStmt(stmt2, 'CREATE TABLE people (name TEXT);');
			expect(stmt1).to.not.equal(stmt2);
			expect(stmt1).to.not.equal(db.prepare('CREATE TABLE people (name TEXT)'));
			done();
		});
	});
	it('should create a prepared Statement object with just an expression', function (done) {
		var db = new Database(util.next());
		db.on('open', function () {
			var stmt = db.prepare('SELECT 555');
			expect(stmt.source).to.equal('SELECT 555');
			expect(stmt.constructor.name).to.equal('Statement');
			expect(stmt.database).to.equal(db);
			expect(stmt.returnsData).to.equal(true);
			expect(function () {
				new stmt.constructor('SELECT 555');
			}).to.throw(TypeError);
			done();
		});
	});
});
