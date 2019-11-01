============
SANS Changes
============

.. contents:: Table of Contents
   :local:

.. figure:: ../../images/ISISSansInterface/q_wavelength_release_4.2.png
  :class: screenshot
  :align: center
  :figwidth: 70%
  :alt: The Q, Wavelength tab of ISIS SANS


.. warning:: **Developers:** Sort changes under appropriate heading
    putting new features at the top of the section, followed by
    improvements, followed by bug fixes.

Caveats
#######
- Attempting to load or process data in the SANS GUI will initially inform
  the user that their file is not found. Pressing load or process again will
  correctly load or process data if the user directories is setup.

  This is a side-effect of the SANS usability improvements, specifically the
  one that avoids users having to re-enter their run number when the row turns
  blue. It will be resolved for the next release.

New
###
- Support for shifting both monitor 4 and 5 on Zoom including a new setting in
  the ISIS SANS GUI. A new user file command has also been added to
  perform monitor shifts without changing the selected transmission spectrum.
- New :ref:`HFIRSANS2Wavelength <algm-HFIRSANS2Wavelength-v1>` algorithm to
  "convert" CG2 event files to histograms in wavelength.

Improved
########

- Option in :ref:`EQSANSCorrectFrame <algm-EQSANSCorrectFrame>` to correct
  TOF by path to individual pixel.
- New CG2 definition file.
- Option in :ref:`EQSANSCorrectFrame <algm-EQSANSCorrectFrame-v1>` to correct
  TOF by path to individual pixel
- New CG2 definition file
- A bug causing large batch files (1000+ runs) to take minutes to load into the
  ISIS SANS GUI has been fixed. Large batch files will now load within seconds.
- :ref:`ApplyTransmissionCorrection <algm-ApplyTransmissionCorrection-v1>` now
  can be supplied any transmission workspace that is supported
  by :ref:`Divide <algm-Divide-v1>` .

Multiple ISIS SANS GUI usability fixes including:

- Numbers do not need to be re-entered if a file is not found, additionally
  Mantid will not attempt to find data until Load or Process is selected, so
  users can setup their data directories beforehand. However, this
  comes with the caveat above.
- Clicking on a cell in the table and typing will automatically start editing
  the cell without having to double click it.
- Sample thickness is set when a user presses load or process selected,
  this resolves problems with a run not being found and having to re-enter
  each number individually.
- Wavelength ranges such as *1,2,4,5* are now accepted. This example would
  convert into 1-2, 2-4, 4-5.
- *Open Mask File* and *Open Batch File* will remember their previously
  selected files.
- *Open Mask File* and *Open Batch File* will only show .txt and .csv files
  respectively by default.
- The "visual noise" of the *General* and *Q, Wavelength* settings tabs has
  been reduced.
- The check-boxes enabling extra table options, such as *Sample Geometry* have
  been moved alongside the table controls.

- Multiple SANS Workflow Algorithms were converted into internal scripts.
  This removes the need for passing SANSState objects in unrolled histories.
  Additionally, it speeds up the reduction of each run by ~35%.

Removed
#######

The following SANS Workflow algorithms were removed:
- SANSCalculateTransmission
- SANSCreateAdjustment
- SANSCrop
- SANSConvertToQ
- SANSConvertToWavelength
- SANSMaskWorkspace
- SANSMove
- SANSNormalizeToMonitor
- SANSScale
- SANSSliceEvent

:ref:`Release 4.2.0 <v4.2.0>`
