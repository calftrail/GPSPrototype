//
//  FRGPSController.m
//  GPSPrototype
//
//  Created by Nathan Vander Wilt on 4/9/08.
//  Copyright 2008 __MyCompanyName__. All rights reserved.
//

#import "FRGPSController.h"

#include "TLSerialBasics.h"
#include "FRGPSTesting.h"

@implementation FRGPSController

static void serialPortAddCallback(void* userData, io_iterator_t iterator);
static void serialPortRemoveCallback(void* userData, io_iterator_t iterator);

#pragma mark Lifecycle

- (id)init {
	self = [super init];
	if (self) {
		activeSerialPorts = [NSMutableArray new];
		CFRunLoopSourceRef notifyRunLoopSource;
		// initialize notification source for added devices
		addNotificationPort = IONotificationPortCreate(kIOMasterPortDefault);
		notifyRunLoopSource = IONotificationPortGetRunLoopSource(addNotificationPort);
		CFRunLoopAddSource(CFRunLoopGetCurrent(), notifyRunLoopSource, kCFRunLoopDefaultMode);
		// initialize notification source for removed devices
		removeNotificationPort = IONotificationPortCreate(kIOMasterPortDefault);
		notifyRunLoopSource = IONotificationPortGetRunLoopSource(removeNotificationPort);
		CFRunLoopAddSource(CFRunLoopGetCurrent(), notifyRunLoopSource, kCFRunLoopDefaultMode);		
	}
	return self;
}

- (void)awakeFromNib {
	IOReturn result;
	result = TLFindNewSerialPortsContinually(addNotificationPort,
											 serialPortAddCallback,
											 self,
											 &addedSerialPortIterator);
	if (result == kIOReturnSuccess) serialPortAddCallback(self, addedSerialPortIterator);
	
	result = TLRegisterSerialNotification(kIOTerminatedNotification,
										  removeNotificationPort,
										  serialPortRemoveCallback,
										  self,
										  &removedSerialPortIterator);
	// Even though removals are unlikely, we still need to consume the iterator to get future notifications.
	if (result == kIOReturnSuccess) serialPortRemoveCallback(self, removedSerialPortIterator);
}

- (void)dealloc {
	[activeSerialPorts release];
	(void)IOObjectRelease(addedSerialPortIterator);
	IONotificationPortDestroy(addNotificationPort);
	(void)IOObjectRelease(removedSerialPortIterator);
	IONotificationPortDestroy(removeNotificationPort);
	[super dealloc];
}

#pragma mark User interface handling

- (NSString*)devicePathAtIndex:(unsigned)index {
	return [[activeSerialPorts objectAtIndex:index] valueForKey:@"path"];
}

- (NSString*)deviceNameAtIndex:(unsigned)index {
	return [[activeSerialPorts objectAtIndex:index] valueForKey:@"name"];
}

- (unsigned)indexForDevicePath:(NSString*)devicePath {
	unsigned portIdx;
	for (portIdx = 0; portIdx < [activeSerialPorts count]; ++portIdx) {
		if ([[self devicePathAtIndex:portIdx] isEqualToString:devicePath]) return portIdx;
	}
	return NSNotFound;
}

- (IBAction)startTransfer:(id)sender {
	(void)sender;
	int portIdx = [serialPortPicker indexOfSelectedItem];
	if (portIdx < 0) return;
	NSString* pickedPortPath = [self devicePathAtIndex:portIdx];
	const char* temporaryPortPath = [pickedPortPath UTF8String];
	testUsingDeviceFile(temporaryPortPath);
}

- (IBAction)changePort:(id)sender {
	(void)sender;
	//printf("port potentially changed\n");
}

- (IBAction)updateDropdown:(id)sender {
	(void)sender;
	[serialPortPicker removeAllItems];
	unsigned portIdx;
	for (portIdx = 0; portIdx < [activeSerialPorts count]; ++portIdx) {
		NSString* deviceName = [self deviceNameAtIndex:portIdx];
		[serialPortPicker addItemWithTitle:deviceName];
	}
	[self changePort:self];
}

#pragma mark I/O Kit handling

- (void)addSerialDevices:(io_iterator_t)serialPortIterator {
	io_object_t serialService;
	IOIteratorReset(serialPortIterator);
	while ( (serialService = IOIteratorNext(serialPortIterator)) ) {
		IOReturn result;
		CFStringRef devicePath;
		result = TLCreateStringForSerialPortPath(serialService, &devicePath);
		NSAssert1(result == kIOReturnSuccess,
				  @"Failed to get device-file path for added serial port (Error: %i)", result);
		CFStringRef deviceName;
		result = TLCreateStringForSerialPortName(serialService, &deviceName, NULL);
		if (result != kIOReturnSuccess) {
			deviceName = (CFStringRef)[[NSString alloc] initWithString:@"<Serial Port>"];
		}
		// Release the io_service_t now that we are done with it, ignoring any error.
		(void)IOObjectRelease(serialService);
		
		NSDictionary* portInfo = [NSDictionary dictionaryWithObjectsAndKeys:
								  [(id)deviceName autorelease], @"name",
								  [(id)devicePath autorelease], @"path", nil];
		[activeSerialPorts addObject:portInfo];
		
		printf("Adding '%s' at path: %s\n", [(NSString*)deviceName UTF8String], [(NSString*)devicePath UTF8String]);
	}
	[self updateDropdown:self];
}

static void serialPortAddCallback(void* userData, io_iterator_t iterator) {
	[(id)userData addSerialDevices:iterator];
}

- (void)removeSerialDevices:(io_iterator_t)serialPortIterator {
	io_object_t serialService;
	IOIteratorReset(serialPortIterator);
	while ( (serialService = IOIteratorNext(serialPortIterator)) ) {
		CFStringRef devicePath;
		IOReturn result = TLCreateStringForSerialPortPath(serialService, &devicePath);
		NSAssert1(result == kIOReturnSuccess,
				  @"Failed to get device-file path for removed serial port. (Error: %i)", result);
		// Release the io_service_t now that we are done with it, ignoring any error.
		(void)IOObjectRelease(serialService);
		
		printf("Removing device at path: %s\n", [(NSString*)devicePath UTF8String]);
		
		unsigned portIdx = [self indexForDevicePath:(id)devicePath]; 
		[activeSerialPorts removeObjectAtIndex:portIdx];
		CFRelease(devicePath);
	}
	[self updateDropdown:self];
}

static void serialPortRemoveCallback(void* userData, io_iterator_t iterator) {
	[(id)userData removeSerialDevices:iterator];
}

@end
