/*
	Script for automatically adjusting the light in a lifx bulb to match a preconfigurd brightness value.
	basically i can grab the current brightness value, and then make sure that it stays the same.
*/


var yargs = require('yargs')
    .usage('application for setting the light to a constant desired brightness level\nUsage: $0 [-rh] [-dc]')
    .alias('r','reset')
    .describe ('r','reset brightness to current light value')
    .alias('d','debug')
    .describe ('d','debug mode, more messages')
    .alias('h','help')
    .describe ('h','show help')
    .alias('c','continous')
    .describe('c','run application continously')
	fs = require('fs'),
	http = require('http'),
	cl = require('../constantlight'),
	lifx = require('lifx'),
	packet = require('./node_modules/lifx/packet'),
	moment = require('moment'),
	util = require('util');
var argv = yargs.argv;

var lx;
var config;
var bulb;

if(argv.h)
{
	yargs.showHelp();
}
else
{
	fs.readFile('./config.json', function (err, data) {
	    if (err) throw err;
	    config = JSON.parse(data);

	    if(argv.r)
	    	updateBrightnessFactor();
	    else
	    	adjustBrightness();
	});
}

function updateBrightnessFactor()
{
	// application mode for setting light value to the current state. grab, save and exit
	grabCurrentBrightnessValue(function(err,inBrightness) {
		    if (err) throw err;
		    config.brightness = inBrightness;
			fs.writeFile('./config.json', JSON.stringify(config, null, 2), function (err) {
	    		console.log('Brightness value reset to',config.brightness);
	        });  
	    });
}

function grabCurrentBrightnessValue(callback)
{
	http.get(config.cameraSnapshotURL, function(res) {
		var imageName = './images/snapshot_' + moment().format('HH_mm_mm') + '.jpg';
		if(argv.d)
			console.log('Debug: saving snapshot image in ',imageName);
		var stream = fs.createWriteStream(imageName);
		res.pipe(stream);
	  	stream.on('finish', function (inImageName) {
	  		if(argv.d)
				console.log('Debug: calculating brightness on',inImageName);
	  		var brightness = cl.calculateBrightness(inImageName)
	  		if(argv.d)
    			console.log('Debug: calculated brightness:',brightness);
    		else
    			fs.unlink(inImageName);
    		callback(null,brightness);
  		}.bind(null,imageName));
  	}).on('error', function(e) {
  		console.log('Got error while grabbing image:',e);
  		callback(e,config.brightness);
	});
 }

 function adjustBrightness()
 {
 	console.log('Starting light adjustment on',moment().format('MMMM Do YYYY, HH:mm:ss'));
	console.log('Desired brightness level',config.brightness);
 	lx = lifx.init();
 	waitForBulbReady(adjustLightWithBulb);
 }

 function waitForBulbReady(inCallback)
 {
 	var state = {callback:inCallback};
 	state.listener = waitForBulbListener.bind(null,state);
 	console.log('Waiting for bulb to become available...');
	lx.on('bulb', state.listener);
 }

 function waitForBulbListener(inState,b) {
 	if(argv.d)
 		console.log('Debug: found bulb',b.name);
	if(b.name == config.bulb)
	{
		console.log('Found bulb',b.name,', Starting Adjustment...');
		bulb = b;
		lx.removeListener('bulb',inState.listener);
		inState.callback(b);
	}
}

function adjustLightWithBulb()
{
	// continously adjust light till happy. don't change light if happy.
	grabCurrentBrightnessValue(adjustLightFromGrabbedValue);
}

function adjustLightFromGrabbedValue(err,inBrightness)
{
	if(err) throw err;
	
	console.log('Current Brightness',inBrightness);

	if(inBrightness > config.brightness+config.tolerance)
	{
		if(argv.d)
			console.log('Debug: going darker');
		goDarker();
	}
	else if(inBrightness < config.brightness-config.tolerance)
	{
		if(argv.d)
			console.log('Debug: going lighter');
		goLighter();
	}
	else
	{
		console.log('K, no further adjustments required, done here');
		finishAdjustment();
	}
}

function goDarker()
{
	getCurrentBulbState(function(inBulbState){

		if(inBulbState.brightness == 0x0000 || inBulbState.power == 0x0000)
		{
			console.log('Bulb at min level, cannot adjust light to darker, Stopping');
			finishAdjustment();
			return;
		}

		var illuminationValue = (inBulbState.brightness-config.illuminationStepSize) < 0x0000 ? 0x0000: (inBulbState.brightness-config.illuminationStepSize);
		var power;
		if(illuminationValue < config.turnOffLevel)
		{
			console.log('Turning off bulb');
			lx.lightsOff(bulb);
	 		waitForSettingToAdjust({power:0x0000},adjustLightWithBulb);
	}
		else
		{
			power = 0xffff;
 		if(argv.d)
				console.log('Debug: setting illumination to',illuminationValue,inBulbState.power == 0x0000 ? ' and turning lights on':'');
 		waitForSettingToAdjust({brightness:illuminationValue,power:power},adjustLightWithBulb);
 		if(argv.d)
 			console.log('Debug: before lights colour');
		lx.lightsColour(inBulbState.hue, inBulbState.saturation,illuminationValue,inBulbState.kelvin,0, bulb);
 		if(argv.d)
 			console.log('Debug: after lights colour');
		}

	},function(){console.log('Cannot communicate with the bulb, Stopping [go darker]'); finishAdjustment();});
}

