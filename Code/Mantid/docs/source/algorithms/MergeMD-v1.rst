.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

This algorithm merges several :ref:`MDWorkspaces <MDWorkspace>` together
into one by adding their events together.

The algorithm starts by going through the list of
:ref:`MDWorkspaces <MDWorkspace>` to find the extents that fully encompass
all input workspaces in each dimension. The number and names of
dimensions must match for all input workspaces.

The output workspace is created with these dimensions and the box
parameters specified above. Then the events from each input workspace
are appended to the output.

See also: :ref:`algm-MergeMDFiles`, for merging when system
memory is too small to keep the entire workspace in memory.

Usage
-----

**Example - merge two MD workspaces:**

.. testcode:: ExMergeMD

   # Create sample inelastic workspace for MARI instrument containing 1 at all spectra 
   ws1=CreateSimulationWorkspace(Instrument='MAR',BinParams='-10,1,10',UnitX='DeltaE')
   AddSampleLog(ws1,'Ei','12.','Number')
   # get first MD workspace;
   mdWs1 =ConvertToMD(InputWorkspace=ws1,QDimensions='|Q|',QConversionScales='Q in A^-1',MinValues='0,-10',MaxValues='5,10')   
   # Create another inelastic workspace
   ws1=CreateSimulationWorkspace(Instrument='MAR',BinParams='-5,1,15',UnitX='DeltaE')
   AddSampleLog(ws1,'Ei','20.','Number')
   # get second MD workspace;
   mdWs2 =ConvertToMD(InputWorkspace=ws1,QDimensions='|Q|',QConversionScales='Q in A^-1',MinValues='0,-5',MaxValues='10,15')   

   # Merge MD workspaces
   SumWS=MergeMD(InputWorkspaces='mdWs1,mdWs2',SplitInto='100,100')

    # check it looks like the one we wanted
   print 'merged workspace of type : {0}\n'.format(type(SumWS)),
   print '****************************************************************'   
   print 'workspace 1 has {0} dimensions with {1} points and {2} events'.format(mdWs1.getNumDims(),mdWs1.getNPoints(),mdWs1.getNEvents());
   d1=mdWs1.getDimension(0);d2=mdWs1.getDimension(1)
   print 'with d1 min_max={0}:{1}, d2 min_max={2}:{3}'.format(d1.getMinimum(),d1.getMaximum(),d2.getMinimum(),d2.getMaximum())
   print 'workspace 2 has {0} dimensions with {1} points and {2} events'.format(mdWs2.getNumDims(),mdWs2.getNPoints(),mdWs2.getNEvents());   
   d1=mdWs2.getDimension(0);d2=mdWs2.getDimension(1)
   print 'with d1 min_max={0}:{1}, d2 min_max={2}:{3}'.format(d1.getMinimum(),d1.getMaximum(),d2.getMinimum(),d2.getMaximum())
   print '****************************************************************'
   print 'Merged WS has   {0} dimensions with {1} points and {2} events'.format(SumWS.getNumDims(),SumWS.getNPoints(),SumWS.getNEvents());
   d1=SumWS.getDimension(0);d2=SumWS.getDimension(1)
   print 'with d1 min_max={0}:{1}, d2 min_max={2}:{3}'.format(d1.getMinimum(),d1.getMaximum(),d2.getMinimum(),d2.getMaximum())
   print '****************************************************************'   

   
**Output:**

.. testoutput:: ExMergeMD

   merged workspace of type : <class 'mantid.api._api.IMDEventWorkspace'>
   ****************************************************************
   workspace 1 has 2 dimensions with 18231 points and 18231 events
   with d1 min_max=0.0:5.0, d2 min_max=-10.0:10.0
   workspace 2 has 2 dimensions with 15606 points and 15606 events
   with d1 min_max=0.0:10.0, d2 min_max=-5.0:15.0
   ****************************************************************
   Merged WS has   2 dimensions with 33837 points and 33837 events
   with d1 min_max=0.0:10.0, d2 min_max=-10.0:15.0
   ****************************************************************



.. categories::
