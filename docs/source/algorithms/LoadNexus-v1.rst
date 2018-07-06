.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

The algorithm LoadNexus will read the given Nexus file and try to
identify its type so that it can be read into a workspace. The file name
can be an absolute or relative path and should have the extension .nxs
or .nx5.

The type of Nexus file is identified as follows:

* If the file has a group of class ``SDS`` of name ``"definition"`` or ``"analysis"`` 
  with value ``"muonTD"`` or ``"pulsedTD"``, 
  then it is taken to be a muon Nexus file and :ref:`algm-LoadMuonNexus` is called.

* Else if main entry is ``"mantid_workspace_1"``
  then it is taken to be a processed Nexus file and :ref:`algm-LoadNexusProcessed` is called.
  *The spectrum properties are ignored in this case.*

* Else if main entry is ``"raw_data_1"``
  then it is taken to be an ISIS Nexus file and :ref:`algm-LoadISISNexus` is called.

* Else if instrument group has a ``"SNSdetector_calibration_id"`` item, 
  then :ref:`algm-LoadTOFRawNexus` is called.

* Else exception indicating unsupported type of Nexus file is thrown. 

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

   print("The 1st x-value of the first spectrum is: {}".format(ws.readX(0)[0]))

Output:

.. testoutput:: ExLoadISISnexus

   The 1st x-value of the first spectrum is: 5.0

**Example - Load ISIS Muon file:**
(see :ref:`algm-LoadMuonNexus` for more options)

.. testcode:: ExLoadISISMuon

   # Load ISIS multiperiod muon MUSR dataset
   ws = LoadNexus('MUSR00015189.nxs')

   print("The number of periods (entries) is: {}".format(ws[0].getNumberOfEntries()))

Output:

.. testoutput:: ExLoadISISMuon

   The number of periods (entries) is: 2

**Example - Load Mantid processed Nexus file ISIS:**
(see :ref:`algm-LoadNexusProcessed` for more options:

.. testcode:: ExLoadNexusProcessedWithLoadNexus

   # Load Mantid processed GEM data file
   ws = LoadNexus('focussed.nxs')

   print("The number of histograms (spectra) is: {}".format(ws.getNumberHistograms()))

Output:

.. testoutput:: ExLoadNexusProcessedWithLoadNexus

   The number of histograms (spectra) is: 6


.. categories::

.. sourcelink::
