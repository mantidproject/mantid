.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

The algorithm is essentially a wrapper around the :ref:`algm-MuonProcess` algorithm, summing a group of detector IDs, and optionally performing an asymmetry calculation. This algorithm only applies to a grouping of detectors (see ... for the equivalent algorithm on pairs of groups), it does not crop the data.

ApplyMuonDetectorGrouping takes an input workspace containing the data (containing multiple detector IDs), creates a unique name for the grouped data, adding it to (or replacing in) the supplied WorkspaceGroup. Multiple period data is supported through SummedPeriods and SubtractedPeriods. Various other optional behaviour is possible as detailed below.

Analysis
########

The algorithm supports two analysis types:

- *Counts* - The spectra associated to the given detector IDs via grouping are summed (with errors added in quadrature). Note that if several detector IDs are associated to the same spectra, the spectra will be counted **once for each ID**. 
- *Asymmetry* - The summation from Counts is performed. Then the asymmetry between the given group (specified via *GroupIndex*) and the Muon exponential decay is calculated (see :ref:`algm-EstimateMuonAsymmetryFromCounts`).

For multiple period data, the action of SummedPeriods and SubtractedPeriods is performed **before** the Counts/Asymmetry analysis.

The way in which period data is merged before the analysis is determined by
**SummedPeriodSet** and **SubtractedPeriodSet**.
For example, setting *SummedPeriodSet* to "1,2" and *SubtractedPeriodSet* to "3,4" would result in
the period arithmetic :math:`(1+2)-(3+4)`.

The workspaces which hold the analysed data are stored in the *WorkspaceGroup*, using a unique name. From the following variables

#. The *InputWorkspaceGroup* name is "inputGroup"
#. *analysisType* is "Counts"
#. *groupName* is "test"

then the name would be "inputGroup; Group; test; Counts; #1". A second workspace is also created with the same name, but ending in "#1_Raw", the only difference being that the *RebinArgs* are not applied to the latter workspace.

An option exists to apply dead time correction to the counts if *ApplyDeadTimeCorrection* is true, requiring a *DeadTimeTable* with the format as in :ref:`algm-DeadTimeCorr`.

The *TimeOffset* parameter shifts the time axis to start at the given time (all bins are offset by the value). In :ref:`algm-MuonProcess` this is equivalent to *LoadedTimeZero* - *TimeZero*. Note that the *TimeMin* and *TimeMax* refer to the values before the offset is applied.

Usage
-----

.. include:: ../usagedata-note.txt

.. note::

   For examples of applying custom dead times, please refer to :ref:`algm-ApplyDeadTimeCorr`
   documentation.

   For examples of applying custom grouping, please refer to :ref:`algm-MuonGroupDetectors`
   documentation.

.. categories::

.. sourcelink::
