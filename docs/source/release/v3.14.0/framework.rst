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

- We have changed the logging in Mantid to stop writing the a high level version of the log to a file.  This had been causing numerous problems including inconsistent behaviour with multiple instances of Mantid, performance problems when logging at detailed levels, and excessive network usage in some scenarios.  This does not change the rest of the logging that you see in the message display in Mantidplot or the console window. A warning message will appear if configuration for the removed componets of logging is found.

  - Associated with this we have also simplified the python methods used to control logging.

    .. code-block:: python

	  	# The two methods
	  	ConfigService.SetConsoleLogLevel(int)
	  	ConfigService.SetFileLogLevel(int)

	  	# Have been replaced by
	  	ConfigService.SetLogLevel(int)

Nexus Geometry Loading
----------------------
:ref:`LoadEmptyInstrument <algm-LoadEmptyInstrument>` will now load instrument geometry from hdf5 `NeXus <https://www.nexusformat.org/>`_ format files. Files consistent with the standard following the introduction of `NXinstrument <http://download.nexusformat.org/doc/html/classes/base_classes/NXinstrument.html?highlight=nxinstrument>`_ (and related types) will be used to build the entire in-memory instrument geometry within Mantid. This IDF-free route is primarily envisioned for the ESS. This marks the completion of the first phase in the feasibility and rollout of support for the new format. Over coming releases we will be expanding our support for the NeXus geometry both across Loading and Saving algorithms. While dependent on the instrument, we are overall seeing significant improvements in instrument load times over loading from equivalent IDF based implementations.

Stability
---------




Algorithms
----------


New Algorithms
##############



Improvements
############
- :ref:`AppendSpectra <algm-AppendSpectra>` can append now multiple times the same event workspace.

Bugfixes
########
- :ref:`FilterEvents <algm-FilterEvents-v1>` output workspaces now contain the goniometer.


Python
------

New
###

 - New python validator type: `:class:`~mantid.geometry.OrientedLattice`. Checks whether a workspace has an oriented lattice object attached.


Improvements
############

- :ref:`ChudleyElliot <func-ChudleyElliot>` includes hbar in the definition

Bugfixes
########


:ref:`Release 3.14.0 <v3.14.0>`


