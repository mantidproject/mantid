.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This is the specialized loader for the raw `.nxs` files produced by IN16B back-scattering spectrometer at ILL.
This loader can load only a single file at each call. If loading more than one file is required, please refer to
:ref:`Load <algm-Load>` or :ref:`LoadAndMerge <algm-LoadAndMerge>` algorithms, which are more suited to that task.


The loader supports both QENS and FWS types of data, both with or without mirror sense.
The number of active single detectors, and their actual positions are read from the `.nxs` file overriding the ones in the instrument definition.
The output workspace will be dimensionless containing the number of counts per energy channel for each detector pixel.
It will have `empty` x-axis unit.

The `LoadDetectors` allows to choose which data to load for IN16B: those acquired by the spectrometer,
or those from the diffractometer.

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
