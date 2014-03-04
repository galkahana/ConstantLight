
var fs = require('fs');
var http = require('http');
var cl = require('../constantlight');

http.get("http://guest:guest@10.0.0.174/snapshot.jpg", function(res) {
	res.pipe(fs.createWriteStream('./images/snapshot.jpg'))   

  	res.on('end', function (chunk) {
    	console.log('Current image brightness:',cl.calculateBrightness('./images/snapshot.jpg'));
    	console.log('Done');
  	});

}).on('error', function(e) {
  	console.log("Got error: " + e.message);
});