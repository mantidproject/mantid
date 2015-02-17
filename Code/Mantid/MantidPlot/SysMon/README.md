SysMon
======

Python qt system monitor which utilizes the Python psutil and platform modules to provide system information for display.

This application has been adapted to work with psutil version 1 and version 2 modules as there are some command syntax changes between these two versions.

The SysMon user interface has been divided into a main window which imports the tabs to make a standalone application, or the tabs can be imported into other applications as a QWidget.  Thus there are separate .ui files corresponding to each.  

The code which imports the tabs into the main program resides in SysMon.pyw.  This is where to look to see how to include the tabs into your own application.  All files except SysMon.pyw and ui_sysmonMainWindow.* will be required when tabs are incorporated in other applications.

The following command line arguments have been added:  
  --help to print out the help message.
  --nompl to run the application minus matplotlib in support of the current MantidPlot (removes those tabs requiring matplotlib).
  --custom to enable the custom menubar item in the standalone application (currently supports checking Matlab license status on SNS analysis computers).

To run as a standalone application via the corresponding command lines:
  *Change to the folder containing the Sysmon software, then:
  *DOS: python SysMon.pyw
  *Linux: ./SysMon.pyw
  
The standalone application been tested on Windows and RHEL Linux, but not on Mac yet.

Note that configuration and global constants and variables now reside in config.py.

