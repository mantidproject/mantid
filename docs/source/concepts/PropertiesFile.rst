.. _Properties File:

Properties File
===============

The ``*.properties`` Files
--------------------------

The Mantid framework is configured using up to three simple text ``*.properties`` files that are read an interpreted every time the framework is started. These properties are not the same as the properties of algorithms. All three have the same format. The three files are read from first to last, with the values in subsequent ``*.properties`` overriding those in previous ones.

1. Install directory ``Mantid.properties`` defines the default configuration that the development team suggest as sensible defaults. This file should not be altered by users as it will be replaced with every new install or upgrade of Mantid.
2. ``/etc/mantid.local.properties`` is an optional, linux only file that sets shared defaults on a shared system. This is commonly used for setting the ``default.facility``, ``default.instrument``, and ``datasearch.searcharchive`` properties.
3. Home directory ``Mantid.user.properties`` is where users may override any property setting in Mantid. Any Property setting in this file will override anything set in the ``Mantid.properties`` file. Simply either enter the property you wish to override in this file together with it's new value. The change will take effect the next time Mantid is started. Subsequent installs or upgrades of Mantid will never alter this file.

The user properties file, ``Mantid.user.properties``, can be found

* windows: ``$MantidInstallDirectory\bin\Mantid.user.properties``
* linux and mac-os: ``$HOME/.mantid/Mantid.user.properties``


The Properties
--------------

.. note:: Use forward slash (``/``) or double up on the number of backslash (``\``) characters for all paths


.. note:: Boolean properties evaluate ``true``, ``1``, and ``on`` (case insensitive) as ``true``, all other values as ``false``.


General properties
******************

+----------------------------------+--------------------------------------------------+------------------------+
|Property                          |Description                                       | Example value          |
+==================================+==================================================+========================+
| ``algorithms.categories.hidden`` | A comma separated list of any categories of      | ``Muons,Testing``      |
|                                  | algorithms that should be hidden in Mantid.      |                        |
+----------------------------------+--------------------------------------------------+------------------------+
| ``algorithms.deprecated``        | Action upon invoking a deprecated algorithm.     | ``Log`` or ``Raise``   |
|                                  | ``Log`` causes a log message at error level.     |                        |
|                                  | ``Raise`` causes a ``RuntimError``.              |                        |
+----------------------------------+--------------------------------------------------+------------------------+
| ``algorithms.alias.deprecated``  | Action upon invoking the algorithm via one of    | ``Log`` or ``Raise``   |
|                                  | its deprecated aliases.                          |                        |
|                                  | ``Log`` causes a log message at error level.     |                        |
|                                  | ``Raise`` causes a ``RuntimError``.              |                        |
+----------------------------------+--------------------------------------------------+------------------------+
| ``curvefitting.guiExclude``      | A semicolon separated list of function names     | ``ExpDecay;Gaussian;`` |
|                                  | that should be hidden in Mantid.                 |                        |
+----------------------------------+--------------------------------------------------+------------------------+
| ``MultiThreaded.MaxCores``       | Sets the maximum number of cores available to be | ``0``                  |
|                                  | used for threads for                             |                        |
|                                  | `OpenMP <http://www.openmp.org/>`_. If zero it   |                        |
|                                  | will use one thread per logical core available.  |                        |
+----------------------------------+--------------------------------------------------+------------------------+

.. _Facility Properties:

Facility and instrument properties
**********************************

+------------------------------+----------------------------------------------------+---------------------+
|Property                      |Description                                         |Example value        |
+==============================+====================================================+=====================+
| ``default.facility``         | The name of the default facility. The facility     | ``ISIS``            |
|                              | must be defined within the facilities.xml file to  |                     |
|                              | be considered valid. The file is described         |                     |
|                              | :ref:`here <Facilities file>`.                     |                     |
+------------------------------+----------------------------------------------------+---------------------+
| ``default.instrument``       | The name of the default instrument. The instrument | ``WISH``            |
|                              | must be defined within the facilities.xml file to  |                     |
|                              | be valid. The file is described                    |                     |
|                              | :ref:`here <Facilities file>`.                     |                     |
+------------------------------+----------------------------------------------------+---------------------+
| ``Q.convention``             | The convention for converting to Q. For            | ``Crystallography`` |
|                              | ``Inelastic`` the convention is ki-kf.  For        | or ``Inelastic``    |
|                              | ``Crystallography`` the convention is kf-ki.       |                     |
+------------------------------+----------------------------------------------------+---------------------+

