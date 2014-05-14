"use strict"
var fs = require('fs');
var sum = 0.0;

function newLine(it, ju, score) {
  var item = parseInt(it, 10) - 1;
  var judge = parseInt(ju, 10) - 1;
  sum += parseInt(score, 10);
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
  console.log(sum / (230 * 3));
  fs.writeFile('art.base', newFile, function(err) {
    if (err) console.log(err);
  });
});