.. _MDHistoWorkspace:

======================
MD Histogram Workspace
======================

.. contents::
  :local:

The MD Histogram Workspace[MDHistoWorkspace] is a simple multi-dimensional workspace. In
contrast to the :ref:`MDWorkspace <MDWorkspace>`, which contains
points in space, the MDHistoWorkspace consists of a signal and error
spread around space on a regular grid.

In a way, the MDHistoWorkspace is to a
:ref:`MDWorkspace <MDWorkspace>` is what the
:ref:`Workspace2D <Workspace2D>` is to the
:ref:`EventWorkspace <EventWorkspace>`.

Creating a MDHistoWorkspace
---------------------------

MDHistoWorkspaces typically have 3 or 4 dimensions, although they can be
created in up to 9 dimensions.

-  You can bin a :ref:`MDWorkspace <MDWorkspace>` to a
   MDHistoWorkspace using the :ref:`BinMD <algm-BinMD>` algorithm.

   -  You can use :ref:`CreateMDWorkspace <algm-CreateMDWorkspace>` to create a
      blank MDWorkspace first, if you do not have data to bin.

-  Paraview and the `Vates Simple
   Interface <http://www.mantidproject.org/VatesSimpleInterface>`__ will create a MDHistoWorkspace
   from a :ref:`MDWorkspace <MDWorkspace>` when rebinning on a regular
   grid.

Viewing a MDHistoWorkspace
--------------------------

-  MDHistoWorkspaces can be created and visualized directly within
   Paraview and the `Vates Simple
   Interface <http://www.mantidproject.org/VatesSimpleInterface>`__ when rebinning along a regular
   grid.
-  You can right-click on the workspace and select:

   -  **Plot MD**: to perform a 1D plot of the signal in the workspace
      (only works on 1D MDHistoWorkspaces).
   -  **Show Slice Viewer**: to open the `Slice
      Viewer <http://www.mantidproject.org/MantidPlot:_SliceViewer>`__, which shows 2D slices of the
      multiple-dimension workspace.


Working with MD Histo Workspaces in Python
------------------------------------------

Accessing Workspaces
####################

The methods for getting a variable to an MDHistoWorkspace is the same as shown in the :ref:`Workspace <Workspace-Accessing_Workspaces>` help page.

If you want to check if a variable points to something that is an MDHistoWorkspace you can use this:

.. testcode:: CheckMDHistoWorkspace

   from mantid.api import IMDHistoWorkspace

   ws=CreateMDHistoWorkspace(Dimensionality=2,Extents='-3,3,-10,10', \
                           SignalInput=range(0,100),ErrorInput=range(0,100),\
                           NumberOfBins='10,10',Names='Dim1,Dim2',Units='MomentumTransfer,EnergyTransfer')

   if isinstance(ws, IMDHistoWorkspace):
    print ws.name() + " is a " + ws.id()

Output:

.. testoutput:: CheckMDHistoWorkspace
    :options: +NORMALIZE_WHITESPACE

    ws is a MDHistoWorkspace


MD Histo Workspace Properties
#############################

For a full list of the available properties and operation look at the :py:obj:`IMDHistoWorkspace api page <mantid.api.IMDHistoWorkspace>`.

.. testcode:: MDHistoWorkspaceProperties

   ws=CreateMDHistoWorkspace(Dimensionality=2,Extents='-3,3,-10,10', \
                                 SignalInput=range(0,100),ErrorInput=range(0,100),\
                                 NumberOfBins='10,10',Names='Dim1,Dim2',Units='MomentumTransfer,EnergyTransfer')

   print "Number of events =", ws.getNEvents()
   print "Number of dimensions =", ws.getNumDims()
   print "Normalization =", ws.displayNormalization()
   for i in range(ws.getNumDims()):
      dimension = ws.getDimension(i)
      print "\tDimension {0} Name: {1}".format(i,
         dimension.name)

.. testoutput:: MDHistoWorkspaceProperties
   :hide:
   :options: +ELLIPSIS,+NORMALIZE_WHITESPACE

   Number of events = 0
   Number of dimensions = 2
   Normalization = NoNormalization
      Dimension 0 Name: Dim1
      Dimension 1 Name: Dim2

Dimensions
^^^^^^^^^^

As a generic multi dimensional container being able to access information about the dimensions is very important.

.. testcode:: MDHistoWorkspaceDimensions

   ws=CreateMDHistoWorkspace(Dimensionality=2,Extents='-3,3,-10,10', \
                                 SignalInput=range(0,100),ErrorInput=range(0,100),\
                                 NumberOfBins='10,10',Names='Dim1,Dim2',Units='MomentumTransfer,EnergyTransfer')

   print "Number of dimensions =", ws.getNumDims()
   for i in range(ws.getNumDims()):
     dimension = ws.getDimension(i)
     print "\tDimension {0} Name: {1} id: {2} Range: {3}-{4} {5}".format(i,
         dimension.getDimensionId(),
         dimension.name,
         dimension.getMinimum(),
         dimension.getMaximum(),
         dimension.getUnits())

   print "The dimension assigned to X =", ws.getXDimension().name
   print "The dimension assigned to Y =", ws.getYDimension().name
   try:
     print "The dimension assigned to Z =", ws.getZDimension().name
   except RuntimeError:
      # if the dimension does not exist you will get a RuntimeError
     print "Workspace does not have a Z dimension"

   # you can also get a dimension by it's id
   dim = ws.getDimensionIndexById("Dim1")
   # or name
   dim = ws.getDimensionIndexByName("Dim2")