.. _Directory Properties:

Directory Properties
********************

+--------------------------------------+---------------------------------------------------+-------------------------------------+
|Property                              |Description                                        |Example value                        |
+======================================+===================================================+=====================================+
| ``colormaps.directory``              | The directory where colormaps are located         | ``/opt/mantid/colormaps``           |
+--------------------------------------+---------------------------------------------------+-------------------------------------+
| ``datasearch.directories``           | A semi-colon(``;``) separated list of directories | ``../data;\\\\isis\\isis$\\ndxgem`` |
|                                      | to use to search for data.                        |                                     |
+--------------------------------------+---------------------------------------------------+-------------------------------------+
| ``datacachesearch.directory``        | The directory where data cache is located         | ``/data/instrument``                |
+--------------------------------------+---------------------------------------------------+-------------------------------------+
| ``datasearch.searcharchive``         | ``on`` (only the default facility), ``off``       | ``on`` or ``hfir,sns``              |
|                                      | (none), ``all`` (all archives), or a list of      |                                     |
|                                      | individual facilities to search for files in the  |                                     |
|                                      | data archive                                      |                                     |
+--------------------------------------+---------------------------------------------------+-------------------------------------+
| ``defaultsave.directory``            | A default directory to use for saving files.      | ``../data``                         |
|                                      | the data archive                                  |                                     |
+--------------------------------------+---------------------------------------------------+-------------------------------------+
| ``framework.plugins.directory``      | The path to the directory that contains the       | ``../plugins``                      |
|                                      | Mantid plugin libraries                           |                                     |
+--------------------------------------+---------------------------------------------------+-------------------------------------+
| ``framework.plugins.exclude``        | A list of substrings to allow libraries to be     | ``Qt5``                             |
|                                      | skipped                                           |                                     |
+--------------------------------------+---------------------------------------------------+-------------------------------------+
| ``instrumentDefinition.directory``   | Where to load instrument definition files from    | ``../Test/Instrument``              |
+--------------------------------------+---------------------------------------------------+-------------------------------------+
| ``mantidqt.plugins.directory``       | The path to the directory containing the          | ``../plugins/qtX``                  |
|                                      | Mantid Qt-based plugin libraries                  |                                     |
+--------------------------------------+---------------------------------------------------+-------------------------------------+
| ``parameterDefinition.directory``    | Where to load parameter definition files from     | ``../Test/Instrument``              |
+--------------------------------------+---------------------------------------------------+-------------------------------------+
| ``pythonscripts.directories``        | Python will also search the listed directories    | ``../scripts`` or ``C:/MyScripts``  |
|                                      | when importing modules.                           |                                     |
+--------------------------------------+---------------------------------------------------+-------------------------------------+
| ``pythonscripts.directory``          | **DEPRECATED:** Use ``pythonscripts.directories`` | N/A                                 |
|                                      | instead                                           |                                     |
+--------------------------------------+---------------------------------------------------+-------------------------------------+
| ``requiredpythonscript.directories`` | A list of directories containing Python scripts   | N/A                                 |
|                                      | that Mantid requires to function correctly.       |                                     |
|                                      | **WARNING:** Do not alter the default value.      |                                     |
+--------------------------------------+---------------------------------------------------+-------------------------------------+
| ``requiredpythonscript.directories`` | A list of directories containing Python scripts   | N/A                                 |
|                                      | that Mantid requires to function correctly.       |                                     |
|                                      | **WARNING:** Do not alter the default value.      |                                     |
+--------------------------------------+---------------------------------------------------+-------------------------------------+
| ``python.plugins.manifest``          | A path to the location of the manifest file       | N/A                                 |
|                                      | containing paths to each of the python algorithm  |                                     |
|                                      | files.                                            |                                     |
|                                      | **WARNING:** Do not alter the default value.      |                                     |
+--------------------------------------+---------------------------------------------------+-------------------------------------+
| ``python.templates.directory``       | The directory of python .in files used as         | N/A                                 |
|                                      | templates when generating python scripts from     |                                     |
|                                      | within an algorithm.                              |                                     |
+--------------------------------------+---------------------------------------------------+-------------------------------------+


