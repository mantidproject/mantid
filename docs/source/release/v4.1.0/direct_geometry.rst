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

Removed
#######

- The deprecated versions 1 and 2 of :ref:`algm-GetEiMonDet` have been removed. Use version 3 instead.

Interfaces
----------

New features
############


Improvements
############


Bugfixes
########

Python
------

Improvements
############

- :func:`directtools.plotSofQW` now supports plotting of transposed and non-transposed workspaces.

- In :ref:`PyChop <PyChop>`, the sample-size effect calculation was improved to account for the annular shape.

:ref:`Release 4.1.0 <v4.1.0>`
