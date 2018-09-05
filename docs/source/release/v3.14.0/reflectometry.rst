=====================
Reflectometry Changes
=====================

.. contents:: Table of Contents
   :local:


ISIS Reflectometry Interface
----------------------------

New
###

* ``SaveReflectometryAscii`` is a general algorithm which saves the first spectrum of a workspace in Ascii format particularly suited for reflectometry data.

Improvements
############

- The four Ascii save algorithms ``SaveANSTOAscii``, ``SaveILLCosmosAscii``, ``SaveReflCustomAscii`` and ``SaveReflThreeColumnAscii`` now correctly save x-error and can treat correctly point data and histograms. They are, however, deprecated in favour of ``SaveReflectometryAscii``. Please see ``SaveReflectometryAscii`` for more documentation.

Algorithms
----------

New
###

- Added algorithm :ref:`algm-CreateFloodWorkspace` which makes a workspace for subsequent flood corrections.
- Added algorithm :ref:`algm-ApplyFloodWorkspace` which applies flood corrections to a workspace.
- :ref:`FindReflectometryLines <algm-FindReflectometryLines-v2>` has been rewritten and updated to version 2. The new version finds a single line by a Gaussian fit. Version 1 has been deprecated and will be removed in a future release.

Liquids Reflectometer
---------------------

- Default x-direction pixel range for the scaling factor calculation is now set to the full width of the detector as opposed to a restricted guess.

ISIS Reflectometry Interface
----------------------------

New
###



Improved
########

- The interface now supports the Wildes method for polarization corrections as well as Fredrikze when configured in the parameters file.

Bug fixes
#########



Algorithms
----------


New
###



Improved
########

- :ref:`algm-ReflectometryReductionOneAuto` now supports the Wildes method for polarization corrections as well as Fredrikze when configured in the parameters file.

Bug fixes
#########

- A bug has been fixed on the Settings tab where the IncludePartialBins check box had been hidden by a misplaced text entry box.

Algorithms
----------

Bug fixes
#########

- In :ref:`algm-ReflectometryReductionOneAuto` an issue where if you gave only one of either MomentumTransferMax or MomentumTransferMin were specified it would be ignored, this has been fixed.

:ref:`Release 3.14.0 <v3.14.0>`
