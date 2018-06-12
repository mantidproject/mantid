.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm loads a `.nxs` file produced by IN16B back-scattering spectrometer at ILL.
It supports both QENS and FWS types of data, both with or without mirror sense.
The number of active single detectors, and their actual positions are read from the `.nxs` file overriding the ones in the instrument definition.
The output workspace will be dimensionless containing the number of counts per energy channel for each detector pixel.
It will have `empty` x-axis unit.

Usage
-----

**Example - Load ILL IN16B NeXus file:**

.. testcode:: python

   # Load ILL IN16B data file into a workspace 2D.
   ws = LoadILLIndirect('ILL/IN16B/136555.nxs')

   print("ws has {0} spectra and {1} bins".format(ws.getNumberHistograms(),ws.blocksize()))

.. testoutput:: python

	ws has 2051 spectra and 2048 bins

.. testcleanup:: python

    DeleteWorkspace('ws')

.. categories::

.. sourcelink::
