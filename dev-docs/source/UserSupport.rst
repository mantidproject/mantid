.. _UserSupport:

============
User Support
============

.. contents::
  :local:

Introduction
############

As Mantid continues to facilitate cutting-edge scientific research, for an 
increasing number of users, the support side of Mantid is growing more 
and more important. This can be in many circumstances and through 
different avenues; therefore, below is detailed our support procedures.

The main purpose of user support for the Mantid project, is to aide contact between the users and developers.

.. figure:: images/errorReporter.png
   :class: screenshot
   :width: 700px
   :align: right
   :alt: Error reporter
        
   *Error reporter sends details directly to Mantid support*

Bugs and Error Reports
######################

1.	Users can report bugs via the `Mantid Help Forum <https://forum.mantidproject.org/>`_ or the `Mantid Help Email <mantid-help@mantidproject.org>`_, or from collected **Error Reports**. Currently this is a quick first contact with the team, but doesn't give much detail about the usage or unexpected error.
2.	The bug is verified and reproduced by the support team.
3.	The impact and importance are assessed by the support team member by contacting the users, instrument scientists, developers or project manager as appropriate.
4.	A GitHub issue to resolve the problem is created if appropriate and/or workaround tested if possible.
5.	The user is contacted to give a link to the created issue and/or workaround solution, by the support team.
6.	When any issue is completed naming a user, that user is contacted to let them know it will be available in the nightly build and next release.  The gatekeeper that merges the fix should message the appropriate developer, to remind them to contact the original reporter. This could simply be through adding a comment while merging that points this out.


Troubleshooting
###############

This is a list designed to take a user through how to gain diagnostic information, particularly when Mantid (Workbench) fails to **launch**.

For performance profiling check out our `recommended tools <http://developer.mantidproject.org/ToolsOverview.html#profiling>`_. 


.. _Trouble_Windows:

Windows
=======

For a full release, ``C:\MantidInstall\`` is likely the correct install path. Take care to readjust this to ``C:\MantidNightlyInstall\`` if you are diagnosing a nightly version.

1. Does the **splash screen** appear? Can get a rough idea how far through launch it stops.


