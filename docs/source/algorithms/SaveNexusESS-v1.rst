
.. algorithm::

.. summary::

.. relatedalgorithms::
:ref:`algm-SaveNexusGeometry`
:ref:`algm-SaveProcessedNexus`
:ref:`algm-LoadInstrumentGeometry`
.. properties::

Description
-----------

Saves a processed nexus file similar to :ref:`algm-SaveProcessedNexus`, but provides nexus geometry which is an accurate snapshot of the calibrated/transformed instrument geometry in-memory. One current major difference between the two algorithms is that SaveNexusESS does not support the generation of a single processed file based on a :ref:`GroupWorkspace <WorkspaceGroup>` input.

This algorithm may be deprecated in future in favour of a master :ref:`algm-SaveProcessedNexus` algorithm.

This algorithm currently provides not shape information for component geometry.

Usage
-----
..  Try not to use files in your examples,
    but if you cannot avoid it then the (small) files must be added to
    autotestdata\UsageData and the following tag unindented
    .. include:: ../usagedata-note.txt

**Example - SaveNexusESS**

.. testcode:: SaveNexusESSExample

   # Create a host workspace
   ws_out = CreateSampleWorkspace()
   SaveNexusESS(Filename="local.nxs", InputWorkspace=ws_out)

   ws_in = Load(Filename="local.nxs")

   comparison = CompareWorkspaces(ws_out, ws_in)

   # Print the result
   print "The output workspace has %%i spectra" %% wsOut.getNumberHistograms()

Output:

.. testoutput:: SaveNexusESSExample

  The output workspace has ?? spectra

.. categories::

.. sourcelink::

