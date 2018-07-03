.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

The final unit of the x-axis is changed to momentum (Y) space as defined
by the formula:

.. raw:: html

   <center>

:math:`Y = 0.2393\frac{M}{\epsilon_i^{0.1}}(\omega - \frac{q^2}{2M})`

.. raw:: html

   </center>

where :math:`M` is the mass in atomic mass units,
:math:`\displaystyle\epsilon` is the incident energy,
:math:`\displaystyle\omega` is the energy change and :math:`q` is
defined as :math:`\sqrt{(k_0^2 + k_1^2 - 2k_0k_1\cos(\theta))}`.

The TOF is used to calculate :math:`\displaystyle\epsilon_i` and the
:math:`\displaystyle1/\epsilon` dependence causes an increasing set of
TOF values to be mapped to a decreasing set of :math:`\displaystyle Y`
values. As a result the final :math:`Y`-space values are reversed to
give a workspace with monotonically increasing :math:`Y` values.

Usage
-----

**Example - simple convert to Y-space:**

.. testcode:: ExConvetToYSp

    ###### Simulates Load workspace for and Indirect spectrometer #################
    tof_ws = CreateSimulationWorkspace(Instrument='MAR',BinParams=[-50,2,50],UnitX='TOF')
    SetInstrumentParameter(tof_ws,ParameterName='t0',ParameterType='Number',Value='0.5')
    SetInstrumentParameter(tof_ws,ParameterName='efixed',ParameterType='Number',Value='30.')    
    ###### Convert data to format acceptable by ConvertToYSpace
    tof_ws = ConvertToPointData(tof_ws)
    ###### Convert to Y Space
    wsY=ConvertToYSpace(InputWorkspace='tof_ws',Mass='30')    
    #
    # Look at sample results:
    print('part of the converted workspace:')
    for i in range(0,10): print("{:.9f} {:.11f} {}".format(wsY.readX(0)[i], wsY.readY(0)[i], wsY.readE(0)[i]))



.. testcleanup:: ExConvetToYSp

   DeleteWorkspace(wsY)
   DeleteWorkspace(tof_ws)   

**Output:**

.. testoutput:: ExConvetToYSp

   part of the converted workspace:
   218.179247674 4.44978825566 0.0
   217.970903402 4.44292943299 0.0
   217.763039952 4.43608530232 0.0
   217.555655487 4.42925581439 0.0
   217.348748180 4.42244092015 0.0
   217.142316213 4.41564057078 0.0
   216.936357776 4.40885471770 0.0
   216.730871069 4.40208331255 0.0
   216.525854298 4.39532630718 0.0
   216.321305680 4.38858365367 0.0


.. categories::

.. sourcelink::