Logging Properties
******************

The details of configuring the logging functionality within Mantid will not be explained here. For those who want more
details look into the `POCO logging classes <https://pocoproject.org/docs/package-Foundation.Logging.html>`_ and the
`Log4J logging module <https://logging.apache.org/log4j/>`_ that it closely emulates. There are several comments in the
properties file itself that explain the configuration we provide by default.  However there are some obvious areas that
you may want to alter and those properties are detailed below.

+-------------------------------------------------+---------------------------------------------------+-----------------------------+
|Property                                         |Description                                        |Example value                |
+=================================================+===================================================+=============================+
| ``logging.loggers.root.level``                  |Defines the level of messages to be output         | ``debug``, ``information``, |
|                                                 |by the system.                                     | ``notice``, ``warning``,    |
|                                                 |The default is information, but                    | ``error``, ``critical``     |
|                                                 |this can be lowered to debug for more detailed     | or ``fatal``                |
|                                                 |feedback.                                          |                             |
+-------------------------------------------------+---------------------------------------------------+-----------------------------+
| ``logging.channels.consoleChannel.class``       | Select where log messages appear.                 | ``ConsoleChannel``,         |
|                                                 | ``ConsoleChannel`` writes to stdlog.              | ``StdoutChannel``,          |
|                                                 | ``StdoutChannel`` writes to stdout and can be     | ``PythonStdoutChannel``, or |
|                                                 | redirected using pipes.                           | ``PythonLoggingChannel``    |
|                                                 | ``PythonStdoutChannel`` writes to stdout through  |                             |
|                                                 | python and is visible in jupyter notebooks.       |                             |
|                                                 | ``PythonLoggingChannel`` sends messages to a      |                             |
|                                                 | logger called ``'Mantid'`` from the ``logging``   |                             |
|                                                 | framework of Python's standard library.           |                             |
+-------------------------------------------------+---------------------------------------------------+-----------------------------+

The logging priority levels for the file logging and console logging can also be adjusted in python using the command:

.. testcode:: LoggingConfigExample

  #Set the log to debug level or above (7=debug)
  ConfigService.setLogLevel(7)
  #Set the log to critical level (2=critical) and do not log that it was changed
  ConfigService.setLogLevel(2, True)
  # Set the log to information and do not log that it was changed
  ConfigService.setLogLevel("information", True)

More details on logging can be found in the :ref:`developer docs <mantid-dev:Logging>` .


Mantid Graphical User Interface Properties
******************************************

