.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Takes two arrays of signal and error values, as well as information
describing the dimensionality and extents, and creates a
MDHistoWorkspace (histogrammed multi-dimensional workspace). The
*SignalInput* and *ErrorInput* arrays must be of equal length and have a
length that is equal to the product of all the comma separated arguments
provided to **NumberOfBins**. The order in which the arguments are
specified to each of the properties (for those taking multiple
arguments) is important, as they are assumed to match by the order they
are declared. For example, specifying **Names = A,B** and
**Units = U1,U2** will generate two dimensions, the first with a name
of *A* and units of *U1* and the second with a name of *B* and units of
*U2*. The same logic applies to the **Extents** inputs. Signal and Error
inputs are read in such that, the first entries in the file will be
entered across the first dimension specified, and the zeroth index in
the other dimensions. The second set of entries will be entered across
the first dimension and the 1st index in the second dimension, and the
zeroth index in the others.

Alternatives
------------

A very similar algorithm to this is
:ref:`algm-ImportMDHistoWorkspace`, which takes it's
input signal and error values from a text file rather than from arrays.
Another alternative is to use :ref:`algm-ConvertToMD` which works
on MatrixWorkspaces, and allows log values to be included in the
dimensionality.

Usage
-----

**Example - Create MDHistoWorkspace**

.. testcode:: exCreateMDHisto

   S  =range(0,100);
   ERR=range(0,100);   
   # create Histo workspace   
   ws=CreateMDHistoWorkspace(Dimensionality=2,Extents='-3,3,-10,10',SignalInput=S,ErrorInput=ERR,\
                              NumberOfBins='10,10',Names='Dim1,Dim2',Units='MomentumTransfer,EnergyTransfer')

   # check it looks like the one we wanted
   print('created workspace is of type: {0}'.format(ws.id()))
   print('and has {0} dimensions with {1} points and {2} events'.
	 format(ws.getNumDims(),ws.getNPoints(),ws.getNEvents()))
   d1=ws.getDimension(0)
   print('dimension 0 has ID: {0}; nBins={1}; min: {2}; max: {3} in units of: {4}'.
         format(d1.getDimensionId(), d1.getNBins(), d1.getMinimum(), d1.getMaximum(),d1.getUnits()))
   d1=ws.getDimension(1)   
   print('dimension 1 has ID: {0}; nBins={1}; min: {2}; max: {3} in units of: {4}'.
         format(d1.getDimensionId(), d1.getNBins(), d1.getMinimum(), d1.getMaximum(), d1.getUnits()))

  
**Output:**

.. testoutput:: exCreateMDHisto

   created workspace is of type: MDHistoWorkspace
   and has 2 dimensions with 100 points and 0 events 
   dimension 0 has ID: Dim1; nBins=10; min: -3.0; max: 3.0 in units of: MomentumTransfer
   dimension 1 has ID: Dim2; nBins=10; min: -10.0; max: 10.0 in units of: EnergyTransfer

.. categories::

.. sourcelink::
