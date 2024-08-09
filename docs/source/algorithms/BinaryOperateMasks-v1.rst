.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

A binary operation will be conducted on two SpecialWorkspace2D (i.e.,
masking workspace). The binary operations supported include AND, OR and
XOR (exclusive or). The operation is done between the corresponding
spectra of these two input workspaces, i.e.,

.. math:: spec_i^{output} = spec_i^{in 1} \times spec_i^{in 2}

.. math:: spec_i^{output} = spec_i^{in 1} + spec_i^{in 2}

.. math:: spec_i^{output} = spec_i^{in 1} \oplus spec_i^{in 2}

Output
------

A SpecialWorkspace2D with the same dimension and geometry as the input
two SpecialWorkspace2D.

Usage
-----

*Example - Binary Operation Usage*

.. testcode:: BinaryOperateMasksExample

   # Create some masked workspaces
   ws1 = CreateSampleWorkspace(NumBanks=1,BankPixelWidth=1)
   ws2 = CreateSampleWorkspace(NumBanks=1,BankPixelWidth=1)
   MaskDetectors(ws1, WorkspaceIndexList=0)
   MaskDetectors(ws2, WorkspaceIndexList=0)
   a, list = ExtractMask(ws1)
   b, list = ExtractMask(ws2)

   # Run using AND
   _and = BinaryOperateMasks(a, b, OperationType='AND')
   # Run using OR
   _or = BinaryOperateMasks(a, b, OperationType='OR')
   # Run using XOR
   _xor = BinaryOperateMasks(a, b, OperationType='XOR')

   print(_and.readY(0))
   print(_or.readY(0))
   print(_xor.readY(0))

Output:

.. testoutput::  BinaryOperateMasksExample

   [2.]
   [1.]
   [0.]

.. categories::

.. sourcelink::