+----------------------------------------------------+----------------------------------------------------+-----------------+
|Property                                            |Description                                         |Example value    |
+====================================================+====================================================+=================+
| ``Notifications.Enabled``                          |Should Mantid use System Notifications for          | ``On``, ``Off`` |
|                                                    |important messages?                                 |                 |
+----------------------------------------------------+----------------------------------------------------+-----------------+
| ``cluster.submission``                             |Enable cluster submission elements in GUIs          | ``On``, ``Off`` |
+----------------------------------------------------+----------------------------------------------------+-----------------+
| ``MantidOptions.InstrumentView.UseOpenGL``         |Controls the use of OpenGL in rendering the         | ``On``, ``Off`` |
|                                                    |"unwrapped" (flat) instrument views.                |                 |
+----------------------------------------------------+----------------------------------------------------+-----------------+
| ``MantidOptions.InstrumentView.MesaBugWorkaround`` |Will reduce the size of the OpenGL display lists    | ``On``, ``Off`` |
|                                                    |used when drawing the Instrument View. By doing     |                 |
|                                                    |this we reduce the chance that we will hit a memory |                 |
|                                                    |allocation bug in the Mesa graphics library. This   |                 |
|                                                    |is only relevant if you using both Linux and a      |                 |
|                                                    |broken version of Mesa.                             |                 |
+----------------------------------------------------+----------------------------------------------------+-----------------+
| ``MantidOptions.InvisibleWorkspaces``              |Do not show 'invisible' workspaces                  | ``0``, ``1``    |
+----------------------------------------------------+----------------------------------------------------+-----------------+
| ``PeakColumn.hklPrec``                             |Precision of hkl values shown in tables             | ``2``           |
+----------------------------------------------------+----------------------------------------------------+-----------------+


Network Properties
******************

+-------------------------------------------+---------------------------------------------------+---------------------------------+
|Property                                   |Description                                        |Example value                    |
+===========================================+===================================================+=================================+
| ``catalog.timeout.value``                 | Network timeout for ICAT4 requests                | ``30``                          |
+-------------------------------------------+---------------------------------------------------+---------------------------------+
| ``CheckMantidVersion.OnStartup``          | Check if there is a newer version available and   |                                 |
|                                           | logs a message at ``information`` level           | ``1``                           |
+-------------------------------------------+---------------------------------------------------+---------------------------------+
| ``ISISDAE.Timeout``                       | Timeout for network requests when reading live    |  ``100``                        |
|                                           | data from ISIS (in seconds)                       |                                 |
+-------------------------------------------+---------------------------------------------------+---------------------------------+
| ``network.default.timeout``               |Defines the default timeout for all network        | ``30``                          |
|                                           |operations (in seconds).                           |                                 |
+-------------------------------------------+---------------------------------------------------+---------------------------------+
| ``network.scriptrepo.timeout``            |The timeout for network operations in the script   | ``5``                           |
|                                           |repository, this overrides the default timeout.    |                                 |
+-------------------------------------------+---------------------------------------------------+---------------------------------+
| ``network.github.api_token``              |The api token for github calls used by             | (not shown)                     |
|                                           |``DownloadInstrument``. Setting this to ``unset``  |                                 |
|                                           |or an empty string will turn off authentication.   |                                 |
+-------------------------------------------+---------------------------------------------------+---------------------------------+
| ``proxy.host``                            | Allows the system proxy to be overridden, if not  | ``http://www.proxy.org``        |
|                                           | set mantid will use the system proxy              |                                 |
+-------------------------------------------+---------------------------------------------------+---------------------------------+
| ``proxy.port``                            | Must be set if proxy.host is set                  | ``8080``                        |
+-------------------------------------------+---------------------------------------------------+---------------------------------+
| ``proxy.httpsTargetUrl``                  | A sample url used to determine the system proxy to| ``http://www.google.com``       |
|                                           | use on windows.                                   |                                 |
+-------------------------------------------+---------------------------------------------------+---------------------------------+
| ``UpdateInstrumentDefinitions.OnStartup`` | Download new instrument definition files and      |                                 |
|                                           | ``Facilities.xml`` to ``~/.mantid/instruments``   |                                 |
|                                           | on linux or ``APPDATA`` directory on windows. If  |                                 |
|                                           | this is disabled, previously downloaded           |                                 |
|                                           | instruments are ignored and only those in the     |                                 |
|                                           | installation are used.                            | ``1``                           |
+-------------------------------------------+---------------------------------------------------+---------------------------------+
| ``usagereports.enabled``                  | Enable usage reporting                            | ``1``                           |
+-------------------------------------------+---------------------------------------------------+---------------------------------+


ScriptRepository Properties
***************************

