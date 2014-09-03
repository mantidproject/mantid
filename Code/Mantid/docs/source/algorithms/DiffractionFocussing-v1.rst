.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

|Example of RAW GEM data focused across the 5 detector banks| Given an
InputWorkspace and a Grouping filename, the algorithm performs the
following:

#. The calibration file is read and a map of corresponding udet-group is
   created.
#. The algorithm determine the X boundaries for each group as the upper
   and lower limits of all contributing detectors to this group and
   determine a logarithmic step that will ensure preserving the number
   of bins in the initial workspace.
#. All histograms are read and rebinned to the new grid for their group.
#. A new workspace with N histograms is created.

Within the `CalFile <http://www.mantidproject.org/CalFile>`_ any detectors with the 'select' flag
can be set to zero or with a group number of 0 or -ve groups are not
included in the analysis.

Since the new X boundaries depend on the group and not the entire
workspace, this focusing algorithm does not create overestimated X
ranges for multi-group instruments. However it is important to remember
that this means that this algorithm outputs a `ragged
workspace <http://www.mantidproject.org/Ragged_Workspace>`_. Some 2D and 3D plots will not display
the data correctly.

The DiffractionFocussing algorithm uses GroupDetectors algorithm to
combine data from several spectra according to GroupingFileName file
which is a `CalFile <http://www.mantidproject.org/CalFile>`_.

For EventWorkspaces
###################

The algorithm can be used with an :ref:`EventWorkspace <EventWorkspace>`
input, and will create an EventWorkspace output if a different workspace
is specified.

The main difference vs. using a Workspace2D is that the event lists from
all the incoming pixels are simply appended in the grouped spectra; this
means that you can rebin the resulting spectra to finer bins with no
loss of data. In fact, it is unnecessary to bin your incoming data at
all; binning can be performed as the very last step.

.. |Example of RAW GEM data focused across the 5 detector banks| image:: /images/GEM_Focused.png

.. categories::