function goLighter()
{
	getCurrentBulbState(function(inBulbState){
		
		if(inBulbState.brightness == 0xffff && inBulbState.power == 0xffff)
		{
			console.log('Bulb at max level, cannot adjust light to lighter, Stopping');
			finishAdjustment();
			return;
		}

		var illuminationValue = (inBulbState.brightness+config.illuminationStepSize) > 0xffff ? 0xffff: (inBulbState.brightness+config.illuminationStepSize);
		if(argv.d)
			console.log('Debug: changing illumination to ',illuminationValue, inBulbState.power == 0x0000 ? 'and turning lights on':'');

		waitForSettingToAdjust({brightness:illuminationValue,power:0xffff},adjustLightWithBulb);
		if(inBulbState.power == 0x0000)
		{
			console.log('Turning on bulb');
			lx.lightsOn(bulb);
		}
		if(argv.d)
			console.log('Debug: before lights colour');
		lx.lightsColour(inBulbState.hue, inBulbState.saturation,illuminationValue,inBulbState.kelvin,0, bulb);
		if(argv.d)
			console.log('Debug: after lights colour');

	},function(){console.log('Cannot communicate with the bulb, Stopping [go lighter]'); finishAdjustment();});
}


function getCurrentBulbState(callback,fallbackcallback)
{
	var recieved = false;
	lx.once('bulbstate',function(data)
		{
			recieved = true;
			if(data.bulb.name != bulb.name)
			{
				if(argv.d)
					console.log('Got state for', data.bulb.name, 'expecting state for',bulb.name);
				getCurrentBulbState(callback,fallbackcallback) // not supposed to happen...as i am sending a packet only to that bulb...but make sure
			}

			callback(data.state);
		});
	lx._sendToOneOrAll(packet.getLightState(),bulb); // send getstate to just the bulb that we want

	// retry call to get state till i get it done
	var retries = 0;
	setTimeout(function()
	{
		if(!recieved)
		{
			lx._sendToOneOrAll(packet.getLightState(),bulb);
			++retries;
			// if failed retries to get state, revert to fallback
			if(retries == 5)
				fallbackcallback();
		}
	},5000);


}

function waitForSettingToAdjust(inWaitForState,inCallback,inRetries)
{
	getCurrentBulbState(function(inBulbState)
		{
			var allOK = true;
			for(var key in inWaitForState)
			{
				if(inWaitForState[key] != inBulbState[key])
					allOK = false;
			}

			if(allOK)
			{
				inCallback();
			}
			else
			{
				setTimeout(function()
				{
					if(inRetries > 0 && (inRetries % 5 == 0))
					{
						if(inRetries == 25)
						{
							console.log('Cannot communicate with the bulb, Stopping [25 retries in waitForSettingToAdjust]'); 
							finishAdjustment();
							return;
						}

						// k, maybe command didn't work that well, try again.
						console.log('Adjustment command didnt work, trying again. This might mean communication issues if this happens again');

						// adjust power
						if(inWaitForState.power == 0x0000 && inWaitForState.power != undefined && inBulbState == 0xffff)
							lx.lightsOff(bulb);
						else if(inWaitForState.power = 0xffff && inBulbState == 0x0000)
							lx.lightsOn(bulb);

						// adjust brightness
						if(inWaitForState.brightness != undefined && inWaitForState.brightness != inWaitForState.brightness)
							lx.lightsColour(inBulbState.hue, inBulbState.saturation,inWaitForState.brightness,inBulbState.kelvin,0, bulb);
						waitForSettingToAdjust(inWaitForState,inCallback,inRetries+1);

					}
					else
						waitForSettingToAdjust(inWaitForState,inCallback,inRetries ? inRetries+1:1);
				},1000);
			}
		},function(){console.log('Cannot communicate with the bulb, Stopping [waitForSettingToAdjust]'); finishAdjustment();});
}

function finishAdjustment()
{
	lx.close();
	console.log('Done with light adjustment');
	if(argv.c)
	{
		console.log('Waiting for next round (press ctrl-c to break)...');
		setTimeout(adjustBrightness,config.interAjdustmentsWait);
	}
}

