======================
UI & Usability Changes
======================

.. contents:: Table of Contents
   :local:

.. warning:: **Developers:** Sort changes under appropriate heading
    putting new features at the top of the section, followed by
    improvements, followed by bug fixes.

Installation
------------

Workbench
---------

- Fixed a bug where MantidPlot would crash if the sample log fields used for run start and end contained non-ISO8601 conforming values.
- Fixed an issue where updating a workspace changes the number format from decimal to scientific notation if the workspace is being viewed.
- Added :mod:`mantid.plots` to provide convenience functions for plotting mantid workspaces with matplotlib

SliceViewer and Vates Simple Interface
--------------------------------------

- Update SwitchToSliceViewer (shift + click) in MultiSlice view to work with nonorthogonal axes.
- Pressing alt while clicking an arrow in the MultiSlice view opens a text box where one may precisely enter the slice position.
- Fixed bug which would cause slice viewer to crash when deleting an overlaid peaks workspace.
- Fixed a bug where overwriting peaks workspaces with overlaid in the slice viewer with peak backgrounds shown cause Mantid to crash.

.. figure:: ../../images/VatesMultiSliceView.png
   :class: screenshot
   :align: right

MultiDataset Fitting Interface
------------------------------

- After a simultaneous fit the parameters are saved in a TableWorkspace made to simplify plotting their values against the datasets.
  The parameters are organised into columns and each row corresponds to a dataset.

SpectrumView
------------

- The SpectrumView is now available as a standalone widget in mantidpython. Below is a simple example of how the widget can be used in python.

.. code-block:: python
   :emphasize-lines: 11,12

   import PyQt4
   import mantid.simpleapi as simpleapi
   import mantidqtpython as mpy
   import sys

   SpectrumView = mpy.MantidQt.SpectrumView.SpectrumView
   app = PyQt4.QtGui.QApplication(sys.argv)

   wsOut = simpleapi.Load("/Path/to/multispectrum/dataset")

   sv = SpectrumView()
   sv.renderWorkspace("wsOut")
   sv.show()
   app.exec_()

:ref:`Release 3.12.0 <v3.12.0>`
