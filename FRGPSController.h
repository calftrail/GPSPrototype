//
//  FRGPSController.h
//  GPSPrototype
//
//  Created by Nathan Vander Wilt on 4/9/08.
//  Copyright 2008 __MyCompanyName__. All rights reserved.
//

#import <Cocoa/Cocoa.h>

@interface FRGPSController : NSObject {
	IBOutlet NSPopUpButton* serialPortPicker;
	IBOutlet NSButton* startButton;
	IBOutlet NSTextField* statusLabel;
	IBOutlet NSProgressIndicator* progressBar;
@private
	NSMutableArray* activeSerialPorts;
	IONotificationPortRef addNotificationPort;
	IONotificationPortRef removeNotificationPort;
	io_iterator_t addedSerialPortIterator;
	io_iterator_t removedSerialPortIterator;
}

- (IBAction)startTransfer:(id)sender;
- (IBAction)changePort:(id)sender;

@end
