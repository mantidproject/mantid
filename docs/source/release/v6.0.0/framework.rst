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

- Added packing fraction to :ref:`Material <Materials>` to separate number density and effective number density.

Algorithms
----------

- The calculation of a distance has been updated in Track to correctly calculate the distance for objects that have multiple intercepting surfaces, e.g. hollow cylinder. This affect algorithms such as :ref:`AbsorptionCorrection <algm-AbsorptionCorrection>` where you may now get slightly different values.

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

Bugfixes
########


:ref:`Release 6.0.0 <v6.0.0>`
