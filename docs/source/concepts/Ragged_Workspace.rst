.. _Ragged_Workspace:

Ragged Workspace
=================

What are they?
--------------

A ragged workspace is one where the spectra within it do not share a common set of x values. Ragged workspaces commonly occur after converting the X units using :ref:`ConvertUnits <algm-ConvertUnits>` or after :ref:`DiffractionFocussing <algm-DiffractionFocussing>`.

Displaying the ragged data using 1D line plots or 2D colour or contour plots is fine, but the data will display incorrectly when using 3D plots. The instrument view is unaffected as it displays integrated data.

Many algorithms cannot operate on Ragged Workspaces, either due to logical or implementation restrictions. If you encounter these problems you may wish to run a :ref:`Rebin <algm-Rebin>` algorithm first to map the data to a regular datagrid of X values.

.. image:: /images/MBC_Ragged.png
   :width: 40%
   :alt: Ragged Workspace

.. categories:: Concepts
