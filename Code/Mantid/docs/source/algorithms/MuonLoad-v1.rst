.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

The algorithm replicates the sequence of actions undertaken by
MuonAnalysis in order to produce a Muon workspace ready for fitting.

Specifically:

#. Load the specified filename
#. Apply dead time correction
#. Group the workspace
#. Offset, crop and rebin the workspace
#. Use :ref:`algm-MuonCalculateAsymmetry` to get the
   resulting workspace.

Workflow
--------

.. diagram:: MuonLoad-v1_wkflw.dot

Usage
-----

.. include:: ../usagedata-note.txt

.. note::

   For more extensive usage examples on result type / period options please refer to the
   :ref:`algm-MuonCalculateAsymmetry` documentation.

   For example of applying custom dead times, please refer to :ref:`algm-ApplyDeadTimeCorr`
   documentation.

   For example of applying custom grouping, please refer to :ref:`algm-MuonGroupDetectors`
   documentation.

**Example - Integrated pair asymmetry for MUSR run:**

.. testcode:: ExPairAsymmetry

   output = MuonLoad(Filename = 'MUSR0015189.nxs',
                     TimeZero = 0.55,
                     Xmin = 0.11,
                     Xmax = 12,
                     OutputType = "PairAsymmetry",
                     PairFirstIndex = 0,
                     PairSecondIndex = 1,
                     Alpha = 1.0)

   output_int = Integration(output)

   print 'Integrated asymmetry for the run: {0:.3f}'.format(output_int.readY(0)[0])

Output:

.. testoutput:: ExPairAsymmetry

   Integrated asymmetry for the run: 1.701

.. categories::
