=====================
Reflectometry Changes
=====================

.. contents:: Table of Contents
   :local:

.. warning:: **Developers:** Sort changes under appropriate heading
    putting new features at the top of the section, followed by
    improvements, followed by bug fixes.

Algorithms
----------

New
###

- Added algorithm :ref:`algm-CreateFloodWorkspace` which makes a workspace for subsequent flood corrections.

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

:ref:`Release 3.14.0 <v3.14.0>`
