.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Loads an ILL Reflectometry instrument NeXus file into a `Workspace2D <http://www.mantidproject.org/Workspace2D>`_.

This loader updates the detector position according to NeXus file information.
It supports both TOF and non TOF modes.

Usage
-----

**Example - Load ILL D17 NeXus file:**

.. code-block:: python

   # Load ILL D17 data file into a workspace 2D.
   ws = Load('ILLD17_111686.nxs')

   print("This workspace has {} dimensions (spectra) and has {} histograms.".format(ws.getNumDims(), ws.getNumberHistograms()))

Output:
	
	This workspace has 2 dimensions (spectra) and has 258 histograms.

.. categories::

.. sourcelink::
