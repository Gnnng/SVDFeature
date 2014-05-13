# excel-parser

[node](http://nodejs.org/) wrapper for parsing spreadsheets. Supports xls, xlsx.

You can install this module using [npm](http://github.com/isaacs/npm):

    npm install excel-parser

Requires [python](http://www.python.org/) to be installed

For system-specific installation view the [Wiki](https://github.com/vxtindia/excel-parser/wiki)

## API

<a name="worksheets" />
### worksheets(options, callback(err, worksheets))

Get all the worksheets list in given spreadsheet where options are the object of arguments. The result is returned as a object.

__Arguments__

* inFile - Filepath of the source speadsheet

__Example__

```js
var excelParser = require('excel-parser');
excelParser.worksheets({
  inFile: 'my_file.in'
}, function(err, worksheets){
  if(err) console.error(err);
  consol.log(worksheets);
});
```
__Sample output__

```json
[
  {'name': 'Sheet1', id: 1},
  {'name': 'Sheet2', id: 2}
]
```
---------------------------------------
<a name="parse" />
### parse(options, callback(err, records))

Parse spreadsheet with given optinos as an arguments. The result is returned as an array.

__Arguments__

* inFile - Filepath of the source speadsheet
* worksheet - Worksheet name or Id to parse. If nothing specified then default
is 0 and parsed all the worksheets.
* skipEmpty - boolean `true` or `false`. Pass true if want to skip empty cells from spreadsheet.
* searchFor - Object with `term` and `type` values. If you want to get limited
matching rows from spreadsheet then add the matching string array and one of the following search type.
** 'strict' is for the case sensetive and exact string matched.
** 'loose' is for the case insensetive and match relatively.

__Example__

```javascript
var excelParser = require('excel-parser');

excelParser.parse({
  inFile: 'my_file.in',
  worksheet: 1,
  skipEmpty: true,
  searchFor: {
    term: ['my serach term'],
    type: 'loose'
  }
},function(err, records){
  if(err) console.error(err);
  consol.log(records);
});
```
__Sample output__

```json
[
  ['ID', 'Name', 'City'],
  ['1', 'joe', 'Sandy Springs'],
  ['1', 'cole', 'City of Industry']
]
```

## Running Tests

There are unit tests in `test/` directory. To run test suite first run the following command to install dependencies.

    npm install

then run the tests:

    grunt nodeunit

NOTE: Install `npm install -g grunt-cli` for running tests.

## License

Copyright (c) 2013 Shekhar R. Thawali

Licensed under the MIT license.