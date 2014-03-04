
var lifx = require('lifx');
var util = require('util');

var scBulbName = 'Kitchen';

var lx   = lifx.init();
lx.on('bulb', function(b) {
	if(b.name == scBulbName)
	{
		console.log('found the right bulb, starting light adjustment...');

		runLightAdjustment(b);

		//lx.close();
	}
});

function onState(data)
{
	if(data.bulb.name == scBulbName)
	{
		console.log('bulb: ' + util.inspect(data.bulb));
		console.log('state: ' + util.inspect(data.state));	
		lx.removeListener('bulbstate',onState);
		//lx.lightsColour(hue,    saturation, luminance, whiteColour, fadeTime, bulb);
		lx.lightsColour(data.state.hue, data.state.saturation,     0x7700,    data.state.kelvin,      0,   data.bulb);
		lx.close();
	}

}

function runLightAdjustment(b)
{
	console.log('Bulb state: ' + util.inspect(b));
	lx.on('bulbstate',onState);
	lx.requestStatus();

	// check if on or off (per power, either 0x0000 or 0xffff)

	// how to get luminance [or color info for that matter]

	// how to set luminance

	// how to realize max and min levels for luminance

	// how to know when setting luminance is done [you can count if it done immediately or wait for next round]
}