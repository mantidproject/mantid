.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Load data from the Pelican beamline at ANSTO using the AESS framework. The workspace generated is a TOF EventWorkspace.

Usage
-----

.. include:: ../usagedata-note.txt

**Example - Load a Pelican AESS dataset:**

.. testcode:: ExSimple

   ws =  LoadPLNnxs('PLN0136193.nxs')

   print("Number of spectra: {}".format(ws.getNumberHistograms()))

Output:

.. testoutput:: ExSimple

   Number of spectra: 12808


.. categories::

.. sourcelink::
