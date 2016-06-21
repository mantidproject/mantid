.. _Ragged_Workspace:

Ragged Workspace
=================

What are they?
--------------

A ragged workspace is one where the spectra within it do not share a common set of x values. Ragged workspaces commonly occur after Converting the X units using ConvertUnits or after DiffractionFocussing.

Displaying the ragged data using 1D line plots or 2D colour or contour plots is fine, but the data will display incorrectly when using 3D plots. The instrument view is unaffected as it displays integrated data.

Many algorithms cannot operate on Ragged Workspaces, either due to logical or implementation restrictions. If you encounter these problems you may wish to run a Rebin algorithm first to map the data to a regular datagrid of X values.


.. categories:: Concepts