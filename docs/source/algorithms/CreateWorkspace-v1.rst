.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm constructs a :ref:`MatrixWorkspace <MatrixWorkspace>`
when passed a vector for each of the X, Y and optionally E and Dx values.
The E values of the output workspace will be zero if not provided.
The unit for the X Axis can optionally be specified as any of the units in the
Mantid `Unit Factory <http://www.mantidproject.org/Units>`__ (see `the
list of units currently available
<http://www.mantidproject.org/Units>`__).  Multiple spectra may be
created by supplying the NSpec Property (integer, default 1). When
this is provided the vectors are split into equal-sized spectra (all
X, Y, E, Dx values must still be in a single vector for input).

When you use the input property ParentWorkspace, the new workspace is
created with the same instrument (including its parameters), sample
and run information, and comment field as the parent. The Y units and
title are also copied from the parent workspace unless they are
provided in the input properties passed to the algorithm.

Note on old behavior regarding spectrum-detector mapping
########################################################

Until release 3.11 (inclusive) this algorithm created a default (and potentially wrong) mapping from spectra to detectors if no parent workspace was given.
This change has no effect if any of the following applies:

- A parent workspace is given.
- No instrument is loaded into to workspace at a later point.
- An instrument is loaded at a later point but ``LoadInstrument`` is used with ``RewriteSpectraMapping=True``.
- An instrument is loaded at a later point but ``LoadInstrument`` is used with ``RewriteSpectraMapping=False`` but a correct mapping is set up before or after loading the instrument.

That is, only the following use is affected:

.. code-block:: python

     # See usage example below for setting up input data.
     ws = CreateWorkspace(DataX=dataX, DataY=dataY, DataE=dataE, NSpec=4, UnitX="Wavelength")
     LoadInstrument(ws, InstrumentName='MARI', RewriteSpectraMap=False)
     # None of the spectra in `ws` maps to any detectors since detectors were not known
     # at the time of workspace creation. The old behavior used to map spectra 1,2,3,4
     # to detector IDs 1,2,3,4. This worked only accidentally if MARI contains detectors
     # with IDs 1,2,3,4. Accidental mappings are not a feature anymore.

The solution to this is to set ``RewriteSpectraMapping=True``, set the correct mapping by hand via ``ws.getSpectrum(i).setDetectorID(id)``, or use ``CreateWorkspace`` with a parent workspace that contains the correct instrument and mapping.

Usage
-----

.. testcode::

     dataX = [1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16]
     dataY = [1,2,3,4,5,6,7,8,9,10,11,12]

     # The workspace will be named "dataWS1", error values will be zero.
     dataWS1 = CreateWorkspace(DataX=dataX, DataY=dataY, NSpec=4, UnitX="Wavelength")

     # Create a workspace containing the following error values:
     dataE = [1,2,3,4,5,6,7,8,9,10,11,12]
     dataWS2 = CreateWorkspace(DataX=dataX, DataY=dataY, DataE=dataE, NSpec=4, UnitX="Wavelength")

     # Create a workspace containing Dx values:
     dX = [1,2,3,4,5,6,7,8,9,10,11,12]
     dataWS3 = CreateWorkspace(DataX=dataX, DataY=dataY, DataE=dataE, NSpec=4, UnitX="Wavelength", Dx=dX)

.. categories::

.. sourcelink::
