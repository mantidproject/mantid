============
SANS Changes
============

.. contents:: Table of Contents
   :local:

.. warning:: **Developers:** Sort changes under appropriate heading
    putting new features at the top of the section, followed by
    improvements, followed by bug fixes.

Algorithms and instruments
--------------------------

New
###

 - :ref:`SANSILLParameterScan <algm-SANSILLParameterScan>` algorithm added, used to treat data from ILL's D16 on omega scan mode.

Bug Fixed
#########

- Applying a mask to a range of spectra after cropping to a bank could fail
  if there were gaps in the spectrum numbers. The masking will now skip
  over any spectrum numbers not in workspace and emit a warning.
- :ref:`SANSILLReduction <algm-SANSILLIntegration>` could fail if the detector was
  not aligned with the beam. The integration will now use another algorithm in this case.
- Stiching tab for ORNL SANS is visible in the Workbench

ISIS SANS Interface
-------------------

Fixed
#####

- A bug has been fixed where processing old data could fail if it involves -add files produced from 2013 or earlier.
- Batch file selector now only shows CSV files and will handle loading in non-CSV data (such as mask files) gracefully.

:ref:`Release 5.1.0 <v5.1.0>`
