.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Load Ikeda-Carpenter PV parameters from a GSAS instrument file into a table workspace or a workspace group.

Later developments of this algorithm will enable to load back-to-back-exponential pseudo-Voigt parameters translated into
:ref:`BackToBackExponential <func-BackToBackExponential>`.


Usage
-----

**Example - Run LoadGSASInstrumentFile for both TableWorkspace and workspace with Instrument**

.. include:: ../usagedata-note.txt

.. testcode:: ExLoadGSASInstrumentFileSimple

   # We load a GSAS Instrument file with 2 Banks
   tws = LoadGSASInstrumentFile("GSAS_2bank.prm")

   #Print first four rows
   for i in [0,1,2,3]:
      row = tws.row(i)
      print "{'Name': '%s','Value_1': %.2f, 'Value_2': %.2f}" % ( row["Name"], row["Value_1"],  row["Value_2"] )

Output:

.. testoutput:: ExLoadGSASInstrumentFileSimple

   {'Name': 'BANK','Value_1': 1.00, 'Value_2': 2.00}
   {'Name': 'Alph0','Value_1': 0.00, 'Value_2': 0.00}
   {'Name': 'Alph1','Value_1': 0.21, 'Value_2': 0.22}
   {'Name': 'Beta0','Value_1': 31.79, 'Value_2': 31.79}


**Example - Run LoadGSASInstrumentFile and put parameters into workspace**

.. include:: ../usagedata-note.txt

.. testcode:: ExLoadGSASInstrumentFileWorkspace

   # First we load MUSR00015189 workspace group
   Load("MUSR00015189",OutputWorkspace="groupWs")
   groupWs=mtd["groupWs"]
   # We clear instrument parameters...
   ClearInstrumentParameters(groupWs)

   # ...and check they do not exist
   instrument = groupWs.getItem(0).getInstrument()
   print "Alpha0 parameter exists: ",  instrument.hasParameter("Alpha0")
   print "Beta0 parameter exists: ", instrument.hasParameter("Beta0")
   print "SigmaSquared parameter exists: " , instrument.hasParameter("SigmaSquared")

   # Now we load a GSAS Instrument file with 2 Banks into the workspace...
   print "\nLoading parameters from GSAS\n"
   tws = LoadGSASInstrumentFile(Filename="GSAS_2bank.prm",UseBankIDsInFile=True,Workspace=groupWs,Banks=[1,2])

   # ...and check parameters are there again
   instrument = groupWs.getItem(0).getInstrument()
   print "Alpha0 parameter exists: ",  instrument.hasParameter("Alpha0")
   print "Beta0 parameter exists: ", instrument.hasParameter("Beta0")
   print "SigmaSquared parameter exists: " , instrument.hasParameter("SigmaSquared")

Output:

.. testoutput:: ExLoadGSASInstrumentFileWorkspace

   Alpha0 parameter exists:  False
   Beta0 parameter exists:  False
   SigmaSquared parameter exists:  False

   Loading parameters from GSAS

   Alpha0 parameter exists:  True
   Beta0 parameter exists:  True
   SigmaSquared parameter exists:  True

.. categories::
