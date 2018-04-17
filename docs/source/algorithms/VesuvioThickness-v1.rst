
.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Determines the sample density for vesuvio based on sample transmission and composite masses

Usage
-----

**Example - VesuvioThickness**

.. testcode:: VesuvioThicknessExample

   # Algorithm inputs
   masses = [1.0079,27.0,91.0]
   amplitudes = [0.9301589,2.9496644e-02,4.0345035e-02]
   trans_guess = 0.831
   thickness = 5.0
   number_density = 1.0

   # Run algorithm
   dens_tbl, trans_tbl = VesuvioThickness(masses, amplitudes, trans_guess, thickness, number_density)

   # Test output
   print("The final density is: %.4f" % dens_tbl.cell(9,1))
   print("The final transmission is: %.4f" % trans_tbl.cell(9,1))

Output:

.. testoutput:: VesuvioThicknessExample
  :options: +NORMALIZE_WHITESPACE

   The final density is: 24.4515
   The final transmission is: 0.8310

.. categories::

.. sourcelink::
