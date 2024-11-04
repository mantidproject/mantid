.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Load data from the Pelican beamline at ANSTO. The workspace generated is a TOF EventWorkspace.

Usage
-----

.. include:: ../usagedata-note.txt

**Example - Load a Pelican dataset:**

.. testcode:: ExSimple

   ws =  LoadPLN('PLN0044464.hdf', BinaryEventPath='./PLN0044464.bin')

   print("Number of spectra: {}".format(ws.getNumberHistograms()))

Output:

.. testoutput:: ExSimple

   Number of spectra: 12808


.. categories::

.. sourcelink::
