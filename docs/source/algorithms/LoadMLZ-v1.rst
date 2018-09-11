.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Loads an MLZ NeXus file into a :ref:`Workspace2D <Workspace2D>`
with the given name.

This algorithm masks detectors using a mask defined in the input Nexus
file.

It also fills the SampleLogs of the Outputworkspace with
data such as the monitor counts and the channel width.

To date this algorithm only supports: TOFTOF.

Usage
-----

**Example - Load TOFTOF Nexus file:**

.. testcode:: ExLoadTOFTOFnexus

   ws = LoadMLZ(Filename='TOFTOFTestdata.nxs')

   print("Name of the instrument:  {}".format(ws.getInstrument().getName()))
   print("Number of spectra:  {}".format(ws.getNumberHistograms()))
   

Output:

.. testoutput:: ExLoadTOFTOFnexus

   Name of the instrument:  TOFTOF
   Number of spectra:  1006

.. categories::

.. sourcelink::
