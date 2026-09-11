.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Performs a reduction from raw time of flight to energy transfer for an inelastic
indirect geometry instrument at ISIS.

OSIRIS silicon analyser data are supported for the ``111`` and ``333`` reflections. The default spectrum range and
rebinning are obtained from the selected instrument parameter file. For silicon reductions, unreliable edge pixels
are excluded before calibration and reduction. When a calibration workspace is supplied, spectra removed from that
workspace are also excluded from the sample reduction.

The ``Detectors`` grouping method groups the remaining silicon pixels tube by tube using the supplied OSIRIS grouping
file. ``ThetaGroups`` divides the fixed :math:`2\theta` range defined by the instrument parameter file's ``theta-min``
and ``theta-max`` parameters (in degrees) into ``NGroups`` equal-width angular bins. Both OSIRIS silicon reflections
use limits of 8 and 163 degrees. Masking, removing edge pixels or selecting a narrower ``SpectraRange`` does not
change these boundaries. Spectra outside the angular limits are excluded, and empty bins are omitted from the output.
Both outer limits are included; a spectrum exactly on an internal boundary belongs to the lower-angle bin.

Workflow
--------

.. diagram:: ISISIndirectEnergyTransfer-v1_wkflw.dot

Usage
-----

.. include:: ../usagedata-note.txt

**Example - IRIS energy conversion**

.. testcode:: ExIRISReduction

   ISISIndirectEnergyTransfer(InputFiles='IRS21360.raw',
                              OutputWorkspace='IndirectReductions',
                              Instrument='IRIS',
                              Analyser='graphite',
                              Reflection='002',
                              SpectraRange=[3, 53])

   reduction_workspace_names = mtd['IndirectReductions'].getNames()

   for workspace_name in reduction_workspace_names:
      print(workspace_name)

Output:

.. testoutput:: ExIRISReduction

   iris21360_graphite002_red

.. categories::

.. sourcelink::
