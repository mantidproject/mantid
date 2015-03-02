.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Assuming that the *InputWorkspace* contains measured 
counts as a function of time, the algorithm returns a workspace 
containing two spectra (squashograms) as a function of the same time binning.
Either *PhaseList* or *PhaseTable* must be provided. Note: The first column is ignored in both cases.

*PhaseList* is expected to have a six-row header, including overall information, and 6 columns:

1. Boolean type, containing if detector is OK
2. Double type, containing detector asymmetry
3. Double type, containing detector phase
4. Double type, containing detector lag
5. Double type, containing detector deadtime :math:`c`
6. Double type, containing detector deadtime :math:`m`

*PhaseTable* is expected to have four columns:

1. Boolean type, containing if detector is OK
2. Double type, containing detector asymmetry
3. Double type, containing detector phase
4. Double type, containing detector deadtime

Usage
-----

.. include:: ../usagedata-note.txt

**Example - Computing squashograms from PhaseList:**

.. testcode:: ExPhaseQuadList

   # Load a set of spectra from a EMU file
   ws = LoadMuonNexus('EMU00006473.nxs')

   # Create a PhaseList file with some arbitrary detector information
   import os
   phaselist_path = os.path.join(os.path.expanduser("~"),"PhaseList.txt")
   phaselist_file = open(phaselist_path,'w')
   phaselist_file.write("MuSR\n")
   phaselist_file.write("Dummy line\n")
   phaselist_file.write("Dummy line\n")
   phaselist_file.write("Dummy line\n")
   phaselist_file.write("Dummy line\n")
   phaselist_file.write("32 0 60 0.0\n")
   for i in range(0,16):
        phaselist_file.write("1 50.0 0.00 0 0 1\n")
        phaselist_file.write("1 50.0 1.57 0 0 1\n")
   phaselist_file.close()

   ows = PhaseQuad(InputWorkspace='ws',PhaseList=phaselist_path)
   print "Output workspace contains", ows.getNumberHistograms(), "histograms"

Output:

.. testoutput:: ExPhaseQuadList

   Output workspace contains 2 histograms

.. testcleanup:: ExPhaseQuadList

   import os
   try:
       os.remove(phaselist_path)
   except OSError:
       pass

**Example - Computing squashograms from PhaseTable:**

.. testcode:: ExPhaseQuadTable

   # Load a set of spectra from a EMU file
   ws = LoadMuonNexus('EMU00006473.nxs')

   # Create a PhaseTable with some arbitrary detector information
   tab = CreateEmptyTableWorkspace()
   tab.addColumn('bool', 'Status')
   tab.addColumn('double', 'Asymmetry')
   tab.addColumn('double', 'Phase')
   tab.addColumn('double', 'DeadTime')
   for i in range(0,16):
      tab.addRow([1, 50.0, 0.00, 0])
      tab.addRow([1, 50.0, 1.57, 0])

   ows = PhaseQuad(InputWorkspace='ws',PhaseTable='tab')
   print "Output workspace contains", ows.getNumberHistograms(), "histograms"

Output:

.. testoutput:: ExPhaseQuadTable

   Output workspace contains 2 histograms

.. categories::