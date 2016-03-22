Dynamic PDF
================================

.. contents:: Table of Contents
  :local:

Background remover
------------------

Overview
~~~~~~~~

.. interface:: Dynamic PDF Background Remover
  :align: right
  :width: 350

The background remover is a tool to obtain the dynamic radial
distribution G(r,E) function after removal of the multi-phonon background
with a fitting procedure.

Unless stated otherwise in the Mantid preferences, the interface will open as
a window docked within the Mantid application. The interface window can be
undocked by changing its state to *floating*. This can be desirable when
displaying Mantid in small monitors. In the Menu, go to **Windows**, then
select **Change to floating**.


Action Buttons
~~~~~~~~~~~~~~

Load
  Summons the Slice Selector for loading of slices.

?
  Opens this help page.


Slice Selector
--------------

Overview
~~~~~~~~

The Slice Selector interface loads a MatrixWorkspace containing structure
factor S(Q,E). Alternatively, one can load a file containing S(Q,E).

A 2D intensity plot shows overall S(Q,E). A slice, or cut, can be
selected by entering a slice index (beginning at 0, not 1) or by
dragging up and down the horizontal dashed-line in the 2D view. The
slice is shown in the 1D plot.

Action Buttons
~~~~~~~~~~~~~~

"Fit"
  Selects the slice for fitting in the Background Remover interface.


.. categories:: Interfaces DynamicPDF

Developer's Corner
----------------

Diagrams
~~~~~~~~~~~~~~~~~~~~~

.. diagram:: DPDFBackgroundRemover_mainComponents.dot

.. diagram:: DPDFBackgroundRemover_workspaceUpdated.dot

.. diagram:: DPDFBackgroundRemover_sliceSelected.dot
