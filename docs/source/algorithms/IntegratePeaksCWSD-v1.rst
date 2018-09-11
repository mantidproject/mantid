.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm performs integration of single-crystal peaks ... ...


Inputs
######

The algorithms takes 2 mandatory input workspaces and 1 optional workspace:

-  A MDEventWorkspace containing the events in multi-dimensional space.
-  A PeaksWorkspace containing the peaks to be integrated.
-  An optional MaskWorkspace to mask the pixels on the detector

Calculations
############

There are a few of algorithms that are or will be supported to integrate
single crystal diffraction peaks measured by a constant-wavelength reactor-based
diffractometer (aka. 4-circle).

Simple Peak Integration
=======================

Integration is performed by summing the signal from all MDEvents that
are not masked.
The integrated value will be normalized by the monitor counts.

The **assumption** is to load an experiment point (*Pt*) and convert to MDEventWorkspace in Q-space.
This *Pt*, i.e., workspace, contains at most one peak.
Algorithm *FindPeaksMD* is able to find a peak in the MDEventWorkspace and output to a PeaksWorkspace.
A :ref:`UB matrix <Lattice>` is set to this PeaksWorkspace as an option.

The **pseduo-code** of this algorith is:
 1. Go over all the MDEvents;
 2. For Pt. (aka, run number) i, its integrated intensity :math:`I_{i}` is calculated as
    :math:`I_{i} = \sum_{d=0}^{256\times 256}\frac{s_d}{m}`, where :math:`s_i` is the signal of detector i,
    and m is the monitor counts;

Here is how this peak integration algorithm is applied to GUI
 1. Find out all the run numbers by experiment number and scan number;
 2. Execute FindPeaksMD() on all runs (i.e., Pts);
 3. Calculate peak center by :math:`\vec{p} = \frac{1}{\sum_i C_i} \sum_i C_i * \vec{p}_i`, where
    :math:`\vec{p}` is the center of the peak, :math:`C_i` is the bin count of run i;
 4. Execute IntegratePeaksCWSD() to integrate peak for each run (or Pt.) regardless whether a peak is found;
 5. Loop all the Pt. and sum the intensity as :math:`I = \sum_i I_i`, where :math:


Calculation of integrated background
====================================

There are two approaches that are proposed to estimate integrated background.
 1. Simple background removal (3D to 2D)

    * Assuming that :math:`r_p` and :math:`r_b` are defined for the radius of peak and background respectively.
    * Calculate :math:`I_p` by integrating the signals within :math:`r_p`;
    * Calculate :math:`I_pb` by integrating the signals within :math:`r_b`;
    * Then integrated background :math:`I_b = I_pb - I_p`;



 2. Rock scan

Outputs
=======

Here are some values to be output.

  * :math:`I_{scan} \cdot sin(2\theta)` for each scan with :math:`2\theta` fixed,
    where :math:`2\theta` is to the center of the detector.

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
