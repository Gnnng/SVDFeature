var _ = require('underscore'),
    async = require('async'),
    exec = require('child_process').exec,
    fs = require('fs'),
    path = require('path'),
    temp = require('temp'),
    utils = {},
    _this = this;

var _CRCsv = function(args, cb) {
  temp.mkdir('temp', function(err, dirPath) {
    if(err) return cb(err);
    csvFile = path.join(dirPath, 'convert.csv');
    args.push('-c', csvFile);
    utils.execute(args, function(err, stdout) {
      if(err) return cb(err);
      fs.readFile(csvFile, 'utf-8', function(err, csv_data) {
        if(err) return cb(err);
        cb(null, csv_data);
      });
    });
  });
};

var _parseCSV = function(csv_data, cb) {
  if(!csv_data) return cb(null, []);
  var pattern = /(\,|\r?\n|\r|^)(?:"([^"]*(?:""[^"]*)*)"|([^"\,\r\n]*))/gi,
      data = [[]],
      matched = [],
      strMatchedValue;

  while(matched = pattern.exec(csv_data)) {
    var strMatchedDelimiter = matched[1];
    if (
      strMatchedDelimiter.length &&
      (strMatchedDelimiter != ",") &&
      !_.isEmpty(data[data.length-1])
    ) {
      data.push([]);
    }

    if(matched[2]) strMatchedValue = matched[2].replace(/""/g, "\"");
    else strMatchedValue = matched[3];

    data[data.length-1].push(strMatchedValue);
  }

  if(data[data.length-1].length === 1)
    data.splice(data.length-1, 1);

  cb(null, data);
};

var _skipEmpty = function(records, options, cb) {
  if(_.isEmpty(records)) return cb(null, []);

  var skipEmpty = false;
  if(options.skipEmpty && typeof(options.skipEmpty) === 'boolean')
    skipEmpty = options.skipEmpty;

  if(skipEmpty) return cb(null, _.map(records, function(record){return _.compact(record)}));
  else return cb(null, records);
};

var _searchInArray = function(records, options, cb) {
  if(_.isEmpty(records)) return cb(null, []);
  var term, s = "strict", searchType = l = "loose", searchPattern, searched=[];
  if(!options.searchFor) return cb(null, records);
  if(!options.searchFor.term && options.searchFor.term !== 'object')
    return cb(null, records);
  term = _.map(options.searchFor.term, function(t) {
    return t.replace(/\//g, '');
  });

  if(
    options.searchFor.type &&
    (
      options.searchFor.type.trim() === l ||
      options.searchFor.type.trim() === s
    )
  ) searchType = options.searchFor.type;

  if(searchType === s) {
    searchPattern = _.map(term, function(s) { return "\\b" + s + "\\b"; });
    searchPattern = new RegExp('(' + searchPattern.join('|') + ')', "g");
  } else searchPattern = new RegExp('(' + term.join('|') + ')', "gi");

  for(var i=0, len=records.length; i< len; ++i) {
    var strRow = records[i].join(' ').replace(/\//g, '');
    if(searchPattern.test(strRow)) searched.push(records[i]);
  }
  cb(null, searched);
};

utils.execute = function(args, cb) {
  var cmd = ["python", __dirname + "/convert.py"];
  cmd = cmd.concat(args);
  exec(
    cmd.join(' '),
    {cwd: __dirname},
    function(err, stdout, stderr) {
      if(err) return cb(err);
      if(stderr) return cb(stderr);
      cb(null, stdout);
    }
  );
};

utils.pickRecords = function(args, options, cb) {
  var csvFile;
  async.waterfall([
    function(cb){ _CRCsv(args, cb) },
    function(csv_data, cb){ _parseCSV(csv_data, cb) },
    function(parsedArray, cb) { _skipEmpty(parsedArray, options, cb) },
    function(parsedArray, cb) { _searchInArray(parsedArray, options, cb) }
  ], function(err, parsed) {
    if(err) return cb(err);
    cb(null, parsed);
  });
};

exports.utils = utils;