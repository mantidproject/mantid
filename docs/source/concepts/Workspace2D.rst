.. _Workspace2D:

===========
Workspace2D
===========

.. contents::
  :local:

The Workspace2D is a Mantid data type for a
:ref:`MatrixWorkspace <MatrixWorkspace>`.

It consists of a workspace with 1 or more spectra. Typically, each
spectrum will be a histogram. For example, you might have 10 bins, and
so have 11 X-value, 10 Y-values and 10 E-values in a workspace.

In contrast to an :ref:`EventWorkspace <EventWorkspace>`, a Workspace2D
only contains bin information and does not contain the underlying event
data. The :ref:`EventWorkspace <EventWorkspace>` presents itself as a
histogram (with X,Y,E values) but preserves the underlying event data.

For more information on what a Workspace2D contains, see 
:ref:`MatrixWorkspace <MatrixWorkspace>`.

There is also a specialised form of Workspace2D, called a RebinnedOutput
which is produced by :ref:`algm-SofQWNormalisedPolygon`. In addition
to the signal Y and error E values for each bin, it also carries the
fractional weight F of the signal in the bin. This is needed to keep
track of the portions of counts from bins in the original grid which has
been put into the new (rebinned) grid. A more detailed explanation is
given in the description of :ref:`algm-SofQWNormalisedPolygon`. 
For correct handling of the fractional weights in rebinning, the user
is recommended to use the :ref:`algm-Rebin2D` algorithm in preference
to :ref:`algm-Integration` or :ref:`algm-SumSpectra` although the other
algorithms do account for the fractional weights.

.. warning:: Note, however, that all binary and unary operations on
   workspaces will ignore the fractional weights. Thus it is important
   to handle all background subtractions and scaling in the original
   reduced dataset(s) **before** conversion using
   :ref:`algm-SofQWNormalisedPolygon`.

Working with Workspace2Ds in Python
-----------------------------------

Workspace2D is a :ref:`MatrixWorkspace <MatrixWorkspace>` and does not offer any functionality above that of a Matrix Workspace.

Accessing Workspaces
####################

The methods for getting a variable to an EventWorkspace is the same as shown in the :ref:`Workspace <Workspace-Accessing_Workspaces>` help page.

If you want to check if a variable points to something that is a Workspace2D you can use this:

.. testcode:: CheckWorkspace2D

    histoWS = CreateSampleWorkspace()

    if histoWS.id() == "Workspace2D":
        print(histoWS.name() + " is an " + histoWS.id())

Output:

.. testoutput:: CheckWorkspace2D
    :options: +NORMALIZE_WHITESPACE

    histoWS is an Workspace2D

.. include:: WorkspaceNavigation.txt

Pickling Workspaces
###################

A Workspace2D may be `pickled <https://docs.python.org/2/library/pickle.html/>` and de-pickled in python. The current pickling process has the following limitations to beware.

- Only Workspace2D objects can be pickled and de-pickled. Other :ref:`MatrixWorkspace <MatrixWorkspace>` subtypes cannot be pickled
- Meta-data such as sample logs are not pickled
- Masking flags are not pickled
- Scanning Workspace2D objects are not permitted for pickling 
- Workspace2D objects are always converted into :ref:`histograms <HistogramData>` formed as bin edges and counts as part of the pickling process.

In addition, users should prefer using cPickle over pickle, and make sure that the protocol option is set to the HIGHEST_PROTOCOL to ensure that the serialization/deserialization process is as fast as possible.

.. code-block:: python   

  import cPickle as pickle
  pickled = pickle.dumps(ws2d, pickle.HIGHEST_PROTOCOL)

.. categories:: Concepts
