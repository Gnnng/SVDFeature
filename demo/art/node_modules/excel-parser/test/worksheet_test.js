'user strict';
var excelParser = require('../excelParser.js'),
		worksheets = {};

worksheets.fromXls = function(test) {
	test.expect(4);
	excelParser.worksheets({inFile: __dirname + '/master.xls'}, function(err, worksheets){
		test.ok(worksheets, "Found all the worksheets");
		var s1 = worksheets[0].name;
		var i2 = worksheets[1].id;
		test.strictEqual(s1, 'Sheet1', 'string', "Should be string type.");
		test.strictEqual(i2, 2, 'number', "Should be number.");
		test.ifError(err);
		test.done();
	});
};

worksheets.fromXlsx = function(test) {
	test.expect(4);
	excelParser.worksheets({inFile: __dirname + '/master.xlsx'}, function(err, worksheets){
		test.ok(worksheets, "Found all the worksheets");
		var s1 = worksheets[0].name;
		var i2 = worksheets[1].id;
		test.strictEqual(s1, 'Sheet1', 'string', "Should be string type.");
		test.strictEqual(i2, 2, 'number', "Should be number.");
		test.ifError(err);
		test.done();
	});
};
exports.worksheets = worksheets;