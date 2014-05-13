"use strict"
var fs = require('fs');

function newLine(item, judge, score) {
  return [score, '0', '1', '1', judge + ':1', item + ':1'].join(' ');
}

fs.readFile('record.csv', function(err, data){
  if (err) console.log(err);
  var newFile = '';
  console.log(data.toString().split('\r\n'));
  var oldLines = data.toString().split('\r\n');
  oldLines.shift();
  oldLines.forEach(function(line) {
    if (!line) return;
    var arr = line.split(',');
    newFile += newLine(arr[0], arr[1].replace('A', ''), arr[2]) + '\n';
    newFile += newLine(arr[0], arr[3].replace('A', ''), arr[4]) + '\n';
    newFile += newLine(arr[0], arr[5].replace('A', ''), arr[6]) + '\n';
  })
  // console.log(newFile);
  fs.writeFile('art.base', newFile, function(err) {
    if (err) console.log(err);
  });
});