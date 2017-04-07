
.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

This algorithm uses either the Debye or Einstein method to calculate kinetic energy and root mean squared momentum
and in the Debye case, root mean squared displacement, from a given temperature and atomic mass.
The outputs from this can be used to help predict the nature of peaks in Vesuvio data.

Usage
-----

**Example - VesuvioPeakPrediction**

.. testcode:: VesuvioPeakExample

    vesuvio_debye_params = VesuvioPeakPrediction(Model='Debye', Temperature=[300], AtomicMass=63.5, Frequency=20, DebyeTemperature=347)

    vesuvio_einstein_params= VesuvioPeakPrediction(Model='Einstein', Temperature=[300], AtomicMass=63.5, Frequency=20, DebyeTemperature=347)

    vp = vesuvio_debye_params
    print('--------Debye--------')
    for c in vp.keys():
        print('%s: %f' %(c, vp.column(c)[0]))
    vp = vesuvio_einstein_params
    print('\n--------Einstein--------')
    for c in vp.keys():
        print('%s: %f' %(c, vp.column(c)[0]))

**Output:**

.. testoutput:: VesuvioPeakExample

    --------Debye--------
    Temperature(K): 300.000000
    Atomic Mass(AMU): 63.500000
    Debye Temp(K): 347.000000
    Kinetic Energy(mEV): 41.204617
    RMS Momentum(A-1): 20.427128
    RMS Displacement(A): 0.076896

    --------Einstein--------
    Temperature(K): 300.000000
    Atomic Mass(AMU): 63.500000
    Frequency(mEV): 20.000000
    Kinetic Energy(mEV): 13.564904
    Effective Temp(K): 314.814301
    RMS Momentum(A): 20.390684

.. testcleanup:: VesuvioPeakExample

    DeleteWorkspace('vesuvio_einstein_params')
    DeleteWorkspace('vesuvio_debye_params')