2. Does the **error reporter** appear? Is there a useful stacktrace? [If the errorreport won't send, the user can check with "Show Details"]


3. Try launching from a command prompt:

.. code-block:: python

	C:\MantidInstall\bin\MantidWorkbench

If this does not work, try launching with: 

.. code-block:: python

	cd C:\MantidInstall\bin
	set QT_PLUGIN_PATH=%CD%\..\plugins\qt5
	export PYTHONPATH=%CD%;%PYTHONPATH%
	python -m workbench.app.main


4. Does **Qt** import correctly? In a command prompt / terminal window, run the following:

.. code-block:: python

    C:\MantidInstall\bin\mantidpython.bat --classic
    import qtpy.QtCore


5. Do **Mantid Algorithms** import correctly?

.. code-block:: python

    C:\MantidInstall\bin\mantidpython.bat --classic
    import mantid.simpleapi


6. Turn off **Server Checks**: Open ``C:\MantidInstall\bin\Mantid.user.properties`` in any texteditor, add each code line to the end of the file and try to open Workbench after each.
	a. Instrument File : ``UpdateInstrumentDefinitions.OnStartup = 0``
	b. Mantid Version : ``CheckMantidVersion.OnStartup = 0``
	c. Usage Reporting: ``usagereports.enabled = 0``
	d. Project Recovery: ``projectRecovery.enabled=false``


7. Try renaming **Config Files**:

.. code-block:: python

	cd %APPDATA%\mantidproject
	mv mantidproject.ini mantidproject.ini.backup
	# Try again to start workbench, if that doesn't work ...
	
	cd %APPDATA%\mantidproject
	mv mantid mantidbackup
	# Try again to start workbench

Advanced options:

8. Check the PATH for conflicts with Mantid:

.. code-block:: python

	echo %PATH%

.. code-block:: python

    cd C:\MantidInstall\bin\
    python -c "import sys; import os; import pprint; pprint.pprint(sys.path); pprint.pprint(os.environ)"

9. Check for conflicts with **numpy**: ``python -c "import numpy; print(numpy.__file__)"`` Anything outside of ``C:\MantidInstall`` could be a problem.


10.  Try to open workbench. After it fails, open **Event Viewer** (just search in the Windows menu bar) and selected ``Windows Logs->Application``. Double-click on the relevant error line/s and send the crash information to the dev team.

11. **Process Monitor**: `Download here <https://docs.microsoft.com/en-us/sysinternals/downloads/procmon>`_. Extract the ProcessMonitor.zip and run Procmon.exe (which requires admin credentials). Set up a configuration filter for ``Process Name contains python``, click ``ADD``, ``APPLY``, ``OK`` and then launch Mantid Workbench, then back in Process Monitor select File>Save and save as a ``LogFile.PML`` file and send to the dev team.


.. _Trouble_Linux:

Linux
======

For a full release, ``/opt/Mantid/`` is likely the correct install path. Take care to readjust this to ``/opt/mantidnightly/`` if you are diagnosing a nightly version.

1. Does the **splash screen** appear? Can get a rough idea how far through launch it stops.


2. Does the **error reporter** appear? Is there a useful stacktrace? [If the errorreport won't send, the user can check with "Show Details"]


3. Try launching from the terminal:

.. code-block:: python

	/opt/Mantid/bin/mantidworkbench


4. Does **Qt** import correctly? In terminal, run the following:

.. code-block:: python

    /opt/Mantid/bin/mantidpython --classic
    import qtpy.QtCore


5. Do **Mantid Algorithms** import correctly?

.. code-block:: python

    /opt/Mantid/bin/mantidpython --classic
    import mantid.simpleapi


6. Try renaming **Config Files**:

.. code-block:: python

	cd $HOME/.config/mantidproject
	mv mantidproject.ini mantidproject.ini.backup
	# Try again to start workbench, if that doesn't work ...

	cd $HOME
	mv .mantid .mantidbackup
	# Try again to start workbench


7. Turn off **Server Checks**: Open ``$HOME/.mantid/Mantid.user.properties`` in any texteditor, add each code line to the end of the file and try to open Workbench after each.
	a. Instrument File : ``UpdateInstrumentDefinitions.OnStartup = 0``
	b. Mantid Version : ``CheckMantidVersion.OnStartup = 0``
	c. Usage Reporting: ``usagereports.enabled = 0``
	d. Project Recovery: ``projectRecovery.enabled=false``


Advanced Options:


8. Check the PATH for conflicts with Mantid: e.g. Anything relating to ``.local`` could be a problem.

.. code-block:: python

	echo $PATH

.. code-block:: python

    cd /opt/Mantid/bin/
    python -c "import sys; import os; import pprint; pprint.pprint(sys.path); pprint.pprint(os.environ)"


9. Check for conflicts with **numpy**: ``python -c "import numpy; print(numpy.__file__)"`` Anything relating to ``.local`` could be a problem.


10. Further diagnosis for process monitoring: `strace <https://strace.io/>`_.


.. _Trouble_MacOS:

MacOS
=====

1. Does the **splash screen** appear? Can get a rough idea how far through launch it stops.


2. Does the **error reporter** appear? Is there a useful stacktrace? [If the errorreport won't send, the user can check with "Show Details"]


3. Try launching from terminal, by running the following:

.. code-block:: python

	/Applications/MantidWorkbench.app/Contents/MacOS/MantidWorkbench

If this does not work, try launching with: 

.. code-block:: python

	cd /Applications/MantidWorkbench.app/Contents/MacOS
	export QT_PLUGIN_PATH=$PWD/../PlugIns/
	export PYTHONPATH=$PWD:$PYTHONPATH
	python3 -m workbench.app.main


4. Does **Qt** import correctly? 

.. code-block:: python

    /Applications/MantidWorkbench.app/Contents/MacOS/mantidpython --classic
    import qtpy.QtCore


5. Do **Mantid Algorithms** import correctly?

.. code-block:: python

    /Applications/MantidWorkbench.app/Contents/MacOS/mantidpython --classic
    import mantid.simpleapi


6. Turn off **Server Checks**: Open ``$HOME/.mantid/Mantid.user.properties`` in any texteditor, add each code line to the end of the file and try to open Workbench after each.
	a. Instrument File : ``UpdateInstrumentDefinitions.OnStartup = 0``
	b. Mantid Version : ``CheckMantidVersion.OnStartup = 0``
	c. Usage Reporting: ``usagereports.enabled = 0``
	d. Project Recovery: ``projectRecovery.enabled=false``


7. Try renaming **Config files**:

.. code-block:: python

	cd $HOME/.config/mantidproject
	mv mantidproject.ini mantidproject.ini.backup
	# Try again to start workbench, if that doesn't work ...

	cd ~
	mv .mantid .mantidbackup
	# Try again to start workbench

Advanced Options:


8. Check the PATH for conflicts with Mantid: e.g. Anything relating to ``.local`` could be a problem.

.. code-block:: python

	echo $PATH

.. code-block:: python

    cd /Applications/MantidWorkbench.app/Contents/MacOS/
    python -c "import sys; import os; import pprint; pprint.pprint(sys.path); pprint.pprint(os.environ)"


9. Check for conflicts with **numpy**: ``python -c "import numpy; print(numpy.__file__)"`` Anything relating to ``.local`` could be a problem.


10. Further diagnosis for process monitoring: `dtrace <http://dtrace.org/>`_.
