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

Improved
########

- Option in :ref:`EQSANSCorrectFrame <algm-EQSANSCorrectFrame>` to correct
  TOF by path to individual pixel.
- New CG2 definition file.

Multiple GUI usability fixes including:

- Run numbers can be edited with a single click rather than double or
  triple clicks
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
- Clicking on a cell in the table and typing will automatically start editing
  the cell without having to double click it.


:ref:`Release 4.2.0 <v4.2.0>`
