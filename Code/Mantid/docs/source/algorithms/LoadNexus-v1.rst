.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

The algorithm LoadNexus will read the given Nexus file and try to
identify its type so that it can be read into a workspace. The file name
can be an absolute or relative path and should have the extension .nxs
or .nx5. Currently only Nexus Muon Version 1 files are recognised, but
this will be extended as other types are supported such as
:ref:`algm-LoadNexusProcessed`.

If the file contains data for more than one period, a separate workspace
will be generated for each. After the first period the workspace names
will have "\_2", "\_3", and so on, appended to the given workspace name.
For single period data, the optional parameters can be used to control
which spectra are loaded into the workspace. If spectrum\_min and
spectrum\_max are given, then only that range to data will be loaded. If
a spectrum\_list is given than those values will be loaded.

Usage
-----

.. include:: ../usagedata-note.txt

**Example - Load ISIS histogram Nexus file:**
(see :ref:`algm-LoadISISNexus` for more options)

.. testcode:: ExLoadISISnexus

   # Load LOQ histogram dataset
   ws = LoadNexus('LOQ49886.nxs')

   print "The 1st x-value of the first spectrum is: " + str(ws.readX(0)[0])

Output:

.. testoutput:: ExLoadISISnexus

   The 1st x-value of the first spectrum is: 5.0

**Example - Load ISIS Muon file:**
(see :ref:`algm-LoadMuonNexus` for more options)

.. testcode:: ExLoadISISMuon

   # Load ISIS multiperiod muon MUSR dataset
   ws = LoadNexus('MUSR00015189.nxs')

   print "The number of periods (entries) is: " + str(ws[0].getNumberOfEntries())

Output:

.. testoutput:: ExLoadISISMuon

   The number of periods (entries) is: 2

**Example - Load Mantid processed Nexus file ISIS:**
(see :ref:`algm-LoadNexusProcessed` for more options:

.. testcode:: ExLoadNexusProcessedWithLoadNexus

   # Load Mantid processed GEM data file
   ws = LoadNexus('focussed.nxs')

   print "The number of histograms (spectra) is: " + str(ws.getNumberHistograms())

Output:

.. testoutput:: ExLoadNexusProcessedWithLoadNexus

   The number of histograms (spectra) is: 6


.. categories::
