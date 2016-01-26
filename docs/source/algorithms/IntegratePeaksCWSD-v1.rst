.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

This algorithm performs integration of single-crystal peaks ... ...


Inputs
######

The algorithms takes 2 mandatory input workspaces and 1 optional workspace:

-  A MDEventWorkspace containing the events in multi-dimensional space.
   This would be the output of
   :ref:`algm-???`.
-  A PeaksWorkspace containing the peaks to be integrated.
-  An optional MaskWorkspace to mask the pixels on the detector

Calculations
############

There are a few of algorithms that are or will be supported to integrate
single crystal diffraction peaks measured by a constant-wavelength reactor-based
diffractometer (aka. 4-circle).

Simple Peak Integration
=======================

Integration is performed by summing the signal strength from all MDEvents that 
are not masked.
The integrated value will be normalized by the monitor counts.

Masking
#######

Algorithm IntegratePeaksCWSD supports masking detectors. 
An optional MaskWorkspace will define all the detectors that will be masked.

Because the reactor-based single crystal diffratometer may have a moving detector,
the best way to mark a detector to be masked is by its original detector ID.


Background Subtraction
######################

The background signal within PeakRadius is calculated by scaling the
background signal density in the shell to the volume of the peak:

...


Workflow
--------

.. diagram:: HB3A-v1_wokflw.dot

Usage
------

**Example - IntegratePeaks:**


**Output:**

.. code-block:: python

.. categories::

.. sourcelink::
