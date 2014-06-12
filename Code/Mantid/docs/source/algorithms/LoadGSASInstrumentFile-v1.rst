.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Load parameters from a GSAS instrument file into a table workspace

Later developments of this algorithm will enable these parameters to be
put into the instrument of a wotrkspace for either Ikeda-Carpender
pseudo-Voigt translated into `IkedaCarpenterPV <IkedaCarpenterPV>`__ or
back-to-back-exponential pseudo-Voigt translated into
`BackToBackExponential <BackToBackExponential>`__.


Usage
-----

** Example - Run LoadGSASInstrumentFile for both TableWorkspace and workspace with XX Instrument **

.. testcode:: ExLoadGSASInstrumentFileSimple

   # We load a GSAS Instrument file with 2 Banks
   # which will suit MUSR00015189 at a later stage of devolpment

   tws = LoadGSASInstrumentFile("GSAS_2bank.prm")

   #Print first four rows
   for i in [0,1,2,3]:
      row = tws.row(i)
      print "{'Value_1': %.2f, 'Name': '%s', 'Value_2': %.2f}" % ( row["Value_1"], row["Name"], row["Value_2"] )

Output:

.. testoutput:: ExLoadGSASInstrumentFileSimple

   {'Value_1': 1.00, 'Name': 'BANK', 'Value_2': 2.00}
   {'Value_1': 0.00, 'Name': 'Alph0', 'Value_2': 0.00}
   {'Value_1': 0.21, 'Name': 'Alph1', 'Value_2': 0.22}
   {'Value_1': 31.79, 'Name': 'Beta0', 'Value_2': 31.79}

.. categories::