+----------------------------+-----------------------------------------------+----------------------------------------------------------------------+
|Property                    |Description                                    |Example value                                                         |
+============================+===============================================+======================================================================+
| ``ScriptLocalRepository``  |Directory where ScriptRepository is Installed. | ``C:\\MantidInstall\\MyScriptRepository``                            |
+----------------------------+-----------------------------------------------+----------------------------------------------------------------------+
| ``ScriptRepository``       |Base URL for the remote script repository.     | ``https://download.mantidproject.org/scriptrepository/``             |
+----------------------------+-----------------------------------------------+----------------------------------------------------------------------+
| ``ScriptRepositoryIgnore`` |CSV patterns for paths that should not be      | ``*pyc;``                                                            |
|                            |listed at ScriptRepository.                    |                                                                      |
+----------------------------+-----------------------------------------------+----------------------------------------------------------------------+
| ``UploaderWebServer``      |URL for uploading scripts.                     | ``https://upload.mantidproject.org/scriptrepository/payload/publish``|
+----------------------------+-----------------------------------------------+----------------------------------------------------------------------+


Project Recovery
****************

See :ref:`project recovery <Project Recovery>` for more details.

+-----------------------------------------+-----------------------------------------------+------------------+
|Property                                 |Description                                    |Example value     |
+=========================================+===============================================+==================+
| ``projectRecovery.enabled``             |Whether project recovery is enabled            |  ``On``, ``Off`` |
+-----------------------------------------+-----------------------------------------------+------------------+
| ``projectRecovery.numberOfCheckpoints`` |How many checkpoints/backups to keep           | ``5``            |
+-----------------------------------------+-----------------------------------------------+------------------+
| ``projectRecovery.secondsBetween``      |How often to save checkpoints in seconds       | ``60``           |
+-----------------------------------------+-----------------------------------------------+------------------+

Project Saving
**************

+---------------------------------+------------------------------------------------------------------+------------------+
|Property                         |Description                                                       |Example value     |
+=================================+==================================================================+==================+
| ``projectSaving.warningSize``   |Size in bytes of a project before the user is warned when saving  |  ``10737418240`` |
+---------------------------------+------------------------------------------------------------------+------------------+

Plotting Settings
*****************

