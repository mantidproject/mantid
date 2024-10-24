.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Input
#####

This algorithm can be used for one run of sample data. For correction and background reduction
additional measurements of Vanadium(``VanaData``) and an empty can(``EmptyData``) can be specified
but are not necessary. If an empty measurement is specified, it is substracted from the sample
measurement. If a Vanadium measurement is specified, the sample measurement is divided by the
vanadium measurement. If either or both are not specified these steps are skipped. It is highly
recommended to specify both. Additionally, an empty vanadium measurement can be supplied.

Calibration, Grouping, Masking
##############################

The calibration is done using the Calibration File(``CalFile``). Additionally three
implicit workspaces (``<instrument>_group``, ``<instrument>_cal``, ``<instrument>_mask``) are
created during the algorithms execution if they do not exist already. Masking can be applied by an xml file as well.

Binning
#######
The recommended binning (edgebinning) requires an edgebinning file to be specified. If no
edgebinning file is specified, logarithmic binning (standard values: x1=-0.008, x2=0.01) is used.

Manipulating data with constants
################################

The reduced data are checked for negative intensities. If any are found, they are removed either
by adding the most negative valueor by setting the intensity to a specified value. This is done,
because negative values cannot be processed in multidimensional Rietveld refinement.

Output
######
The output of this Workflow algorithm is a p2d file. The p2d file contains values for 2Theta
and lambda (columns 1 and 2) as well as d and dperp (columns 3 and 4). Column 5 contains the
intensity data.


Workflow
-----------

.. diagram:: PowderReduceP2D.dot
.. diagram:: PowderReduceP2D_ProcessSampleRun.dot
.. diagram:: PowderReduceP2D_ProcessCanRun.dot
.. diagram:: PowderReduceP2D_ProcessVanadiumRun.dot

Usage
-----

This is a workflow algorithm to process the results of powder diffraction experiments and create a p2d file for
multidimensional Rietveld refinement. The algorithm is currently tested for the Instruments POWGEN, SNAP and
PTXatPG3 (POWTEX detector at POWGEN instrument).

.. categories::

.. sourcelink::
