=================
Framework Changes
=================

.. contents:: Table of Contents
   :local:

.. warning:: **Developers:** Sort changes under appropriate heading
    putting new features at the top of the section, followed by
    improvements, followed by bug fixes.

Logging
-------

- We have changed the logging in Mantid to stop writing the a high level version of the log to a file.  This had been causing numerous problems including inconsistent behaviour with multiple instances of Mantid, performance problems when logging at detailed levels, and excessive network usage in some scenarios.  This does not change the rest of the logging that you see in the message display in Mantidplot or the console window.  

  - Associated with this we have also simplified the python methods used to control logging.
  
    .. code-block:: python

	  	# The two methods
	  	ConfigService.SetConsoleLogLevel(int)
	  	ConfigService.SetFileLogLevel(int)

	  	# Have been replaced by
	  	ConfigService.SetLogLevel(int)

Stability
---------




Algorithms
----------


New Algorithms
##############



Improvements
############


Bugfixes
########

- :ref:`SaveGDA <algm-SaveGDA>` Now takes a parameter of OutputFilename instead of Filename to better match with similar algorithms.

Python
------

New
###


Improvements
############

- :ref:`ChudleyElliot <func-ChudleyElliot>` includes hbar in the definition

Bugfixes
########


:ref:`Release 3.14.0 <v3.14.0>`