+-------------------------------------+------------------------------------------------------------------+----------------------+
|Property                             |Description                                                       |Example value         |
+=====================================+==================================================================+======================+
|``plots.ShowTitle``                  |Whether to show titles on plots                                   | ``On``, ``Off``      |
+-------------------------------------+------------------------------------------------------------------+----------------------+
|``plots.ShowLegend``                 |Whether to show legend on plots                                   | ``On``, ``Off``      |
+-------------------------------------+------------------------------------------------------------------+----------------------+
|``plots.font``                       |The default font for labels and titles on plots.                  |``Helvetica``         |
+-------------------------------------+------------------------------------------------------------------+----------------------+
|``plots.xAxesScale``                 |The default x scale on 1d plots                                   |``Linear``, ``Log``   |
+-------------------------------------+------------------------------------------------------------------+----------------------+
|``plots.yAxesScale``                 |The default y scale on 1d plots                                   |``Linear``, ``Log``   |
+-------------------------------------+------------------------------------------------------------------+----------------------+
|``plots.x_min``                      |The default minimum x range                                       |``10``                |
+-------------------------------------+------------------------------------------------------------------+----------------------+
|``plots.x_max``                      |The default maximum x range                                       |``1000``              |
+-------------------------------------+------------------------------------------------------------------+----------------------+
|``plots.y_min``                      |The default minimum y range                                       |``10``                |
+-------------------------------------+------------------------------------------------------------------+----------------------+
|``plots.y_max``                      |The default maximum y range                                       |``1000``              |
+-------------------------------------+------------------------------------------------------------------+----------------------+
|``plots.axesLineWidth``              |The default width of the lines that make the axes                 |``1``                 |
+-------------------------------------+------------------------------------------------------------------+----------------------+
|``plots.enableGrid``                 |The default y scale on 1d plots                                   |``Linear``, ``Log``   |
+-------------------------------------+------------------------------------------------------------------+----------------------+
|``plots.ShowMinorTicks``             |Whether to show minor ticks on plots                              | ``On``, ``Off``      |
+-------------------------------------+------------------------------------------------------------------+----------------------+
|``plots.ShowMinorGridlines``         |Whether to show minor gridlines on plots                          | ``On``, ``Off``      |
+-------------------------------------+------------------------------------------------------------------+----------------------+
|``plots.showTicksLeft``              |Whether to show ticks on the left side of the plot                | ``On``, ``Off``      |
+-------------------------------------+------------------------------------------------------------------+----------------------+
|``plots.showTicksBottom``            |Whether to show ticks on the bottom of the plot                   | ``On``, ``Off``      |
+-------------------------------------+------------------------------------------------------------------+----------------------+
|``plots.showTicksRight``             |Whether to show ticks on the right side of the plot               | ``On``, ``Off``      |
+-------------------------------------+------------------------------------------------------------------+----------------------+
|``plots.showTicksTop``               |Whether to show ticks on the top side of the plot                 | ``On``, ``Off``      |
+-------------------------------------+------------------------------------------------------------------+----------------------+
|``plots.showLabelsLeft``             |Whether to show labels on the left side of the plot               | ``On``, ``Off``      |
+-------------------------------------+------------------------------------------------------------------+----------------------+
|``plots.showLabelsBottom``           |Whether to show labels on the bottom of the plot                  | ``On``, ``Off``      |
+-------------------------------------+------------------------------------------------------------------+----------------------+
|``plots.showLabelsRight``            |Whether to show labels on the right side of the plot              | ``On``, ``Off``      |
+-------------------------------------+------------------------------------------------------------------+----------------------+
|``plots.showLabelsTop``              |Whether to show labels on the top side of the plot                | ``On``, ``Off``      |
+-------------------------------------+------------------------------------------------------------------+----------------------+
|``plots.ticks.major.length``         |The default length of the major ticks                             |``6``                 |
+-------------------------------------+------------------------------------------------------------------+----------------------+
|``plots.ticks.major.width``          |The default width of the major ticks                              |``1``                 |
+-------------------------------------+------------------------------------------------------------------+----------------------+
|``plots.ticks.major.direction``      |The default direction of the major ticks                          |``In``, ``Out``,      |
|                                     |                                                                  |``InOut``             |
+-------------------------------------+------------------------------------------------------------------+----------------------+
|``plots.ticks.minor.length``         |The default length of the minor ticks                             |``3``                 |
+-------------------------------------+------------------------------------------------------------------+----------------------+
|``plots.ticks.minor.width``          |The default width of the minor ticks                              |``1``                 |
+-------------------------------------+------------------------------------------------------------------+----------------------+
|``plots.ticks.minor.direction``      |The default direction of the minor ticks                          |``In``, ``Out``,      |
|                                     |                                                                  |``InOut``             |
+-------------------------------------+------------------------------------------------------------------+----------------------+
|``plots.line.Style``                 |Default Line style on 1d plots                                    |``solid``, ``dashed`` |
+-------------------------------------+------------------------------------------------------------------+----------------------+
|``plots.line.DrawStyle``             |Default Draw style on 1d plots                                    |``default``, ``steps``|
+-------------------------------------+------------------------------------------------------------------+----------------------+
|``plots.line.Width``                 |Default Line width on 1d plots                                    |``1.5``               |
+-------------------------------------+------------------------------------------------------------------+----------------------+
|``plots.marker.Style``               |Default marker style on 1d plots                                  |``point``             |
+-------------------------------------+------------------------------------------------------------------+----------------------+
|``plots.marker.Size``                |Default maker size on 1d plots                                    |``6``                 |
+-------------------------------------+------------------------------------------------------------------+----------------------+
|``plots.errorbar.Capsize``           |Default cap size on error bars in 1d plots                        |``1.0``               |
+-------------------------------------+------------------------------------------------------------------+----------------------+
|``plots.errorbar.CapThickness``      |Default cap thickness on error bars in 1d plots                   |``1.0``               |
+-------------------------------------+------------------------------------------------------------------+----------------------+
|``plots.errorbar.errorEvery``        |Default number of error bars for every data point                 |``1``                 |
|                                     |in 1d plots. Must be an integer                                   |                      |
+-------------------------------------+------------------------------------------------------------------+----------------------+
|``plots.errorbar.Width``             |Default width of error bars in 1d plots                           |``1.0``               |
+-------------------------------------+------------------------------------------------------------------+----------------------+
|``plots.errorbar.MarkerStyle``       |Default style for errorbar matrix workspace                       |``circle``            |
+-------------------------------------+------------------------------------------------------------------+----------------------+
|``plots.errorbar.MarkerSize``        |Default size for markers in the errorbar matrix workspace         |``4``                 |
+-------------------------------------+------------------------------------------------------------------+----------------------+
|``plots.markerworkspace.MarkerStyle``|Default marker style for the marker matrix workspace              |``vline``             |
+-------------------------------------+------------------------------------------------------------------+----------------------+
|``plots.markerworkspace.MarkerSize`` |Default marker size for the marker matrix workspace               |``6``                 |
+-------------------------------------+------------------------------------------------------------------+----------------------+
|``plots.legend.FontSize``            |Default legend font size                                          |``8.0``               |
+-------------------------------------+------------------------------------------------------------------+----------------------+
|``plots.legend.Location``            |Default legend location                                           |``best``              |
+-------------------------------------+------------------------------------------------------------------+----------------------+
|``plots.images.Colormap``            |Default colormap for image plots                                  |``viridis``           |
+-------------------------------------+------------------------------------------------------------------+----------------------+
|``plots.images.ColorBarScale``       |Default colorbar scale for image plots                            |``Linear``            |
+-------------------------------------+------------------------------------------------------------------+----------------------+

