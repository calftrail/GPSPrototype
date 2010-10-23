/*
 *  TLSerialBasics.h
 *  GPSPrototype
 *
 *  Created by Nathan Vander Wilt on 4/9/08.
 *  Copyright 2008 __MyCompanyName__. All rights reserved.
 *
 */

#include <IOKit/IOKitLib.h>


/*! @function TLFindSerialPorts
    @abstract Finds currently active serial ports.
    @param matchingServices The created iterator over the active serial ports, on success. The iterator must be released when the iteration is finished. See IOObjectRelease.
    @result A kern_return_t error code. */
IOReturn TLFindSerialPorts(io_iterator_t* matchingServices);

/*! @function TLFindNewSerialPortsContinually
    @abstract Finds active serial ports as they are added.
	@discussion This function is equivalent to calling TLRegisterSerialNotification with a notificationType of kIOMatchedNotification
    @param notifyPort A IONotificationPortRef object that controls how messages will be sent when the armed notification is fired. When the notification is delivered, the io_iterator_t representing the notification should be iterated through to pick up all outstanding objects. When the iteration is finished the notification is rearmed. See IONotificationPortCreate.
    @param callback The callback function called when the notification fires.
    @param userData For the callback's use.
    @param matchingServices An iterator handle is returned on success, and should be released by the caller when the notification is to be destroyed. The notification is armed when the iterator is emptied by calls to IOIteratorNext - when no more objects are returned, the notification is armed. Note the notification is not armed when first created.
    @result A kern_return_t error code. */
IOReturn TLFindNewSerialPortsContinually(IONotificationPortRef notifyPort,
										   IOServiceMatchingCallback callback,
										   void* userData,
										   io_iterator_t* matchingServices);

/*! @function TLRegisterSerialNotification
    @abstract Enables the client to register for any notification type.
    @discussion This wraps the IOServiceAddMatchingNotification function to provide the match dictionary automatically.
    @param notifyPort A IONotificationPortRef object that controls how messages will be sent when the armed notification is fired. When the notification is delivered, the io_iterator_t representing the notification should be iterated through to pick up all outstanding objects. When the iteration is finished the notification is rearmed. See IONotificationPortCreate.
    @param callback The callback function called when the notification fires.
    @param userData For the callback's use.
    @param matchingServices An iterator handle is returned on success, and should be released by the caller when the notification is to be destroyed. The notification is armed when the iterator is emptied by calls to IOIteratorNext - when no more objects are returned, the notification is armed. Note the notification is not armed when first created.
    @result A kern_return_t error code. */
IOReturn TLRegisterSerialNotification(const io_name_t notificationType,
										   IONotificationPortRef notifyPort,
										   IOServiceMatchingCallback callback, void* userData,
										   io_iterator_t* iterator);

/*! @function TLCreateStringForSerialPortPath
    @abstract Gets a filesystem path to a serial service.
    @param serialService The serial service for which to get a path.
    @param deviceFilePath A pointer to a CFStringRef which will be set to a string containing the device-file path in /dev. The user must release this string when it is no longer needed.
    @result A kern_return_t error code. */
IOReturn TLCreateStringForSerialPortPath(io_object_t serialService, CFStringRef* deviceFilePath);

/*! @function TLGetSerialPortPath
    @abstract Gets a POSIX path to a serial service, suitable for use with fopen().
    @discussion This just wraps the TLCreateStringForSerialPortPath function as a convenience.
    @param serialService The serial service for which to get a path.
    @param deviceFilePath An existing buffer where the path will be placed.
    @param maxPathSize Size of the existing buffer deviceFilePath. If the buffer is too small to hold the path, an error will be returned.
    @result A kern_return_t error code. */
IOReturn TLGetSerialPortPath(io_object_t serialService, char* deviceFilePath, CFIndex maxPathSize);

/*! @function TLCreateStringForSerialPortName
    @abstract Makes a best effort to find a human-readable name for the serial service.
    @discussion The created string will typically be in English, though often it is just the product name. Some example strings might be "Keyspan USA-19H" or "USB-Serial Controller", although the function may fall back onto a name like "usbserial" if nothing better is found.
    @param serialService The serial service for which to get the name.
    @param deviceName A pointer to a CFStringRef which will be set to a string containing the human readable name. The user must release this string when it is no longer needed.
    @param failureDiagnostic A pointer to a CFStringRef that will gather potential human readable field names if necessary. This should be NULL if this information is not desired. If the given pointer is not NULL, the user is responsible for checking the reference and releasing it if it has been set. The string reference itself will be set to a created CFString containing the information, or NULL if no diagnostic was needed.
    @result A kern_return_t error code. */
IOReturn TLCreateStringForSerialPortName(io_object_t serialService, CFStringRef* deviceName, CFStringRef* failureDiagnostic);
