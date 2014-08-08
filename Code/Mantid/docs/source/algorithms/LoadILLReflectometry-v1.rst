.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Loads an ILL Reflectometry instrument NeXus file into a `Workspace2D <http://www.mantidproject.org/Workspace2D>`_ with
the given name.

This loader reads the detector position from the NeXus file and places it at the right position.
It supports both TOF and non TOF modes.

To date this Loader only supports D17 data.

Usage
-----

**Example - Load ILL D17 NeXus file:**

.. code-block:: python

   # Load ILL D17 data file into a workspace 2D.
   ws = Load('ILLD17_111686.nxs')

   print "This workspace has", ws.getNumDims(), "dimensions and has", ws.getNumberHistograms(), "histograms."

Output:
	
	This workspace has 2 dimensions and has 258 histograms.

.. categories::