ISIS SANS Interface GUI Settings
*********************************

+---------------------------------+------------------------------------------------------------------+---------------------+
|Property                         |Description                                                       |Example value        |
+=================================+==================================================================+=====================+
|``sans.isis_sans.plotResults``   |Whether to show or hide plot results checkbox                     | ``On``, ``Off``     |
+---------------------------------+------------------------------------------------------------------+---------------------+

Algorithm Profiling Settings
****************************

.. _Algorithm_Profiling:

See :doc:`algorithm profiling <mantid-dev:AlgorithmProfiler>` for more details on using mantid profiler.

+---------------------------------+------------------------------------------------------------------+---------------------------+
|Property                         |Description                                                       |Example value              |
+=================================+==================================================================+===========================+
|``performancelog.filename``      |The filename for saving the log file. This can be the absolute    | ``algotimeregister.out``  |
|                                 |or relative path. This file is overwritten each session. Default  |                           |
|                                 |is ``algotimeregister.out``                                       |                           |
+---------------------------------+------------------------------------------------------------------+---------------------------+
|``performancelog.write``         |Enable or disable writing the performance log. Write is disabled  | ``On``, ``True``, ``1``,  |
|                                 |by default.                                                       | ``Off``, ``False``, ``0`` |
+---------------------------------+------------------------------------------------------------------+---------------------------+


Getting access to Mantid properties
***********************************

To get access to, e.g. data saving path property from a C++ program one has to issue the following command:


.. testcode:: properties

  path = ConfigService.getString("defaultsave.directory")


Modifying User Properties at Run Time
**************************************

:ref:`amend_config <Amend Config>` is a context manager that allows you to temporarily modify configuration settings
related to a facility, instrument, data directory, or any additional keyword arguments. It ensures that the changes are
only applied temporarily within the context and then restored to their original state when the context exits.


.. categories:: Concepts
