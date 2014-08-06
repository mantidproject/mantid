.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Load parameters from a GSAS instrument file into a table workspace

Later developments of this algorithm will enable these parameters to be
put into the instrument of a wotrkspace for either Ikeda-Carpender
pseudo-Voigt translated into :ref:`IkedaCarpenterPV <func-IkedaCarpenterPV>` or
back-to-back-exponential pseudo-Voigt translated into
:ref:`BackToBackExponential <func-BackToBackExponential>`.


Usage
-----

**Example - Run LoadGSASInstrumentFile for both TableWorkspace and workspace with Instrument**

.. include:: ../usagedata-note.txt

.. testcode:: ExLoadGSASInstrumentFileSimple

   # We load a GSAS Instrument file with 2 Banks
   # which will suit MUSR00015189 at a later stage of devolpment

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

.. categories::
