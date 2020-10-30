============
SANS Changes
============

.. contents:: Table of Contents
   :local:

Algorithms and instruments
--------------------------

New
###

- A new algorithm :ref:`SANSILLParameterScan <algm-SANSILLParameterScan>` added, used to treat data from ILL's D16 on omega scan mode.
- :ref:`SaveCanSAS1D <algm-SaveCanSAS1D>` and :ref:`SaveNXCanSAS <algm-SaveNXCanSAS>` now include the batch
  file name in their metadata if one was used to produce the output.

Improvements
############

- In the :ref:`SANSILLAutoProcess <algm-SANSILLAutoProcess>` algorithm automatically produces a stitched result
  for I(Q) processing and several distances.
- Wedges processing in :ref:`SANSILLAutoProcess <algm-SANSILLAutoProcess>` is now functional.
- For the ILL D33 instrument, the detector panels can be processed individually (OutputPanels option, see
  the :ref:`SANSILLAutoProcess documentation <algm-SANSILLAutoProcess>` algorithm). This option is also
  available in the :ref:`DrILL interface <DrILL-ref>`.

Bug Fixed
#########

- Applying a mask to a range of spectra after cropping to a bank could fail
  if there were gaps in the spectrum numbers. The masking will now skip
  over any spectrum numbers not in workspace and emit a warning.
- The :ref:`SANSILLReduction <algm-SANSILLIntegration>` algorithm could fail if the detector was
  not aligned with the beam. The integration will now use another algorithm in this case.
- Stitching tab for ORNL SANS is visible in the Workbench

ISIS SANS Interface
-------------------

New
###

- TOML File V0 support; The format is pinned to version 0 to allow people to
  get a feel for the new format. The legacy parser still exists and has not
  been modified.

Fixed
#####

- A bug has been fixed where processing old data could fail if it involves `-add` files produced from 2013 or earlier.
- Batch file selector now only shows CSV files and will handle loading in non-CSV data (such as mask files) gracefully.

:ref:`Release 5.1.0 <v5.1.0>`