.. testoutput:: MDHistoWorkspaceDimensions
   :hide:
   :options: +ELLIPSIS,+NORMALIZE_WHITESPACE

   Number of dimensions = 2
      Dimension 0 Name: Dim1 id: Dim1 Range: -3.0-3.0 MomentumTransfer
      Dimension 1 Name: Dim2 id: Dim2 Range: -10.0-10.0 EnergyTransfer
   The dimension assigned to X = Dim1
   The dimension assigned to Y = Dim2
   The dimension assigned to Z = Workspace does not have a Z dimension

Accessing the Data
##################

.. testcode:: MDWorkspaceData

   ws=CreateMDHistoWorkspace(Dimensionality=2,Extents='-3,3,-10,10', \
                              SignalInput=range(0,100),ErrorInput=range(0,100),\
                              NumberOfBins='10,10',Names='Dim1,Dim2',Units='MomentumTransfer,EnergyTransfer')
                                    
   # To get the signal and error at a prticular position                            
   index = ws.getLinearIndex(5,5)
   print ws.signalAt(index)
   print ws.errorSquaredAt(index)

   # To extract the whole signal aray
   signalArray =  ws.getSignalArray()
   # or the whole error squared array
   errorSquaredArray =  ws.getErrorSquaredArray()

.. testoutput:: MDWorkspaceData
   :hide:
   :options: +ELLIPSIS,+NORMALIZE_WHITESPACE

   55.0
   3025.0

Arithmetic Operations
#####################

The following algorithms allow you to perform simple arithmetic on the
values:

-  :ref:`MinusMD <algm-MinusMD>`, :ref:`PlusMD <algm-PlusMD>`, :ref:`DivideMD <algm-DivideMD>`,
   :ref:`MultiplyMD <algm-MultiplyMD>`
-  :ref:`ExponentialMD <algm-ExponentialMD>`, :ref:`PowerMD <algm-PowerMD>`,
   :ref:`LogarithmMD <algm-LogarithmMD>`

These arithmetic operations propagate errors as described
`here <http://en.wikipedia.org/wiki/Propagation_of_uncertainty#Example_formulas>`__.
The formulas used are described in each algorithm's wiki page.

The basic arithmetic operators are available from python. For example:

.. testcode:: MDWorkspaceArithmetic

   # Get two workspaces
   A=CreateMDHistoWorkspace(Dimensionality=2,Extents='-3,3,-10,10', \
                                    SignalInput=range(0,100),ErrorInput=range(0,100),\
                                    NumberOfBins='10,10',Names='Dim1,Dim2',Units='MomentumTransfer,EnergyTransfer')
   B = A.clone() 

   # Creating a new workspace
   C = A + B
   C = A - B
   C = A * B
   C = A / B
   # Modifying a workspace in-place
   C += A
   C -= A
   C *= A
   C /= A
   # Operators with doubles
   C = A * 12.3
   C *= 3.45

   #Compound arithmetic expressions can be made, e.g:
   E = (A - B) / (C * C)

Boolean Operations
##################

The MDHistoWorkspace can be treated as a boolean workspace. In this
case, 0.0 is "false" and 1.0 is "true".

The following operations can create a boolean MDHistoWorkspace:

-  :ref:`LessThanMD <algm-LessThanMD>`, :ref:`GreaterThanMD <algm-GreaterThanMD>`,
   :ref:`EqualToMD <algm-EqualToMD>`

These operations can combine/modify boolean MDHistoWorkspaces:

-  :ref:`NotMD <algm-NotMD>`, :ref:`AndMD <algm-AndMD>`, :ref:`OrMD <algm-OrMD>`,
   :ref:`XorMD <algm-XorMD>`

These boolean operators are available from python. Make sure you use the
bitwise operators: & \| ^ ~ , not the "word" operators (and, or, not).
For example:

.. testcode:: MDWorkspaceBoolean

   # Get two workspaces
   A=CreateMDHistoWorkspace(Dimensionality=2,Extents='-3,3,-10,10', \
                                    SignalInput=range(0,100),ErrorInput=range(0,100),\
                                    NumberOfBins='10,10',Names='Dim1,Dim2',Units='MomentumTransfer,EnergyTransfer')
   B = A.clone() 

   # Create boolean workspaces by comparisons
   C = A > B
   D = B < 12.34
   # Combine boolean workspaces using not, or, and, xor:
   not_C = ~C
   C_or_D = C | D
   C_and_D = C & D
   C_xor_D = C ^ D
   C |= D
   C &= D
   C ^= D
   # Compound expressions can be used:
   D = (A > 123) & (A > B) & (A < 456)

Using Boolean Masks
###################
      
The :ref:`SetMDUsingMask <algm-SetMDUsingMask>` algorithm allows you to modify
the values in a MDHistoWorkspace using a mask created using the boolean
operations above. See the `algorithm wiki page <algm-SetMDUsingMask>`__ for
more details.



.. categories:: Concepts
