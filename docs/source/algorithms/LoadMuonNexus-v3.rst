.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

The algorithm LoadMuonNexus will read a Muon Nexus data file and place
the data into the named workspace. The file name can be an absolute or
relative path and should have the extension .nxs or .NXS.

v3 of this algoirthm acts as an algorithm selector. Given the input filename,
the confidence of the following loaders will be assessed and the loader with the
highest confidence selected:
- :ref:`algm-LoadMuonNexus-v1`
- :ref:`algm-LoadMuonNexus-v2`
- :ref:`algm-LoadMuonNexusV2-v1`

This algorithm has the same input properties as :ref:`algm-LoadMuonNexus-v2`, some
algorithm properties only apply to :ref:`algm-LoadMuonNexus-v1`.

Please refer to the documentation for the individual algorithms for implementation detail.

Usage
-----

.. include:: ../usagedata-note.txt

**Example - Load ISIS muon MUSR dataset:**

.. testcode:: LoadMuonNexusOnePeriod

   # Load MUSR dataset
   ws = Load(Filename="MUSR00015189.nxs",EntryNumber=1)
   print("Workspace has  {}  spectra".format(ws[0].getNumberHistograms()))

Output:

.. testoutput:: LoadMuonNexusOnePeriod

   Workspace has  64  spectra

**Example - Load event nexus file with time filtering:**

.. testcode:: ExLoadMuonNexusSomeSpectra

   # Load some spectra
   ws = Load(Filename="MUSR00015189.nxs",SpectrumMin=5,SpectrumMax=10,EntryNumber=1)
   print("Workspace has  {}  spectra".format(ws[0].getNumberHistograms()))

Output:

.. testoutput:: ExLoadMuonNexusSomeSpectra

   Workspace has  6  spectra

**Example - Load dead times into table:**

.. testcode:: ExLoadDeadTimeTable

   # Load some spectra
   ws = Load(Filename="emu00006473.nxs",SpectrumMin=5,SpectrumMax=10,DeadTimeTable="deadTimeTable")
   tab = mtd['deadTimeTable']
   for i in range(0,tab.rowCount()):
       print("{} {:.12f}".format(tab.cell(i,0), tab.cell(i,1)))

Output:

.. testoutput:: ExLoadDeadTimeTable

   5 0.001611122512
   6 0.002150168177
   7 0.010217159986
   8 0.004316862207
   9 0.007436056621
   10 0.004211476538


.. categories::

.. sourcelink::
