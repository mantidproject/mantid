.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This SANS workflow algorithm will compute the sensitivity correction
from a given flood field data set. It will apply the proper corrections
to the data according the the input property manager object. Those
corrections may include dark current subtraction, moving the beam
center, the solid angle correction, and applying a patch.

If an input workspace is given, the computed correction will be applied
to that workspace.

A Nexus file containing a pre-calculated sensitivity correction can also
be supplied for the case where we simply want to apply the correction to
an input workspace.



The relative detector efficiency is computed using the 
:ref:`CalculateEfficiency <algm-CalculateEfficiency>`
algorithm the following way:

:math:`S(x,y)=\frac{I_{flood}(x,y)}{1/N_{pixels}\sum_{i,j}I_{flood}(i,j)}`

where :math:`I_{flood}(x,y)` is the pixel count of the flood data in pixel (x,y). 
If a minimum and/or maximum sensitivity is given, the pixels having an efficiency 
outside the given limits are masked and the efficiency is recomputed without using 
those pixels.
The sample data is then corrected by dividing the intensity in each pixels by 
the efficiency S:

:math:`I'_{sample}(x,y) = \frac{I_{sample}(x,y)}{S(x,y)}`

The pixels found to have an efficiency outside the given limits are also masked 
in the sample data so that they don’t enter any subsequent calculations.
If the user chose to use a dark current data set when starting the reduction 
process, that dark current data will be subtracted from the flood data. The 
subtraction is done before the sensitivity is calculated.
If the user chose to use the solid angle correction for the reduction process, 
that correction will be applied to the flood data before the sensitivity is calculated.

This algorithm is usually called by
:ref:`SANSReduction <algm-SANSReduction>` or :ref:`HFIRSANSReduction <algm-HFIRSANSReduction>`.

**Note 1**: The solid angle correction is either not applied at all, or applied to both 
the flood data to calculate the sensitivity correction and applied to the sample data as part of the reduction process.

**Note 2**: EQSANS has the option to patch masked areas of the detector using the
efficiency calculated for the unmasked portion of each tube. This creates a sensitivity
file that covers the entire detector. This option can be enabled by using the 
:ref:`ComputeSensitivity <algm-ComputeSensitivity>` 
workflow algorithm to create a sensitivity workspace, which can then be saved and used 
for EQSANS reduction as a pre-calculated sensitivity file.

.. categories::

.. sourcelink::
