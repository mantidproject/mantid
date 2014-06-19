.. algorithm::

.. summary::

.. alias::

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
   ws1=CreateSimulationWorkspace(Instrument='MAR',BinParams='-10,1,10',UnitX='DeltaE')
   AddSampleLog(ws1,'Ei','12.','Number')
   ws2=ws1*2;
   # Convert to MD
   mdWs1 =ConvertToMD(InputWorkspace=ws1,QDimensions='|Q|',QConversionScales='Q in A^-1',SplitInto='10,10',MaxRecursionDepth='1')

   # get the query
   table = QueryMDWorkspace(InputWorkspace=mdWs1)
   
   # look at the output:
   col_names=table.keys();
   name0=col_names[0];
   nRows = len(table.column(name0));
   print "Table contains {0} rows".format(nRows)
   print "first 11 of them are:"
   print '{0}'.format("--------------------------------------------------------------------------------------------------------------")
   for name in col_names:
      print '| {0:19}'.format(name),
   print '|'
   print '{0}'.format("--------------------------------------------------------------------------------------------------------------")
   for i in xrange(0,11):
     for name in col_names:
        col = table.column(name);
        print '| {0:<19}'.format(col[i]),
     print '|'
    
    
**Output:**

.. testoutput:: ExQueryMDWorkspace

   Table contains 100 rows
   first 11 of them are: 
   --------------------------------------------------------------------------------------------------------------
   | Signal/none         | Error/none          | Number of Events    | |Q|/MomentumTransfer | DeltaE/DeltaE       |
   --------------------------------------------------------------------------------------------------------------
   | 0.0                 | 0.0                 | 0                   | 0.398168385029      | -9.00000190735      |
   | 338.0               | 0.0                 | 338                 | 0.906549930573      | -9.00000190735      |
   | 164.0               | 0.0                 | 164                 | 1.41493153572       | -9.00000190735      |
   | 164.0               | 0.0                 | 164                 | 1.92331314087       | -9.00000190735      |
   | 153.0               | 0.0                 | 153                 | 2.43169498444       | -9.00000190735      |
   | 162.0               | 0.0                 | 162                 | 2.94007635117       | -9.00000190735      |
   | 171.0               | 0.0                 | 171                 | 3.44845819473       | -9.00000190735      |
   | 195.0               | 0.0                 | 195                 | 3.95683956146       | -9.00000190735      |
   | 231.0               | 0.0                 | 231                 | 4.46522140503       | -9.00000190735      |
   | 258.0               | 0.0                 | 258                 | 4.97360277176       | -9.00000190735      |
   | 28.0                | 0.0                 | 28                  | 0.398168385029      | -7.00000143051      |

.. categories::
