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

describe('Database#transaction()', function () {
	it('should throw an exception if an array of strings is not provided', function (done) {
		var db = new Database(util.next());
		db.on('open', function () {
			expect(function () {db.transaction(123);}).to.throw(TypeError);
			expect(function () {db.transaction(0);}).to.throw(TypeError);
			expect(function () {db.transaction(null);}).to.throw(TypeError);
			expect(function () {db.transaction();}).to.throw(TypeError);
			expect(function () {db.transaction(new String('CREATE TABLE people (name TEXT)'));}).to.throw(TypeError);
			expect(function () {db.transaction({0: 'CREATE TABLE people (name TEXT)', length: 1});}).to.throw(TypeError);
			expect(function () {db.transaction([123]);}).to.throw(TypeError);
			expect(function () {db.transaction([0]);}).to.throw(TypeError);
			expect(function () {db.transaction([null]);}).to.throw(TypeError);
			expect(function () {db.transaction([new String('CREATE TABLE people (name TEXT)')]);}).to.throw(TypeError);
			done();
		});
	});
	it('should throw an exception if no strings are provided', function (done) {
		var db = new Database(util.next());
		db.on('open', function () {
			expect(function () {db.transaction([]);}).to.throw(TypeError);
			done();
		});
	});
	it('should propagate exceptions thrown from array accessors', function (done) {
		var db = new Database(util.next());
		var err = new Error('foobar');
		var arr = ['foo'];
		Object.defineProperty(arr, '0', {get: function () {throw err;}});
		db.on('open', function () {
			expect(function () {db.transaction(arr);}).to.throw(err);
			done();
		});
	});
	it('should throw an exception if invalid SQL is provided', function (done) {
		var db = new Database(util.next());
		db.on('open', function () {
			expect(function () {db.transaction(['CREATE TABLE people (name TEXT']);}).to.throw(Error);
			expect(function () {db.transaction(['INSERT INTO people VALUES (?)']);}).to.throw(Error);
			done();
		});
	});
	it('should throw an exception if a string contains no statements', function (done) {
		var db = new Database(util.next());
		db.on('open', function () {
			expect(function () {db.transaction(['']);}).to.throw(TypeError);
			expect(function () {db.transaction([';']);}).to.throw(TypeError);
			expect(function () {db.transaction(['CREATE TABLE people (name TEXT)', '']);}).to.throw(TypeError);
			expect(function () {db.transaction(['CREATE TABLE people (name TEXT)', ';']);}).to.throw(TypeError);
			done();
		});
	});
	it('should throw an exception if multiple statements exist in one string', function (done) {
		var db = new Database(util.next());
		db.on('open', function () {
			expect(function () {db.transaction(['CREATE TABLE people (name TEXT);CREATE TABLE animals (name TEXT)']);}).to.throw(TypeError);
			expect(function () {db.transaction(['CREATE TABLE people (name TEXT); ']);}).to.throw(TypeError);
			expect(function () {db.transaction(['CREATE TABLE people (name TEXT);;']);}).to.throw(TypeError);
			done();
		});
	});
	it('should throw an exception if any read-only statements are provided', function (done) {
		var db = new Database(util.next());
		db.on('open', function () {
			expect(function () {db.transaction(['SELECT 555']);}).to.throw(TypeError);
			expect(function () {db.transaction(['BEGIN TRANSACTION']);}).to.throw(TypeError);
			expect(function () {db.transaction(['COMMIT TRANSACTION']);}).to.throw(TypeError);
			expect(function () {db.transaction(['ROLLBACK TRANSACTION']);}).to.throw(TypeError);
			expect(function () {db.transaction(['CREATE TABLE people (name TEXT)', 'SELECT 555']);}).to.throw(TypeError);
			expect(function () {db.transaction(['SELECT 555', 'CREATE TABLE people (name TEXT)']);}).to.throw(TypeError);
			expect(function () {db.transaction(['CREATE TABLE people (name TEXT)', 'BEGIN TRANSACTION']);}).to.throw(TypeError);
			expect(function () {db.transaction(['CREATE TABLE people (name TEXT)', 'COMMIT TRANSACTION']);}).to.throw(TypeError);
			expect(function () {db.transaction(['CREATE TABLE people (name TEXT)', 'ROLLBACK TRANSACTION']);}).to.throw(TypeError);
			done();
		});
	});
	it('should create a prepared Transaction object', function (done) {
		var db = new Database(util.next());
		db.on('open', function () {
			var trans1 = db.transaction(['CREATE TABLE people (name TEXT)']);
			var trans2 = db.transaction(['CREATE TABLE people (name TEXT)', 'CREATE TABLE animals (name TEXT);']);
			var trans3 = db.transaction(['CREATE TABLE people (name TEXT);', 'CREATE TABLE animals (name TEXT)']);
			expect(trans1.source).to.equal('CREATE TABLE people (name TEXT);');
			expect(trans2.source).to.equal('CREATE TABLE people (name TEXT);\nCREATE TABLE animals (name TEXT);');
			expect(trans2.source).to.equal(trans3.source);
			expect(trans1.constructor.name).to.equal('Transaction');
			expect(trans2.constructor.name).to.equal('Transaction');
			expect(trans3.constructor.name).to.equal('Transaction');
			expect(trans1.database).to.equal(db);
			expect(trans2.database).to.equal(db);
			expect(trans3.database).to.equal(db);
			expect(function () {
				new trans1.constructor(['CREATE TABLE people (name TEXT)']);
			}).to.throw(TypeError);
			expect(trans1).to.not.equal(trans2);
			expect(trans1).to.not.equal(trans3);
			expect(trans2).to.not.equal(trans3);
			expect(trans1).to.not.equal(db.transaction(['CREATE TABLE people (name TEXT)']));
			done();
		});
	});
});
