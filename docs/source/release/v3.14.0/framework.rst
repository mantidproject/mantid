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

Stability
---------




Algorithms
----------


New Algorithms
##############



Improvements
############
- :ref:`AppendSpectra <algm-AppendSpectra>` can append now multiple times the same event workspace.
- :ref:`Live Data <algm-StartLiveData>` for events in PreserveEvents mode now produces workspaces that have bin boundaries which encompass the total x-range (TOF) for all events across all spectra.

Bugfixes
########
- :ref:`FilterEvents <algm-FilterEvents-v1>` output workspaces now contain the goniometer.


Python
------

New
###

- New python validator type: `:class:`~mantid.geometry.OrientedLattice`. Checks whether a workspace has an oriented lattice object attached.

- We have been making major performance improvements to geometry access in Mantid over the last few releases. We are now exposing these features via Python to give our users direct access to the same benefits as part of their scripts. The newly exposed objects are now available via workspaces and include:

 * :class:`mantid.geometry.ComponentInfo`
 * :class:`mantid.geometry.DetectorInfo`
 * :class:`mantid.api.SpectrumInfo`

- :class:`mantid.geometry.ComponentInfo` is exposed to allow the user to access geometric information about the components which are part of a beamline.

- :class:`mantid.geometry.DetectorInfo` offers the user the ability to access geometric information about the detector(s) which are part of a beamline. ``DetectorInfo`` has also been given an iterator to allow users to write more Pythonic loops rather than normal index based loops.

- :class:`mantid.api.SpectrumInfo` allows the user to access information about the spectra being used in a beamline. ``SpectrumInfo`` has also been given an iterator to allow users to write more Pythonic loops rather than normal index based loops. In addition to this ``SpectrumDefinition`` objects can also be accessed via a :class:`mantid.api.SpectrumInfo` object. The ``SpectrumDefinition`` object can be used to obtain information about the spectrum to detector mapping and provides a definition of what a spectrum comprises, i.e. indices of all detectors that contribute to the data stored in the spectrum.



Improvements
############

- :ref:`ChudleyElliot <func-ChudleyElliot>` includes hbar in the definition

Bugfixes
########


:ref:`Release 3.14.0 <v3.14.0>`
