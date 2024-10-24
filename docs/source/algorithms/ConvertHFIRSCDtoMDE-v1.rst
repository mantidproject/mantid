
.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm will convert the output of :ref:`algm-LoadWANDSCD` or
the autoreduced data from DEMAND (HB3A) into a :ref:`MDEventWorkspace
<MDWorkspace>` in Q-sample, where every pixel at every scan point is
converted to a MDEvent. This is similar to
:ref:`algm-ConvertWANDSCDtoQ` except that it doesn't histogram the
data or do normalization. :ref:`algm-FindPeaksMD` can be run on the
output Q sample space, then the UB can be found and used to then
convert to HKL using :ref:`algm-ConvertWANDSCDtoQ`
. :ref:`algm-IntegratePeaksMD` will also work on the output of this
algorithm.

There is an option to apply the LorentzCorrection using the formula :math:`|\sin(2\theta)\cos(\phi)|/\lambda^3`. This helps lower the sloping background at low :math:`Q`.

Usage
-----

**Example - ConvertHFIRSCDtoMDE**

.. code-block:: python

   LoadWANDSCD(IPTS=7776, RunNumbers='26640-27944', OutputWorkspace='data',Grouping='4x4')
   ConvertHFIRSCDtoMDE(InputWorkspace='data',
                       Wavelength=1.488,
                       OutputWorkspace='Q')


Output:

.. figure:: /images/ConvertHFIRSCDtoMDE.png

.. categories::

.. sourcelink::
