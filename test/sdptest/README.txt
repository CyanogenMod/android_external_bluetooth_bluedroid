Bluedroid Test Application
==========================
The test application provides a small console shell interface that allows
access to the Bluetooth HAL API library though ASCII commands. This is similar
to how the real JNI service would operate. The primary objective of this
application is to allow Bluetooth to be put in DUT Mode for RF/BB BQB test purposes.

This application is mutually exclusive with the Java based Bluetooth.apk. Hence
before launching the application, it should be ensured that the Settings->Bluetooth is OFF.

This application is built as 'sdptool' and shall be available in '/system/bin/sdptool'

Limitations
===========
1.) Settings->Bluetooth must be OFF for this application to work
2.) Currently, only the SIG 'HCI Test Mode' commands are supported. The vendor
specific HCI test mode commands to be added.

Usage instructions
==================
The following section describes the various commands and their usage

Launching the test application
==============================
$ adb root
$ adb shell
root@android:/ # /system/bin/sdptool

 set_aid_and_cap : pid 4374, uid 0 gid 0
 Loading HAL lib + extensions
 HAL library loaded (Success)
>INIT BT HAL REQUEST SUCCESS

Enabling Bluetooth
==================
>enablebt
 ENABLE BT
 adapter_properties_changed  status= 0, num_properties=1
 adapter_properties_changed  status= 0, num_properties=6
 Local Bd Addr = 0:0:0:f6:48:46
 Setinng it to scan
 adapter_properties_changed  status= 0, num_properties=1
>adapter_properties_changed  status= 0, num_properties=1

Initializing SDP interface
========================
>init
 Initializing SDP interface
 Get SDP interface
 btif_sdp_get_interface
>Registering SDP Calbacks

SDP the remote device
================================
>searchservices fc:f8:ae:d7:c2:e6
 The parameter passed is : fc:f8:ae:d7:c2:e6
 Search Services started for fc:f8:ae:d7:c2:e6
 Wait for the SDP to complete
 sdp started succeessfully
 sdp completed .Result=0

 Remote Device Service(s) UUID(s) = 0x1105
 Remote Device Service(s) UUID(s) = 0x110a
 Remote Device Service(s) UUID(s) = 0x110c
 Remote Device Service(s) UUID(s) = 0x1115
 Remote Device Service(s) UUID(s) = 0x112f
 Remote Device Service(s) UUID(s) = 0x1132
 Remote Device Service(s) UUID(s) = 0x1112
 Remote Device Service(s) UUID(s) = 0x111f
 Remote Device Service(s) UUID(s) = 0x1203
 Remote Device Service(s) UUID(s) = 0x1132
>Remote Device Service(s) UUID(s) = 0x1800

Close every thing gracefully, otherwise adaptor/stack goes into undefined state and starting sdptool again may not work as expected.
=================================
>quit
