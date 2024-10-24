.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Load data from the EMU beamline at ANSTO. The workspace generated is a TOF EventWorkspace.

Usage
-----

.. include:: ../usagedata-note.txt

**Example - Load an EMU dataset:**

.. testcode:: ExSimple

   ws = LoadEMU('EMU0006330.tar');

   print("Number of spectra: {}".format(ws.getNumberHistograms()))

Output:

.. testoutput:: ExSimple

   Number of spectra: 6592


.. categories::

.. sourcelink::
