var _ = require('underscore'),
    async = require('async'),
    fs = require('fs'),
    path = require('path'),
    utils = require('./utils').utils,
    excelParser = {};

excelParser.worksheets = function(options, cb) {
  var worksheets, cb = cb && typeof(cb) === "function" ? cb : null, args = [];
  if(!cb) throw new Error("worksheets will required two arguments. worksheets(options, callback)")
  else if(!options || typeof(options) !== 'object') return cb("Invalid arguments.");
  else if(!options.inFile) return cb("File is missing in arguments");

  fs.exists(options.inFile, function(exists) {
    if(!exists) return cb("File not found");
    args = ['-x', '"'+path.relative(__dirname, options.inFile)+'"', '-W'];
    utils.execute(args, function(err, stdout) {
      if(err) return cb(err);
      worksheets = _.compact(stdout.split(/\n/));
      if(worksheets) return cb(null, JSON.parse(worksheets));
      else return cb(new Error("Not found any worksheet in given spreadsheet"));
    });
  });
};

excelParser.parse = function(options, cb) {
  var args = [], _this = this;
  if(!cb) throw new Error("worksheets will required two arguments. worksheets(options, callback)");
  else if(!options || typeof(options) !== 'object') return cb("Invalid arguments.");
  else if(!options.inFile) return cb("File is missing in arguments");
  fs.exists(options.inFile, function(exists) {
    if(!exists) return cb("File not found");
    args = ['-x', '"'+path.relative(__dirname, options.inFile)+'"'];
    if(!options.worksheet) {
      var records = [];
      _this.worksheets(options, function(err, worksheets) {
        if(err) return cb(err);
        async.series(_.map(worksheets, function(worksheet) {
          return function(rcb) {
            args.push('-n', worksheet.id);
            utils.pickRecords(args, options, function(err, parsed) {
              if(err) return cb(err);
              records.push(parsed);
              rcb(null);
            });
          }
        }), function(err, parsed) {
          if(err) return cb(err);
          cb(null, parsed);
        });
      });
    } else {
      if(typeof(options.worksheet) === 'number') {
        args.push('-n', options.worksheet);
      } else if(typeof(options.worksheet) === 'string') {
        args.push('-w', '"'+ options.worksheet.trim() + '"');
      } else {
        return callback(new Error('"worksheet" should be string or intiger'));
      }
      utils.pickRecords(args, options, function(err, parsed) {
        if(err) return cb(err);
        cb(null, parsed);
      });
    }
  });
};
module.exports = excelParser;