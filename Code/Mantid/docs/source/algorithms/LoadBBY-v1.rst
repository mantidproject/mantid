.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Load data from the Bilby beamline at ANSTO. The workspace generated is a TOF EventWorkspace.

.. categories::

Usage
-----

.. include:: ../usagedata-note.txt

**Example - Load a Bilby dataset:**

.. testcode:: ExSimple

   ws = LoadBBY('BBY0000014.tar');

   print "Number of spectra:", ws.getNumberHistograms()

Output:

.. testoutput:: ExSimple

   Number of spectra: 61440
