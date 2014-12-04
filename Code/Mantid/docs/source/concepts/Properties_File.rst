.. _Properties File:

Properties File
===============

The two .Properties Files
-------------------------

The Mantid framework is configured using two simple text .properties files that are read an interpreted every time the framework is started. These properties are not the same as the properties of algorithms.

Mantid.Properties
*****************

This file defines the default configuration that the development team suggest as sensible defaults. This file should not be altered by users as it will be replaced with every new install or upgrade of Mantid.

Mantid.User.Properties
**********************

This is where users may override any property setting in Mantid. Any Property setting in this file will override anything set in the Mantid.Properties file. Simply either enter the property you wish to override in this file together with it's new value. The change will take effect the next time Mantid is started. Subsequent installs or upgrades of Mantid will never alter this file.

The Properties
--------------

Note: Use forward slash (/) or double up on the number of backslash (\) characters for all paths

General properties
******************

+------------------------------+---------------------------------------------------+-------------+
|Property                      |Description                                        |Example value|
+==============================+===================================================+=============+
|algorithms.retained           |The Number of algorithms properties to retain in   | 50          |
|                              |memory for refence in scripts.                     |             |
+------------------------------+---------------------------------------------------+-------------+
|algorithms.categories.hidden  |A comma separated list of any categories of        | Mouns, Test |
|                              |algorithms that should be hidden in Mantid.        | Category    |
+------------------------------+---------------------------------------------------+-------------+
|AlgorithmChaining.SwitchedOn  |Algorithm chain re-running will replay any         | 0 or 1      |
|                              |subsequent algorithms whenever a workspace is      |             |
|                              |replaced.                                          |             |
|                              |                                                   |             |
|                              |This was an early implementation and its use is    |             |
|                              |no longer advised as it could cause race conditions|             |
|                              |when combined with scripts.                        |             |
+------------------------------+---------------------------------------------------+-------------+
|MultiThreaded.MaxCores        |Sets the maximum number of cores available to be   | 0           |
|                              |used for threads for OpenMP. If zero it will use   |             |
|                              |one thread per logical core available.             |             |
+------------------------------+---------------------------------------------------+-------------+

Facility and instrument properties
**********************************

+------------------------------+---------------------------------------------------+-------------+
|Property                      |Description                                        |Example value|
+==============================+===================================================+=============+
|default.facility              |The name of the default facility. The facility must| ISIS        |
|                              |be defined within the facilites.xml file to be     |             |
|                              |considered valid. The file is described here.      |             |
|                              |:ref:`here <Facilities file>`.                     |             |
+------------------------------+---------------------------------------------------+-------------+
|default.instrument            |The name of the default instrument. The instrument | WISH        |
|                              |must be defined within the facilities.xml file to  |             |
|                              |be valid. The file is described                    |             |
|                              |:ref:`here <Facilities file>`.                     |             |
+------------------------------+---------------------------------------------------+-------------+

Directory Properties
********************

+--------------------------------+---------------------------------------------------+-----------------------+
|Property                        |Description                                        |Example value          |
+================================+===================================================+=======================+
|datasearch.directories          |A semi-colon(;) separated list of directories to   |../data;               |
|                                |use to search for data.                            |\\\\isis\\isis$\\ndxgem|
+--------------------------------+---------------------------------------------------+-----------------------+
|defaultsave.directory           |A default directory to use for saving files.       |../data                |
|                                |the data archive                                   |                       |
+--------------------------------+---------------------------------------------------+-----------------------+
|instrumentDefinition.directory  |Where to load instrument definition files from     |../Test/Instrument     |
+--------------------------------+---------------------------------------------------+-----------------------+
|parameterDefinition.directory   |Where to load parameter definition files from      |../Test/Instrument     |
+--------------------------------+---------------------------------------------------+-----------------------+
|pythonscripts.directories       |Python will also search the listed directories when|../scripts;            |
|                                |importing modules.                                 |C:/MyScripts           |
+--------------------------------+---------------------------------------------------+-----------------------+
|pythonscripts.directory         |DEPRECATED: Use pythonscripts.directories          |../scripts             |
|                                |A single location for the Python scripts directory |                       |
+--------------------------------+---------------------------------------------------+-----------------------+
|requiredpythonscript.directories|A list of directories containing Python scripts    |../scripts/SANS;       |
|                                |that Mantid requires to function correctly.        |../scripts/Excitations |
|                                |WARNING: Do not alter the default value.           |                       |
+--------------------------------+---------------------------------------------------+-----------------------+
|plugins.directory               |The path to the directory that contains the Mantid |../Plugins             |
|                                |plugin libraries                                   |                       |
+--------------------------------+---------------------------------------------------+-----------------------+
|requiredpythonscript.directories|A list of directories containing Python scripts    |../scripts/SANS;       |
|                                |that Mantid requires to function correctly.        |../scripts/Excitations |
|                                |WARNING: Do not alter the default value.           |                       |
+--------------------------------+---------------------------------------------------+-----------------------+



