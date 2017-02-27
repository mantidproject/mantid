.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

The Load algorithm is a more intelligent algorithm than most other load
algorithms. When passed a filename it attempts to search the existing
load `algorithms <index.html>`__ and find the most appropriate
to load the given file. The specific load algorithm is then run as a
child algorithm with the exception that it logs messages to the Mantid
logger.

Filename Property
#################

The ``Load`` algorithm changes the default ``Filename`` property to a
:py:obj:`MultipleFileProperty <mantid.api.MultipleFileProperty>` and
follows its syntax.

Specific Load Algorithm Properties
##################################

Each specific loader will have its own properties that are appropriate
to it: SpectrumMin and SpectrumMax for ISIS RAW/NeXuS, FilterByTof\_Min
and FilterByTof\_Max for Event data. The Load algorithm cannot know
about these properties until it has been told the filename and found the
correct loader. Once this has happened the properties of the specific
Load algorithm are redeclared on to that copy of Load.

Loading Nexus files
###################

When the file to be loaded is a :ref:`Nexus file<Nexus file>`,
the type nexus file loaded is determined by its group structure.

If the nexus file has a group of type ``NXevent_data``,
then :ref:`algm-LoadEventNexus` will be run.
Else if the nexus file has a ``/raw_data_1`` path,
then :ref:`algm-LoadISISNexus` will be run
and it will only load data within the group with this pathname.
Also a nexus file with certain groups present will be
loaded by :ref:`algm-LoadMuonNexus`.
A nexus file with a group of path ``/mantid_workspace_1`` is
loaded by :ref:`algm-LoadNexusProcessed`.
See the specific load algorithms for more details.



Usage
-----

.. include:: ../usagedata-note.txt

**Example - Load ISIS histogram Nexus file:**
(see :ref:`algm-LoadISISNexus` for more options)

.. testcode:: ExLoadISISnexusHist

   # Load ISIS LOQ histogram dataset
   ws = Load('LOQ49886.nxs')

   print "The 1st x-value of the first spectrum is: " + str(ws.readX(0)[0])

Output:

.. testoutput:: ExLoadISISnexusHist

   The 1st x-value of the first spectrum is: 5.0

**Example - Load SNS/ISIS event Nexus file:**
(see :ref:`algm-LoadEventNexus` for more options)

.. testcode:: ExLoadEventNexus

   # Load SNS HYS event dataset
   ws = Load('HYS_11092_event.nxs')

   print "The number of histograms (spectra) is: " + str(ws.getNumberHistograms())

Output:

.. testoutput:: ExLoadEventNexus

   The number of histograms (spectra) is: 20480

**Example - Load ISIS Muon file:**
(see :ref:`algm-LoadMuonNexus` for more options)

.. testcode:: ExLoadISISMuon

   # Load ISIS multiperiod muon MUSR dataset
   ws = Load('MUSR00015189.nxs')

   print "The number of periods (entries) is: " + str(ws[0].getNumberOfEntries())

Output:

.. testoutput:: ExLoadISISMuon

   The number of periods (entries) is: 2

**Example - Load Mantid processed Nexus file ISIS:**
(see :ref:`algm-LoadNexusProcessed` for more options)

.. testcode:: ExLoadNexusProcessedWithLoad

   # Load Mantid processed GEM data file
   ws = Load('focussed.nxs')

   print "The number of histograms (spectra) is: " + str(ws.getNumberHistograms())

Output:

.. testoutput:: ExLoadNexusProcessedWithLoad

   The number of histograms (spectra) is: 6

.. categories::

.. sourcelink::
