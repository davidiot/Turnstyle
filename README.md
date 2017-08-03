# Turnstyle

Created for Intel Hacks 2017
Submitted by David Zhou, Daniel Zhou, and Jason Liu

## Instructions

## The Turnstyle Unit
+ Please refer to the [DevPost](https://devpost.com/software/turnstyle) for a complete wiring diagram of our hardware application.
+ Complete Parts List:
	+ [Arduino 101](https://www.sparkfun.com/products/13787)
	+ [2 SR04 Ultrasonic PING sensors](https://www.sparkfun.com/products/13959)
	+ [Magnetic contact switch](https://www.adafruit.com/product/375)
	+ [M/F jumper cables](https://www.sparkfun.com/products/12794)
	+ [Adafruit RGB LCD](https://www.adafruit.com/product/714)
	+ [Jumper wire kit](https://www.sparkfun.com/products/124)
	+ [Mini Breadboard](https://www.sparkfun.com/products/12043)
	+ [Mini USB cable](https://www.sparkfun.com/products/512)
	+ ZTE Maven 2 Android Phone
	+ Cell Phone 5V battery charger
+ The Arduino code for the application can be found in ```Arduino/Turnstyle/Turnstyle.ino```.
+ For a more detailed serial printout for debugging purposes, set the ```debug``` boolean to true.  Note that doing this will make the unit incompatible with the Node server, but will make it easier to find possible hardware problems.
+ Using the buttons on the LCD, it is possible to change the orientation of the Turnstyle (the direction that is considered entering).
+ Walk in an out of the door and the Turnstyle will keep track of the room's population!  The Turnstyle unit will only sense movement if the door is open beyond a certain angle.  

## The Node Streaming Server
+ By piping the serial output of the Arduino into our ```Node.js``` server, we can stream the data from the Turnstyle unit in real time.
+ The instructions for setting up the server are as follows:
    + Install [Node.js](https://nodejs.org/en/download/) (we used the LTS version): 
    + Open a terminal and install serialport using ```npm install serialport```.
    + Make sure the ```portname``` in the Node ```server.js``` file (located in the ```Node/``` directory of the repo) is the same listed in the Arduino (ours was ```COM4``` on a Windows machine).
    + Run the Arduino sketch, and make sure the ```debug``` variable to false.  The serial monitor printout should be population numbers delimited with ```*```s.
    + Run the server in a terminal using ```node server.js```.
    + The plots will update in real time; navigate to their respective URLs to view your data!
+ This server uses the ```plotly``` [streaming API](https://plot.ly/streaming/).
+ The live dashboard can be found at [here](https://plot.ly/dashboard/Turnstyle:4/view).  This is a proof of concept of a web application that can be accessed by anyone and offer valuable real-time data to help reduce overcrowding.
+ The three individual plots can be viewed separately and are explained as follows:
    + The [Population vs. Time](https://plot.ly/~Turnstyle/0/population-vs-time/) plot updates in real time and shows population as a function of time.  This graph gives people live updates about how crowded a place of interest is.
    + The [Average Population](https://plot.ly/~Turnstyle/2/average-population-per-hour/) is similar to the first plot, but is averaged over each hour and aggregated over the course of several days.  While the first plot is great for getting real-time data, this plot gives people an idea of the generally most popular times at any particular location.
    + The [Traffic](https://plot.ly/~Turnstyle/1/traffic-per-hour/) plot shows total population changes for each hour, accumulated over a long period of time.  This plot will allow people to figure out when traffic is worst in spaces such as schools, workplaces, and gyms.  Hopefully, it will encourage people to arrive or leave slightly earlier or later and mitigate the effects of rush hours and traffic jams.
+ Note that because we are using the free version of the ```plotly``` API, all data will disappear after a certain amount of time, and there is a limit to the number of API calls we can make.

### The Mobile App
+ The mobile application can be deployed using [Evothings Studio](https://evothings.com/download/).  The instructions for using it are included in the download link.  The Evothings application located in the ```Evothings``` folder of the repository can be sent to a mobile device using Evothings Studio, a working internet connection, and one of the following applications:

#### Android
+ Download the [Evothings Viewer](https://play.google.com/store/apps/details?id=com.evothings.evothingsviewer&hl=en) from the Google Play Store.

#### iOS:
+ Download the [Evothings Viewer](https://itunes.apple.com/nz/app/evothings-viewer/id1029452707?mt=8) or [CGTek Viewer](https://itunes.apple.com/us/app/cgtek-viewer/id1135118284?mt=8) from the Apple Store (note: the iOS version of the Evothings Viewer does not seem to be available in the US at the time of writing).

#### Usage
+ Upon startup, the application will attempt to connect to the Turnstyle unit.  It will indicate when it has connected or been disconnected in the status bar.  Tapping the status bar will prompt the application to reconnect.
+ The population updates in real time with the number displayed on the LCD of the Turnstyle unit.  The application will also display whether the door is currently open or closed, and update this information in real time.
+ If the population increases beyond the maximum occupancy displayed underneath the population, an alert will be displayed to indiate a possible fire hazard.  In the future, we would like to consider configuring this feature to alert the fire department if the population gets too high.
+ Tapping the Analytics button will open up the ```plotly``` Dashboard described in the previous section.
+ Tapping the Settings button opens up several options for the user to adjust.
    + The Set Capacity field allows the user to adjust the maximum occupancy of the room
    + The Set Population field allows the user to manually adjust the population (this is for demo purposes and also allows the unit to be installed in a room with people already in it)
    + The Alert Me checkbox allows the user to turn on security features after business hours.
+ An alert is displayed if the security features are enabled and the door is opened.

#### Update 8/3/2017: We've now included an ```.apk``` file built with Evothings Studio for easier testing on Android devices.  It can be found in the root directory.  Unfortunately, iOS users will stiil have to follow the steps described above.