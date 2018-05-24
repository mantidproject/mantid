.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Returns the relative efficiency of the forward detector group compared
to the backward detector group. If *Alpha* is larger than 1 more counts
has been collected in the forward group.

.. note::

   This algorithm leaves the input workspace unchanged. To group detectors
   in a workspace use :ref:`algm-MuonGroupDetectors`.

Usage
-----

**Example - Calculating Alpha:**

.. testcode:: ExSimple

   y = [1,1,1,1,1] + [2,2,2,2,2]
   x = [1,2,3,4,5,6] * 2
   input = CreateWorkspace(x,y, NSpec=2)

   alpha = AlphaCalc(input)

   print('Alpha value: {0:.3f}'.format(alpha))

Output:

.. testoutput:: ExSimple

   Alpha value: 0.500

**Example - Calculating Alpha, reversing forward and backward spectra:**

.. testcode:: ExReversed

   y = [1,1,1,1,1] + [2,2,2,2,2]
   x = [1,2,3,4,5,6] * 2
   input = CreateWorkspace(x,y, NSpec=2)

   alpha = AlphaCalc(input,
                     ForwardSpectra=[2],
                     BackwardSpectra=[1])

   print('Alpha value: {0:.3f}'.format(alpha))

Output:

.. testoutput:: ExReversed

   Alpha value: 2.000

.. categories::

.. sourcelink::
