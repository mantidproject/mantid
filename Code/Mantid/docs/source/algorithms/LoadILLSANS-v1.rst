.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Loads an ILL D33 NeXus file into a `Workspace2D <http://www.mantidproject.org/Workspace2D>`_ with
the given name.

This loader reads the detector positions from the NeXus file and places all the five detectors (front and back) at the right positions.
It supports both TOF and non TOF modes.

Usage
-----

**Example - Load ILL D33 NeXus file:**

.. code-block:: python

   # Load ILL D33 data file into a workspace 2D.
   ws = Load('ILLD33_sample_001425.nxs')

   print "This workspace has", ws.getNumDims(), "dimensions and has", ws.getNumberHistograms(), "histograms."

Output:
	
	This workspace has 2 dimensions and has 65538 histograms.

.. categories::
