/*
 *  TLSerialBasics.c
 *  GPSPrototype
 *
 *  Created by Nathan Vander Wilt on 4/9/08.
 *  Copyright 2008 __MyCompanyName__. All rights reserved.
 *
 */

#include "TLSerialBasics.h"

#include <stdio.h>
#include <IOKit/serial/IOSerialKeys.h>


static CFMutableDictionaryRef TLSerialPortMatchDictionary() {
	// Serial devices are instances of class IOSerialBSDClient.
	CFMutableDictionaryRef classesToMatch = IOServiceMatching(kIOSerialBSDServiceValue);
	if (classesToMatch) {
		/*
		 Each serial device object has a property with key
		 kIOSerialBSDTypeKey and a value that is one of
		 kIOSerialBSDAllTypes, kIOSerialBSDModemType,
		 or kIOSerialBSDRS232Type.
		 */
		CFDictionarySetValue(classesToMatch, CFSTR(kIOSerialBSDTypeKey), CFSTR(kIOSerialBSDRS232Type));
	}
	
	if (!classesToMatch) {
		printf("IOServiceMatching returned a NULL dictionary.\n");
	}
	
	return classesToMatch;
}

IOReturn TLFindSerialPorts(io_iterator_t* matchingServices) {
	IOReturn kernResult;
	
	mach_port_t masterPort;
	kernResult = IOMasterPort(MACH_PORT_NULL, &masterPort);
	if (kernResult != kIOReturnSuccess)	{
		printf("IOMasterPort returned %d\n", kernResult);
		goto exit;
	}
	
	CFMutableDictionaryRef classesToMatch = TLSerialPortMatchDictionary();
	kernResult = IOServiceGetMatchingServices(masterPort, classesToMatch, matchingServices);
	if (kernResult != kIOReturnSuccess) {
		printf("IOServiceGetMatchingServices returned %d\n", kernResult);
		goto exit;
	}
	
exit:
	return kernResult;
}

IOReturn TLRegisterSerialNotification(const io_name_t notificationType,
												  IONotificationPortRef notifyPort,
												  IOServiceMatchingCallback callback, void* userData,
												  io_iterator_t* iterator)
{
	// see http://developer.apple.com/documentation/DeviceDrivers/Conceptual/AccessingHardware/AH_IOKitLib_API/chapter_5_section_3.html
	// and http://developer.apple.com/documentation/DeviceDrivers/Conceptual/AccessingHardware/AH_Finding_Devices/chapter_4_section_2.html
	
	CFMutableDictionaryRef classesToMatch = TLSerialPortMatchDictionary();
	IOReturn result;
	mach_port_t masterPort;
	result = IOMasterPort(MACH_PORT_NULL, &masterPort);
	if (result != kIOReturnSuccess) return result;
	
	result = IOServiceAddMatchingNotification(notifyPort,
											  notificationType,
											  classesToMatch,
											  callback,
											  userData,
											  iterator);
	return result;
}

IOReturn TLFindNewSerialPortsContinually(IONotificationPortRef notifyPort,
				   IOServiceMatchingCallback callback, void* userData,
				   io_iterator_t* iterator)
{
	return TLRegisterSerialNotification(kIOMatchedNotification, notifyPort, callback, userData, iterator);
}

IOReturn TLCreateStringForSerialPortPath(io_object_t serialService, CFStringRef* deviceFilePath) {
	*deviceFilePath = NULL;
	/*
	 Get the callout device's path (/dev/cu.xxxxx).
	 The callout device should almost always be used. You would use the dialin device (/dev/tty.xxxxx)
	 when monitoring a serial port for incoming calls, for example, a fax listener.
	 */
	*deviceFilePath = IORegistryEntryCreateCFProperty(serialService,
													  CFSTR(kIOCalloutDeviceKey),
													  kCFAllocatorDefault, 0);
	return *deviceFilePath ? kIOReturnSuccess : kIOReturnError;
}

IOReturn TLGetSerialPortPath(io_object_t serialService, char* deviceFilePath, CFIndex maxPathSize) {
	*deviceFilePath = '\0';
	CFStringRef deviceFilePathAsCFString;
	(void)TLCreateStringForSerialPortPath(serialService, &deviceFilePathAsCFString);
	if (!deviceFilePathAsCFString) return kIOReturnError;
	// Convert the path from a CFString to a NULL-terminated C string for use with the POSIX open() call.
	Boolean result = CFStringGetCString(deviceFilePathAsCFString, deviceFilePath, maxPathSize, kCFStringEncodingASCII);
	CFRelease(deviceFilePathAsCFString);
	return result ? kIOReturnSuccess : kIOReturnError;
}

IOReturn TLCreateStringForSerialPortName(io_object_t serialService, CFStringRef* deviceName, CFStringRef* failureDiagnostic) {
	*deviceName = NULL;
	if (failureDiagnostic) *failureDiagnostic = NULL;
	/*
	 This tries a number of different key names in order, so that ideally the best one will be found first.
	 The more human-readable key names are usually found in parent entries, thus the recursive parent search.
	 */
	
	CFTypeRef deviceNameAsCFString;
	deviceNameAsCFString = IORegistryEntrySearchCFProperty(serialService,
														   kIOServicePlane,
														   CFSTR("USB Product Name"),		// for "Keyspan USA-19H" adapter and hopefully other Keyspan's
														   kCFAllocatorDefault,
														   kIORegistryIterateParents | kIORegistryIterateRecursively);
	if (deviceNameAsCFString) goto found;
	
	deviceNameAsCFString = IORegistryEntrySearchCFProperty(serialService,
														   kIOServicePlane,
														   CFSTR("Product Name"),	// for "USB-Serial Controller" on PL-2302
														   kCFAllocatorDefault,
														   kIORegistryIterateParents | kIORegistryIterateRecursively);
	if (deviceNameAsCFString) goto found;
	
	if (failureDiagnostic) {
		/*
		 TODO: If none of the above keys are found () this should wrap up a bunch
		 of sample data for optional submission from user. This could help us return
		 better values in future releases.
		 
		 This should stop when a device of suitably generic IOProviderClass (like
		 IOResources or IOPCIDevice) is encountered.
		 */
		// TODO: perhaps also check IOTTYBaseName, unless that key is guaranteed
	}
	
	deviceNameAsCFString = IORegistryEntrySearchCFProperty(serialService,
														   kIOServicePlane,
														   CFSTR("IOTTYBaseName"),	// seems the best fallback ( eg "Bluetooth-PDA-Sync")
														   kCFAllocatorDefault,
														   kIORegistryIterateParents | kIORegistryIterateRecursively);
	if (deviceNameAsCFString) goto found;
	
	return kIOReturnError;
	
found:
	*deviceName = deviceNameAsCFString;
	return kIOReturnSuccess;
}



