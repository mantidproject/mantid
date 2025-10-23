.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Load timestamped data from the Bilby beamline at ANSTO. The workspace generated is a TOF EventWorkspace.

.. categories::
   DataHandling, ANSTO

.. sourcelink::

Usage
-----

.. include:: ../usagedata-note.txt

**Example - Load a Bilby dataset:**

.. testcode:: ExSimple

   ws = LoadBBY2('BBY0081723.nxs')

   print("Number of spectra: {}".format(ws.getNumberHistograms()))

Output:

.. testoutput:: ExSimple

   Number of spectra: 61440
