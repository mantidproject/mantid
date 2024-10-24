.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Scales the `InputWorkspace` by :math:`1/|F(Q)|^2` where :math:`F(Q)` is
the magnetic form factor for the ion specified in `IonName`.

`IonName` must be specified as a string with the element name followed
by a number which indicates the charge / oxidation state. E.g.
`Fe2` indicates :math:`\mathrm{Fe}^{2+}` or Fe(II).

Restrictions on the Input Workspace
###################################

The input workspace must have one axis with units of `MomentumTransfer`.

Usage
-----

.. include:: ../usagedata-note.txt

**ISIS Example**

The following code will run a reduction on a MARI (ISIS) dataset and apply
the algorithm to the reduced data. The datafiles (runs 21334, 21337, 21346) and
map file 'mari_res2013.map' should be in your path. Run number 21337 is a
measurement of a PrAl3 sample from the neutron training course. The single
crystal field excitation around 4.5 meV should have an intensity variation that
follows the magnetic form factor. Thus, the integrating between 4 and 5 meV in
the corrected workspace should yield a nearly flat line along the :math:`|Q|`
direction.

.. code:: python

    from Direct import DirectEnergyConversion
    from mantid.simpleapi import *
    rd = DirectEnergyConversion.DirectEnergyConversion('MARI')
    ws = rd.convert_to_energy(21334, 21337, 15, [-15,0.05,15], 'mari_res2013.map',
        monovan_run=21346, sample_mass=10.62, sample_rmm=221.856, monovan_mapfile='mari_res2013.map')
    ws_sqw = SofQW3(ws, [0,0.05,6], 'Direct', 15)
    ws_corr = MagFormFactorCorrection(ws_sqw, IonName='Pr3', FormFactorWorkspace='Pr3FF')

**Test Example**

This example uses a generated dataset so that it will run on automated tests
of the build system where the above datafiles do not exist.

.. testcode:: ExGenerated

    import numpy as np
    ws = CreateSampleWorkspace(binWidth = 0.1, XMin = 0, XMax = 50, XUnit = 'DeltaE')
    ws = ScaleX(ws, -15, "Add")
    LoadInstrument(ws, InstrumentName='MARI', RewriteSpectraMap = True)
    ws = SofQW(ws, [0, 0.05, 8], 'Direct', 35)
    Q = ws.getAxis(1).extractValues()
    for i in range(len(Q)-1):
        qv = ( (Q[i]+Q[i+1])*0.5 ) / 4 / np.pi
        y = ws.dataY(i)
        y *= np.exp(-16*qv*qv)
    ws_corr = MagFormFactorCorrection(ws, IonName='Fe3', FormFactorWorkspace='Fe3FF')

.. categories::
