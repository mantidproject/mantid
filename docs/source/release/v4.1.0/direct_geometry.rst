=======================
Direct Geometry Changes
=======================

.. contents:: Table of Contents
   :local:

New Instruments
---------------

- Added support for the new Panther spectrometer at the ILL.


Algorithms
----------

New Algorithms
##############

- :ref:`algm-FlippingRatioCorrectionMD` algorithm was introduced to account for polarization effects on HYSPEC, but it's not instrument specific.

Improvements
############

- :ref:`CylinderAbsorption <algm-CylinderAbsorption>` now has a `CylinderAxis` property to set the direction of the cylinder axis.
- Improved support for thin-walled hollow cylinder shapes in :ref:`algm-MonteCarloAbsorption`.
  The points are now generated in cylindrical coordinates to improve performance.


Removed
#######

- The deprecated versions 1 and 2 of :ref:`algm-GetEiMonDet` have been removed. Use version 3 instead.
- All algorithms and code related to VATES Quantification have been removed.

Interfaces
----------

New features
############


Improvements
############

- New instrument geometry for CNCS.
- The :ref:`GetEiT0atSNS <algm-GetEiT0atSNS>` algorithm was improved to handle monitors where TOF is not wrapped to the first frame.
  It is also improved to handle the situation when monitor peak(s) are close to pump pulses (cannot be too close though).

Bugfixes
########

- :ref:`MDNorm <algm-MDNorm>` now allows completely integrating the energy transfer dimension.

Python
------

Improvements
############

- :func:`directtools.plotSofQW` now supports plotting of transposed and non-transposed workspaces.

- In :ref:`PyChop <PyChop>`, the sample-size effect calculation was improved to account for the annular shape.

:ref:`Release 4.1.0 <v4.1.0>`
