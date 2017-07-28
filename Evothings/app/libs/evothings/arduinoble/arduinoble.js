// File: arduinoble.js

// Load library EasyBLE.
evothings.loadScript('libs/evothings/easyble/easyble.js');

/**
 * @namespace
 * @author Mikael Kindborg
 * @description <p>Functions for communicating with an Arduino BLE shield.</p>
 * <p>It is safe practise to call function {@link evothings.scriptsLoaded}
 * to ensure dependent libraries are loaded before calling functions
 * in this library.</p>
 *
 * @todo This is a very simple library that has only write capability,
 * read and notification functions should be added.
 *
 * @todo Add function to set the write characteristic UUID to make
 * the code more generic.
 */

evothings.arduinoble = {};

;(function()
{
	// Internal functions.
	var internal = {};

	// UUIDs
	evothings.arduinoble.SERVICE_UUID = '468db76d-4b92-48a4-8727-426f9a4a2482';
	evothings.arduinoble.POPULATION_UUID = 'd75b671b-6ea4-464e-89fd-1ab8ad76440b';
	evothings.arduinoble.OPEN_UUID = '8404e92d-0ca7-480b-8b3f-7a1e4c8406f1';
	evothings.arduinoble.ORIENTATION_UUID = '4ac1aade-3086-4ca1-92e1-0de3a0076674';
	evothings.arduinoble.POPULATION_DESCRIPTOR = '0a61d834-ac49-48f5-b85e-414f6481fb72';
	evothings.arduinoble.OPEN_DESCRIPTOR = '04f8476f-6769-4bf2-af37-c9524145f4e3';

	/**
	 * Stop any ongoing scan and disconnect all devices.
	 * @public
	 */
	evothings.arduinoble.close = function()
	{
		evothings.easyble.stopScan();
		evothings.easyble.closeConnectedDevices();
	};

	/**
	 * Called when you've connected to an Arduino BLE shield.
	 * @callback evothings.arduinoble.connectsuccess
	 * @param {evothings.arduinoble.ArduinoBLEDevice} device -
	 * The connected BLE shield.
	 */

	/**
	 * Connect to a BLE-shield.
	 * @param deviceName BLE name if the shield.
	 * @param {evothings.arduinoble.connectsuccess} success -
	 * Success callback: success(device)
	 * @param {function} fail - Error callback: fail(errorCode)
	 * @example
	 * evothings.arduinoble.connect(
	 *   'arduinoble', // Name of BLE shield.
	 *   function(device)
	 *   {
	 *     console.log('connected!');
	 *     device.writeDataArray(new Uint8Array([1]));
	 *     evothings.arduinoble.close();
	 *   },
	 *   function(errorCode)
	 *   {
	 *     console.log('Error: ' + errorCode);
	 *   });
	 * @public
	 */
	evothings.arduinoble.connect = function(deviceName, success, fail)
	{
		evothings.easyble.reportDeviceOnce(true);
		evothings.easyble.startScan(
			function(device)
			{
				if (device.hasName(deviceName))
				{
					evothings.easyble.stopScan();
					internal.connectToDevice(device, success, fail);
				}
			},
			function(errorCode)
			{
				fail(errorCode);
			});
	};

	/**
	 * Connect to the BLE shield.
	 * @private
	 */
	internal.connectToDevice = function(device, success, fail)
	{
		device.connect(
			function(device)
			{
				// Get services info.
				internal.getServices(device, success, fail);
				// intial reading
			},
			function(errorCode)
			{
				fail(errorCode);
			});
	};

	/**
	 * Read all services from the device.
	 * @private
	 */
	internal.getServices = function(device, success, fail)
	{
		device.readServices(
			null, // null means read info for all services
			function(device)
			{
				internal.addMethodsToDeviceObject(device);
				internal.startCharacteristicNotification(device);
				device.initialRead();
				success(device);
			},
			function(errorCode)
			{
				fail(errorCode);
			});
	};

	/**
	 * Add instance methods to the device object.
	 * @private
	 */
	internal.addMethodsToDeviceObject = function(device)
	{
		/**
		 * Object that holds info about an Arduino BLE shield.
		 * @namespace evothings.arduinoble.ArduinoBLEDevice
		 */

		/**
		 * @function writeDataArray
		 * @description Write data to an Arduino BLE shield.
		 * @param uint8array - The data to be written.
		 * @memberof evothings.arduinoble.ArduinoBLEDevice
		 * @instance
		 * @public
		 */
		device.writeDataArray = function(uint8array, uuid)
		{
			uuid = uuid || '713d0003-503e-4c75-ba94-3148f18d941e';
			device.writeCharacteristic(
				uuid,
				uint8array,
				function()
				{
					console.log('writeCharacteristic success');
				},
				function(errorCode)
				{
					console.log('writeCharacteristic error: ' + errorCode);
				});
		};

		device.initialRead = function()
		{
			// bad repeated code because these are undefined outside for some reason.
			var POPULATION_UUID = 'd75b671b-6ea4-464e-89fd-1ab8ad76440b';
			var OPEN_UUID = '8404e92d-0ca7-480b-8b3f-7a1e4c8406f1';
			device.readCharacteristic(
						POPULATION_UUID,
						function(data)
						{
							device.updatePopulation(data);
						},
						function(errorCode)
						{
							console.log('BLE readCharacteristic error: ' + errorCode);
						});
			device.readCharacteristic(
						OPEN_UUID,
						function(data)
						{
							device.updateOpen(data);
						},
						function(errorCode)
						{
							console.log('BLE readCharacteristic error: ' + errorCode);
						});
			device.cap = 100;
			device.alert = false;
		};

		device.checkPopulation = function()
		{
			var overflow = (parseInt(device.pop) > parseInt(device.cap));
			if (overflow) {
				alert("WARNING: MAXIMUM OCCUPANCY EXCEEDED!");
			}
			document.getElementById('population').style.color = overflow ? 'Red' : 'Black';
		};

		device.updatePopulation = function(data)
		{
			var population = new DataView(data).getUint16(0, true);
			device.pop = population;
			document.getElementById('population').innerHTML = population;
			document.getElementById('popselect').value = population;
			device.checkPopulation();
		};

		device.updateOpen = function(data)
		{
			var open = new DataView(data).getUint8(0, true);
			document.getElementById('openOrClosed').innerHTML = open ? 'OPEN' : 'CLOSED';
			if (device.alert && open) {
				alert("WARNING: POSSIBLE SECURITY BREACH!");
			}
		};
	};

	/**
	 * Read characteristic data.  - added by David
	 */
	internal.startCharacteristicNotification = function(device)
	{

		// Set characteristic notifications to ON.
		device.writeDescriptor(
			evothings.arduinoble.POPULATION_UUID,
			evothings.arduinoble.POPULATION_DESCRIPTOR, // Notification descriptor.
			new Uint8Array([1,0]),
			function()
			{
				console.log('Status: writeDescriptor ok.');
			},
			function(errorCode)
			{
				// This error will happen on iOS, since this descriptor is not
				// listed when requesting descriptors. On iOS you are not allowed
				// to use the configuration descriptor explicitly. It should be
				// safe to ignore this error.
				console.log('Error: writeDescriptor: ' + errorCode + '.');
			});

		device.writeDescriptor(
			evothings.arduinoble.OPEN_UUID,
			evothings.arduinoble.OPEN_DESCRIPTOR, // Notification descriptor.
			new Uint8Array([1,0]),
			function()
			{
				console.log('Status: writeDescriptor ok.');
			},
			function(errorCode)
			{
				// This error will happen on iOS, since this descriptor is not
				// listed when requesting descriptors. On iOS you are not allowed
				// to use the configuration descriptor explicitly. It should be
				// safe to ignore this error.
				console.log('Error: writeDescriptor: ' + errorCode + '.');
			});
		
		
		// Start population notifications.
		device.enableNotification(
			evothings.arduinoble.POPULATION_UUID,
			function(data)
			{
				device.updatePopulation(data);
			},
			function(errorCode)
			{
				console.log('Error: enableNotification: ' + errorCode + '.');
			});
		
		
		// Start open/close notifications.
		device.enableNotification(
			evothings.arduinoble.OPEN_UUID,
			function(data)
			{
				device.updateOpen(data);
			},
			function(errorCode)
			{
				console.log('Error: enableNotification: ' + errorCode + '.');
			});
	};

})();