Logging Properties
******************

The details of configuring the logging functionality within Mantid will not be explained here. For those who want more details look into the POCO logging classes and the Log4J logging module that it closely emulates. There are several comments in the properties file itself that explain the configuration we provide by default.However there are some obvious areas that you may want to alter and those properties are detailed below.

+----------------------------------------+---------------------------------------------------+-----------------------+
|Property                                |Description                                        |Example value          |
+========================================+===================================================+=======================+
|logging.loggers.root.level              |Defines the lowest level of messages to be output  |debug, informtion,     |
|                                        |by the system. The default is information, but this|notice, warning,       |
|                                        |can be lowered to debug for more detailed feedback.|error, critical        |
|                                        |                                                   |or fatal               |
+----------------------------------------+---------------------------------------------------+-----------------------+
|logging.channels.fileFilterChannel.level|The lowest level messages to output to the log     |debug, informtion,     |
|                                        |file. The default is warning, but this can be      |notice, warning,       |
|                                        |lowered to debug for more detailed feedback. The   |error, critical        |
|                                        |higher level of this and logging.loggers.root.level|or fatal               |
|                                        |will apply.                                        |                       |
+----------------------------------------+---------------------------------------------------+-----------------------+
|logging.channels.fileChannel.path       | The Path to the log file.                         |../logs/mantid.log     |
+----------------------------------------+---------------------------------------------------+-----------------------+

MantidPlot Properties
*********************

+--------------------------------------+---------------------------------------------------+-----------------------+
|Property                              |Description                                        |Example value          |
+======================================+===================================================+=======================+
|MantidOptions.InvisibleWorkspaces     |Do not show 'invisible' workspaces                 |0, 1                   |
+--------------------------------------+---------------------------------------------------+-----------------------+
|MantidOptions.InstrumentView.UseOpenGL|Controls the use of OpenGL in rendering the        |On, Off                |
|                                      |"unwrapped" (flat) instrument views.               |                       |
+--------------------------------------+---------------------------------------------------+-----------------------+

ScriptRepository Properties
***************************

+-----------------------+-----------------------------------------------+----------------------------------------------------------------+
|Property               |Description                                    |Example value                                                   |
+=======================+===============================================+================================================================+
|ScriptLocalRepository  |Directory where ScriptRepository is Installed. |c:\MantidInstall\MyScriptRepository                             |
+-----------------------+-----------------------------------------------+----------------------------------------------------------------+
|ScriptRepository       |Base URL for the remote script repository.     |http://download.mantidproject.org/scriptrepository/             |
+-----------------------+-----------------------------------------------+----------------------------------------------------------------+
|UploaderWebServer      |URL for uploading scripts.                     |http://upload.mantidproject.org/scriptrepository/payload/publish|
+-----------------------+-----------------------------------------------+----------------------------------------------------------------+
|ScriptRepositoryIgnore |CSV patterns for paths that should not be      |\*pyc;                                                          |
|                       |listed at ScriptRepository.                    |                                                                |
+-----------------------+-----------------------------------------------+----------------------------------------------------------------+


Getting access to Mantid properties
***********************************

To get access to, e.g. data saving path property from a C++ program one has to issue the following command:


.. testcode:: properties

  path = ConfigService.getString("defaultsave.directory")

.. categories:: Concepts
