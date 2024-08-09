.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

The fluctuation dissipation theorem [1,2] relates the dynamic
susceptibility to the scattering function

:math:`\left(1-e^{-\frac{E}{k_B T}}\right) S(\mathbf{q}, E) = \frac{1}{\pi} \chi'' (\mathbf{q}, E)`

where :math:`E` is the energy transfer to the system. The algorithm
assumes that the y axis of the input workspace contains the scattering
function :math:`S`. The y axis of the output workspace will contain the
dynamic susceptibility. The temperature is either extracted as the average of the
values stored in the appropriate entry of the log attached to the workspace
(user supplies the name of the entry) or user can pass a number for the temperature.

[1] S. W. Lovesey - Theory of Neutron Scattering from Condensed Matter,
vol 1

[2] I. A. Zaliznyak and S. H. Lee - Magnetic Neutron Scattering in
"Modern techniques for characterizing magnetic materials"

Usage
-----

**Example - Run Applied Detailed Balance**

.. testcode:: ExApplyDetailedBalanceSimple

   ws = CreateWorkspace(DataX='-5,-4,-3,-2,-1,0,1,2,3,4,5',DataY='2,2,2,2,2,2,2,2,2,2',DataE='1,1,1,1,1,1,1,1,1,1',UnitX='DeltaE')
   ows = ApplyDetailedBalance(InputWorkspace='ws',OutputWorkspace='ows',Temperature='100', OutputUnits='Frequency')

   print("The Y values in the Output Workspace are")
   print(ows.readY(0)[0:5])
   print(ows.readY(0)[5:10])

Output:

.. testoutput:: ExApplyDetailedBalanceSimple

   The Y values in the Output Workspace are
   [-4.30861792 -3.14812682 -2.11478496 -1.19466121 -0.37535083]
   [0.35419179 1.00380206 1.58223777 2.09729717 2.55592407]

.. categories::

.. sourcelink::
