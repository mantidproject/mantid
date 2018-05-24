.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm outputs a table workspace containing summary data about
each box within an IMDWorkspace. The table workspace can be used as a
basis for plotting within MantidPlot.

Format
------

-  Column 1: Signal (double)
-  Column 2: Error (double)
-  Column 3: Number of Events (integer)
-  Column 4: Coords of box center (string)

Usage
-----

**Example - query MD workspace and look at the result:**

.. testcode:: ExQueryMDWorkspace

   # create sample inelastic workspace for MARI instrument containing 1 at all spectra 
   ws1 = CreateSimulationWorkspace(Instrument='MAR', BinParams='-10,1,10', UnitX='DeltaE')
   AddSampleLog(ws1, 'Ei', '12.', 'Number')
   ws2 = ws1 * 2;
   # Convert to MD
   mdWs1 = ConvertToMD(InputWorkspace=ws1, QDimensions='|Q|', QConversionScales='Q in A^-1', SplitInto='10,10', MaxRecursionDepth='1')

   # get the query
   table = QueryMDWorkspace(InputWorkspace=mdWs1)
   
   # look at the output:
   col_names = table.keys();
   name0 = col_names[0];
   nRows = len(table.column(name0));
   print("Table contains {0} rows".format(nRows))
   print("first 11 of them are:")
   print("--------------------------------------------------------------------------------------------------------------")
   print(' '.join('| {0:19}'.format(name) for name in col_names) + ' |')
	      
   print("--------------------------------------------------------------------------------------------------------------")
   for i in range(0,11):
      print(' '.join('| {0:>19.4f}'.format(table.column(name)[i]) for name in col_names) + ' |')
    
    
**Output:**

.. testoutput:: ExQueryMDWorkspace

   Table contains 100 rows
   first 11 of them are: 
   --------------------------------------------------------------------------------------------------------------
   | Signal/none         | Error/none          | Number of Events    | |Q|/MomentumTransfer | DeltaE/DeltaE       |
   --------------------------------------------------------------------------------------------------------------
   |              0.0000 |              0.0000 |              0.0000 |              0.3982 |             -9.0000 |
   |            338.0000 |              0.0000 |            338.0000 |              0.9065 |             -9.0000 |
   |            164.0000 |              0.0000 |            164.0000 |              1.4149 |             -9.0000 |
   |            164.0000 |              0.0000 |            164.0000 |              1.9233 |             -9.0000 |
   |            153.0000 |              0.0000 |            153.0000 |              2.4317 |             -9.0000 |
   |            162.0000 |              0.0000 |            162.0000 |              2.9401 |             -9.0000 |
   |            171.0000 |              0.0000 |            171.0000 |              3.4485 |             -9.0000 |
   |            195.0000 |              0.0000 |            195.0000 |              3.9568 |             -9.0000 |
   |            231.0000 |              0.0000 |            231.0000 |              4.4652 |             -9.0000 |
   |            258.0000 |              0.0000 |            258.0000 |              4.9736 |             -9.0000 |
   |             28.0000 |              0.0000 |             28.0000 |              0.3982 |             -7.0000 |

.. categories::

.. sourcelink::
