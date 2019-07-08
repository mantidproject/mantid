=======================
Direct Geometry Changes
=======================

.. contents:: Table of Contents
   :local:

.. warning:: **Developers:** Sort changes under appropriate heading
    putting new features at the top of the section, followed by
    improvements, followed by bug fixes.

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

Removed
#######

- The deprecated versions 1 and 2 of :ref:`algm-GetEiMonDet` have been removed. Use version 3 instead.

Interfaces
----------

New features
############


Improvements
############

- The :ref:`GetEiT0atSNS <algm-GetEiT0atSNS>` algorithm was improved to handle monitors where TOF is not wrapped to the first frame.


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
