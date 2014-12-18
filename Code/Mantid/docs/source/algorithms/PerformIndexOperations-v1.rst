.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

This algorithm can be used to arrange the spectra of a workspace. It can add two spectra and to remove spectra or do a combination of these.
The processing instructions give the details of this.

Instructions
############

The processing instructions consist of a list of numbers that refer to spectra 0 to n-1 and various operators ',',':','+' and '-'.

To remove spectra, list those you want to keep. The ':' symbol indicates a continuous range of spectra, sparing you the need to list every spectrum.
For example if you have 100 spectra (0 to 99) and want to remove the first and last 10 spectra along with spectrum 12, 
you would use processing instructions '10,13:89'. This says keep spectrum 10 along with spectra 13 to 89 inclusive.

To add spectra, use '+' to add two spectra or '-' to add a range. For example you may with to add spectrum 10 to 12 and ignore the rest, you would use '10+12'.
If you were adding five groups of 20, you would use '0-19,20-39,40-59,60-79,80-99'.

One could combine the two, for example '10+12,13:89' would list the sum of spectra 10 and 12 followed by spectra 13 to 89.

Usage
-----

**Example - Get Sum of First Two spectra followed by Last Two spectra**

.. testcode:: ExPerformIndexOperationsSimple

   # Create Workspace of 5 spectra each with two entries.
   ws = CreateWorkspace(DataX=[1,2], DataY=[11,12,21,22,31,32,41,42,51,52], NSpec=5)

   # Run algorithm adding first two spectra and then keeping last two spectra
   ws2 = PerformIndexOperations( ws, "0+1,3,4")

   #print result
   print ws2.readY(0)
   print ws2.readY(1)
   print ws2.readY(2)
   
Output:

.. testoutput:: ExPerformIndexOperationsSimple

   [ 32.  34.]
   [ 41.  42.]
   [ 51.  52.]

.. categories::
