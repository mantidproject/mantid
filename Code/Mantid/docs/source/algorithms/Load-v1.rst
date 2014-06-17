.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

The Load algorithm is a more intelligent algorithm than most other load
algorithms. When passed a filename it attempts to search the existing
load `algorithms <:Category:Algorithms>`__ and find the most appropriate
to load the given file. The specific load algorithm is then run as a
child algorithm with the exception that it logs messages to the Mantid
logger.

Specific Load Algorithm Properties
##################################

Each specific loader will have its own properties that are appropriate
to it: SpectrumMin and SpectrumMax for ISIS RAW/NeXuS, FilterByTof\_Min
and FilterByTof\_Max for Event data. The Load algorithm cannot know
about these properties until it has been told the filename and found the
correct loader. Once this has happened the properties of the specific
Load algorithm are redeclared on to that copy of Load.

Usage
-----

.. include:: ../usagedata-note.txt

**Example - Load ISIS histogram Nexus file:**
(see `LoadISISNexus <http://www.mantidproject.org/LoadISISNexus>`_ for more options)

.. testcode:: ExLoadISISnexusHist

   # Load ISIS LOQ histogram dataset
   ws = Load('LOQ49886.nxs')

   print "The 1st x-value of the first spectrum is: " + str(ws.readX(0)[0])

Output:

.. testoutput:: ExLoadISISnexusHist

   The 1st x-value of the first spectrum is: 5.0

**Example - Load SNS/ISIS event Nexus file:**
(see `LoadEventNexus <http://www.mantidproject.org/LoadEventNexus>`_ for more options)

.. testcode:: ExLoadEventNexus

   # Load SNS HYS event dataset
   ws = Load('HYS_11092_event.nxs')

   print "The number of histograms (spectra) is: " + str(ws.getNumberHistograms())

Output:

.. testoutput:: ExLoadEventNexus

   The number of histograms (spectra) is: 20480

**Example - Load ISIS Muon file:**
(see `LoadMuonNexus <http://www.mantidproject.org/LoadMuonNexus>`_ for more options)

.. testcode:: ExLoadISISMuon

   # Load ISIS multiperiod muon MUSR dataset
   ws = Load('MUSR00015189.nxs')

   print "The number of periods (entries) is: " + str(ws[0].getNumberOfEntries())

Output:

.. testoutput:: ExLoadISISMuon

   The number of periods (entries) is: 2

**Example - Load Mantid processed Nexus file ISIS:**
(see `LoadNexusProcessed <http://www.mantidproject.org/LoadNexusProcessed>`_ for more options)

.. testcode:: ExLoadNexusProcessedWithLoad

   # Load Mantid processed GEM data file
   ws = Load('focussed.nxs')

   print "The number of histograms (spectra) is: " + str(ws.getNumberHistograms())

Output:

.. testoutput:: ExLoadNexusProcessedWithLoad

   The number of histograms (spectra) is: 6

.. categories::
