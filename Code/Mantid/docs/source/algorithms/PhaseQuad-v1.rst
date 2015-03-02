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

**Example - Computing squashograms:**

.. testcode:: ExCompSquash

   # Load the first two spectra from a MUSR run
   input = LoadMuonNexus('MUSR0015189.nxs',EntryNumber=1,SpectrumMin=1,SpectrumMax=2)

   # Create a PhaseList file with some arbitrary detector information
   file = open('PhaseList.txt','w')
   file.write("MuSR\n")
   file.write("Dummy line\n")
   file.write("Dummy line\n")
   file.write("Dummy line\n")
   file.write("Dummy line\n")
   file.write("2 0 60 0.0\n")
   for i in range(0,2):
	   file.write("1 1.0 0.0 -1 -1 -1\n")
   file.close()

   output = PhaseQuad('input','',60,0,0,'PhaseList.txt')
   print "Counts: ", input[0].readY(0)[24]

Output:

.. testoutput:: ExCompSquash

   Counts:  4.0

.. categories::