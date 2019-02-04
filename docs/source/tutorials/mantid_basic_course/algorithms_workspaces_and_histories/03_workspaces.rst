.. _03_workspaces:

==========
Workspaces 
==========

Workspaces come in several forms, but the most common by far is the
:ref:`MatrixWorkspace` which represents XYE data for one
or more spectra. The MatrixWorkspace itself can be sub-grouped into
:ref:`EventWorkspace` and :ref:`Workspace2D`.

.. figure:: /images/MatrixWorkspaceHierachy.png
   :alt: MatrixWorkspaceHierachy.png
   :width: 300px

Workspace2D
===========

A Workspace2D consists of a workspace with 1 or more spectra. Typically,
each spectrum will be a histogram. For each spectrum X, Y (counts) and E
(error) data is stored as a separate array.

|MBC_Workspace2D.png|

Each workspace has two axes, the Spectrum axis and the X-axis. Where
an axis holds a known unit type, it may be converted to another set of
units.

|MBC_axes_annotated.png|

Rebinning a Workspace2D is a one-way process when the rebinning leads to
a coarser structure.

Ragged Workspaces
-----------------

Converting x-axis units can lead to a ragged workspace, in which bin
boundaries are not consistent across the spectra. Some algorithms will
not accept input workspaces that are ragged. The fix to this is to apply
Rebin to the Ragged workspace structure, as the example below shows.

#. run **Load** on *GEM38370_Focussed.nxs* setting the
   **OutputWorkspace** to be *ws*
#. run **ConvertUnits** on *ws* setting **OutputWorkspace** to *lambda*,
   **Target**\ =\ *Wavelength*, **EMode**\ =\ *Elastic*. Plotting this
   in the *Color Fill Plot* demonstrates the ragged X-bins.

   |MBC_Ragged.png|

#. run **Rebin** on *lambda* setting **Params** to *0.5* and
   **OutputWorkspace** to *Rebinned*. Plotting this in the *Color Fill
   Plot* demonstrates that uniform binning across all spectra has been
   achieved.

   |MBC_Rebinned.png|

Event Workspaces
================

An :ref:`EventWorkspace` stores information about each
individual event observation in detectors. More specifically, at a
neutron spallation source, this means that the time of arrival and
detector ID of each individual neutron is recorded. Only fairly recent
advances in computer and acquisition hardware have made storing this
detailed knowledge a practical solution. For example at the SNS facility
all data, except for data collected in monitors, are stored in this way.

Event specifies “when” and “where”

**Pulse time** – when the proton pulse happened in absolute time

**Time-of-flight** – time for the neutron to travel from moderator to
the detector

Basic Example
-------------

.. figure:: /images/Binning_example.png
   :alt: Binning_example.png
   :width: 500px

Rebinning
---------

-  Rebinning is essentially free and can be conducted in-place. This is
   because the data does not need to change, only the overlaying
   histogramming.

Performance
-----------

-  Each event list is separate
-  Sorting events is O(n) = n log(n)
-  Histogramming is O(n) = n
-  Only histogram as needed

Example of Workspace usage
==========================

#. Load the event data HYS_11388_event.nxs
#. Execute the 'SumSpectra' algorithm
#. Rebin with Params=300 and plot, ensure PreserveEvents=True

   |MBC_Rebin_Coarse.png|

#. Rebin with Params=100, the plot will automatically update, ensure
   PreserveEvents=True

   |MBC_Rebin_MED.png|

#. Rebin with Params=10 the plot will automatically update, ensure
   PreserveEvents=True

   |MBC_Rebin_Fine.png|

Keep the workspace open for the next section.

Other Workspace Types
=====================

-  GroupWorkspaces store a collection of other workspaces in a group,
   this can be created manually and is often used in multi-period data.
   Either the whole group or individual members can be processed using
   algorithms.
-  TableWorkspaces stores data as cells. Columns determine the type of
   the data, for example double precision float, while each entry
   appears as a new row. This is analogous to a Microsoft Excel
   Spreadsheet.
-  PeaksWorkspace is a special type of TableWorkspace with additional
   support for Single Crystal peaks.
-  :ref:`MDWorkspace` will be covered later in this course

.. raw:: mediawiki

   {{SlideNavigationLinks|MBC_Algorithms|Mantid_Basic_Course|MBC_History}}

.. |MBC_Workspace2D.png| image:: /images/MBC_Workspace2D.png
   :width: 400px
.. |MBC_axes_annotated.png| image:: /images/MBC_axes_annotated.png
   :width: 400px
.. |MBC_Ragged.png| image:: /images/MBC_Ragged.png
   :width: 300px
.. |MBC_Rebinned.png| image:: /images/MBC_Rebinned.png
   :width: 300px
.. |MBC_Rebin_Coarse.png| image:: /images/MBC_Rebin_Coarse.png
   :width: 400px
.. |MBC_Rebin_MED.png| image:: /images/MBC_Rebin_MED.png
   :width: 400px
.. |MBC_Rebin_Fine.png| image:: /images/MBC_Rebin_Fine.png
   :width: 400px
