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

Bugfixes
########
- :ref:`FilterEvents <algm-FilterEvents-v1>` output workspaces now contain the goniometer.


Python
------

New
###

- New python validator type: `:class:`~mantid.geometry.OrientedLattice`. Checks whether a workspace has an oriented lattice object attached.

- As part of the initiative to move towards ``Instrument 2.0`` new objects have been exposed to Python and are now available via workspaces. The newly exposed classes include:

 * :class:`mantid.geometry.ComponentInfo`
 * :class:`mantid.geometry.DetectorInfo`
 * :class:`mantid.api.SpectrumInfo`

- :class:`mantid.geometry.ComponentInfo` is exposed to allow the user to access geometric information about the components which are part of a beamline. The full documentation can be found at docs/source/api/python/mantid/geometry/ComponentInfo.rst (hyperlink to be added).

- :class:`mantid.geometry.DetectorInfo` offers the user the ability to access geometric information about the detector(s) which are part of a beamline. ``DetectorInfo`` has also been given an iterator to allow users to write more Pythonic loops rather than normal index based loops - this is also helpful for abstraction. The full documentation can be found at docs/source/api/python/mantid/geometry/DetectorInfo.rst (hyperlink to be added).

- :class:`mantid.api.SpectrumInfo` allows the user to access information about the spectra being used in a beamline. In addition to this ``SpectrumDefinition`` objects can also be accessed via a :class:`mantid.api.SpectrumInfo` object. The full documentation can be found at docs/source/api/python/mantid/api/SpectrumInfo.rst (hyperlink to be added).



Improvements
############

SpectrumInfo now has more methods exposed to Python meaning users can access more information about the beamline that they are using.

- :ref:`ChudleyElliot <func-ChudleyElliot>` includes hbar in the definition

Bugfixes
########


:ref:`Release 3.14.0 <v3.14.0>`
