
.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Performs an axis-aligned traspose of a :ref:`MDHistoWorkspace <MDHistoWorkspace>`. Default *Axes* setting gives not transpose. It is possible to remove dimensions from the input workspace by omitting those dimension indexes from the *Axes* property. *Axes* are zero-based indexes.

Usage
-----

**Example - TransposeMD**

.. testcode:: TransposeMDExample

   def print_dims(ws):
       for i in range(ws.getNumDims()):
           print('Dimension %i is %s' % (i, ws.getDimension(i).name))

   mdws = CreateMDWorkspace(Dimensions=3, Extents='-10,10,-10,10,-10,10', Names='A,B,C',          Units='U,U,U')
   FakeMDEventData(InputWorkspace=mdws, PeakParams='500000,0,0,0,3')
   binned = BinMD(InputWorkspace=mdws, AlignedDim0='A,0,10,100',    AlignedDim1='B,-10,10,100', AlignedDim2='C,-10,10,1')
   print('Dimensions before {}'.format(binned.getNumDims()))
   print_dims(binned)
   # Transpose the workspace
   transposed = TransposeMD(binned, Axes=[1,0])
   print('Dimensions after {}'.format(transposed.getNumDims()))
   print_dims(transposed)

Output:

.. testoutput:: TransposeMDExample

   Dimensions before 3
   Dimension 0 is A
   Dimension 1 is B
   Dimension 2 is C
   Dimensions after 2
   Dimension 0 is B
   Dimension 1 is A

.. categories::

.. sourcelink::
