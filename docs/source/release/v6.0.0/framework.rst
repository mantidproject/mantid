=================
Framework Changes
=================

.. contents:: Table of Contents
   :local:

.. warning:: **Developers:** Sort changes under appropriate heading
    putting new features at the top of the section, followed by
    improvements, followed by bug fixes.

Concepts
--------

- Change sphinx documentation to use mathjax to render equations
- Added packing fraction to :ref:`Material <Materials>` to separate number density and effective number density.
- Added a feature allowing time-dependent values for individual instrument parameters.

Algorithms
----------

- The calculation of a distance has been updated in Track to correctly calculate the distance for objects that have multiple intercepting surfaces, e.g. hollow cylinder. This affect algorithms such as :ref:`AbsorptionCorrection <algm-AbsorptionCorrection>` where you may now get slightly different values.
- Added the ability to specify the packing fraction and effective number density to :ref:`SetSample <algm-SetSample>` and :ref:`SetSampleMaterial <algm-SetSampleMaterial>`.

Data Objects
------------

Python
------


.. contents:: Table of Contents
   :local:

.. warning:: **Developers:** Sort changes under appropriate heading
    putting new features at the top of the section, followed by
    improvements, followed by bug fixes.

Installation
------------


MantidWorkbench
---------------

See :doc:`mantidworkbench`.

SliceViewer and Vates Simple Interface
--------------------------------------

Improvements
############
- Member function: MDGeometry::getNumNonIntegratedDims() returns the number of non-integrated dimensions present.

Bugfixes
########
- Error log messages from an EqualBinChecker are now no longer produced when editing python scripts if a workspace is present with unequal bin sizes
- Warning log messages from the InstrumentValidator are no longer produced when editing some python scripts.
- A bug has been fixed when plotting bin plots on a workspace with numerical axis.
- A bug is fixed when setting the same axis to multiple workspaces, which would cause a crash when deleting the workspaces.

:ref:`Release 6.0.0 <v6.0.0>`
