'user strict';

var excelParser = require('../excelParser.js'),
    parser = {};

parser.parse_10000_xls = function(test) {
  test.expect(4);
  excelParser.parse({
    inFile: __dirname + '/master.xls',
    worksheet: 1
  }, function(err, results) {
    var len = results.length,
        r1 = results[0][0],
        r10000 = results[results.length-1][0];

    test.strictEqual(len, 10000, 'number', 'Should be number');
    test.strictEqual(r1, 'Name', 'string', 'Should be string');
    test.strictEqual(r10000, 'Airhole', 'string', 'Should be string');
    test.ifError(err);
    test.done();
  });
};

parser.parse_10000_xlsx = function(test) {
  test.expect(4);
  excelParser.parse({
    inFile: __dirname + '/master.xlsx',
    worksheet: 1
  }, function(err, results) {
    var len = results.length,
        r1 = results[0][0],
        r10000 = results[results.length-1][0];

    test.strictEqual(len, 10000, 'number', 'Should be number');
    test.strictEqual(r1, 'Name', 'string', 'Should be string');
    test.strictEqual(r10000, 'Airhole', 'string', 'Should be string');
    test.ifError(err);
    test.done();
  });
};

parser.searchLooseWithXls = function(test) {
  test.expect(4);
  excelParser.parse({
    inFile: __dirname + '/master.xls',
    worksheet: 1,
    searchFor: {
      term: ["baker"],
      type: "loose"
    }
  }, function(err, records) {
    test.ok(records, "Successfully parsed all worksheets from multi_worksheets.xls");
    var a = records.length;
    var e = records[0][0];
    test.strictEqual(a, 1699, 'number', "Found 3 records");
    test.strictEqual(e, 'Baker', 'string', 'Found it');
    test.ifError(err);
    test.done();
  });
};

parser.searchLooseWithXlsx = function(test) {
  test.expect(4);
  excelParser.parse({
    inFile: __dirname + '/master.xlsx',
    worksheet: 1,
    searchFor: {
      term: ["baker"],
      type: "loose"
    }
  }, function(err, records) {
    test.ok(records, "Successfully parsed all worksheets from multi_worksheets.xls");
    var a = records.length;
    var e = records[0][0];
    test.strictEqual(a, 1699, 'number', "Found 3 records");
    test.strictEqual(e, 'Baker', 'string', 'Found it');
    test.ifError(err);
    test.done();
  });
};

parser.searchStrictWithXls = function(test) {
  test.expect(4);
  excelParser.parse({
    inFile: __dirname + '/master.xls',
    worksheet: 1,
    searchFor: {
      term: ["Denim"],
      type: "strict"
    }
  }, function(err, records) {
    test.ok(records, "Successfully parsed all worksheets from multi_worksheets.xls");
    var a = records.length;
    var e = records[0][0];
    test.strictEqual(a, 1710, 'number', "Found 3 records");
    test.strictEqual(e, 'Denim', 'string', 'Found it');
    test.ifError(err);
    test.done();
  });
};

parser.searchStrictWithXlsx = function(test) {
  test.expect(4);
  excelParser.parse({
    inFile: __dirname + '/master.xlsx',
    worksheet: 1,
    searchFor: {
      term: ["Denim"],
      type: "strict"
    }
  }, function(err, records) {
    test.ok(records, "Successfully parsed all worksheets from multi_worksheets.xls");
    var a = records.length;
    var e = records[0][0];
    test.strictEqual(a, 1710, 'number', "Found 3 records");
    test.strictEqual(e, 'Denim', 'string', 'Found it');
    test.ifError(err);
    test.done();
  });
};

parser.skipEmptyForXls = function(test) {
  test.expect(4);
  excelParser.parse({
    inFile: __dirname + '/master.xls',
    worksheet: 1,
    skipEmpty: true
  }, function(err, records) {
    test.ok(records, "Successfully parsed all worksheets from multi_worksheets.xls");
    var a = records.length;
    var e = records[9][1];
    test.strictEqual(a, 10000, 'number', "Found 10000 records");
    test.strictEqual(e, 'Test product description', 'string', 'Found it');
    test.ifError(err);
    test.done();
  });
}

parser.skipEmptyForXlsx = function(test) {
  test.expect(4);
  excelParser.parse({
    inFile: __dirname + '/master.xlsx',
    worksheet: 1,
    skipEmpty: true
  }, function(err, records) {
    test.ok(records, "Successfully parsed all worksheets from multi_worksheets.xls");
    var a = records.length;
    var e = records[10][0];
    test.strictEqual(a, 10000, 'number', "Found 10000 records");
    test.strictEqual(e, '1803', 'string', 'Found it');
    test.ifError(err);
    test.done();
  });
}

parser.emptyWorksheetForXls = function(test) {
  test.expect(2);
  excelParser.parse({
    inFile: __dirname + '/master.xls',
    worksheet: 2
  }, function(err, records) {
    var a = records.length;
    test.strictEqual(a, 0, 'number', "Found 0 records");
    test.ifError(err);
    test.done();
  });
}

parser.emptyWorksheetForXlsx = function(test) {
  test.expect(2);
  excelParser.parse({
    inFile: __dirname + '/master.xlsx',
    worksheet: 2
  }, function(err, records) {
    var a = records.length;
    test.strictEqual(a, 0, 'number', "Found 0 records");
    test.ifError(err);
    test.done();
  });
}

exports.parser = parser;