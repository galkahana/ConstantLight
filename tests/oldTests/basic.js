var cl = require('../constantlight');


var fs = require('fs');

if (!String.prototype.endsWith) {
    Object.defineProperty(String.prototype, 'endsWith', {
                          enumerable: false,
                          configurable: false,
                          writable: false,
                          value: function (searchString, position) {
                          position = position || this.length;
                          position = position - searchString.length;
                          return this.lastIndexOf(searchString) === position;
                          }
                          });
}

var files = fs.readdirSync('./images');
files.forEach(
        function(element, index, array)
              {
                    var aString = element.toString();
                    if(aString.endsWith('.jpg'))
                    {
						console.log(cl.calculateBrightness('./images/' + aString));
                    }
              }
    );
console.log('done - ok');