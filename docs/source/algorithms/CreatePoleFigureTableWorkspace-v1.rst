.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Create a table with the information required to produce a pole figure, namely, a collection of alphas, betas,
and intensities.

If a peak parameters table is provided, the intensities and chi squared values are used by the algorithm,
otherwise the chi squared will be assumed to be ``0`` and the intensities assumed to be ``1``.

The algorithm supports selection of which spectra from the input are included in the final table, by use of both
a threshold on the peak position and on the chi squared value. If a reflection is specified, the algorithm will
check that the fit ``X0`` value in the peak parameter table is within ``PeakThreshold`` of the d spacing value
for the specified peak (Note: as such, specifying a reflection requires a
:ref:`Crystal Structure be set<algm-SetCrystalStructure>` on the Input Workspace). If a chi squared threshold is
specified, only spectra with a fit ``chi2`` value in the peak parameter table will be included in the output table.


Usage
-----

**ConjoinFiles Example**

.. testcode:: CreatePoleFigureTableEx

    # import mantid algorithms, numpy and matplotlib
    from mantid.simpleapi import *
    import matplotlib.pyplot as plt
    import numpy as np
    from mantid.geometry import CrystalStructure

    # create a workspace
    ws = CreateWorkspace(
        DataX=[0., 1.],
        DataY=[1., 2., 3.,4.],
        NSpec=4)


    # set the instrument to have a detector at 2theta
    EditInstrumentGeometry(Workspace='ws',
                           PrimaryFlightPath = 50, L2=[50,50,50,50], Polar=[85, 90, 95, 90],
                           Azimuthal= [0,0,0,45], DetectorIDs = [0,1,2,3])

    # set a crystal structure
    xtal = CrystalStructure("5.431 5.431 5.431", "F d -3 m", "Si 0 0 0 1.0 0.05")

    # declare a reflection of interest
    hkl = "(1,1,1)"

    # define a sample shape
    sample_shape = f"""
    <cylinder id="A">
      <centre-of-bottom-base x="0" y="0" z="0.05" />
      <axis x="0" y="0" z="-1" />
      <radius val="0.1" />
      <height val="0.1" />
    </cylinder>
    """

    # apply a generic rotation, set the sample and crystal structure
    SetGoniometer("ws", Axis0="45,0,1,0,1")
    SetSample('ws', Geometry={'Shape': 'CSG', 'Value': sample_shape})
    ws.sample().setCrystalStructure(xtal)

    # Create an example PeakParameter Table
    peak_param_table = CreateEmptyTableWorkspace(OutputWorkspace = "PeakParameterWS")
    for col in ("I", "X0", "chi2"):
        peak_param_table.addColumn("double", col)
    for i in range(4):
        val = 1.0 + (i / 10)
        peak_param_table.addRow([val, 3.14+(i/100), i]) # 3.14 hkl (1,1,1) dSpacing


    # run alg
    CreatePoleFigureTableWorkspace(InputWorkspace = "ws",
                                           PeakParameterWorkspace = "PeakParameterWS",
                                           OutputWorkspace = "outWS",
                                           PeakPositionThreshold = 0.0,
                                           Chi2Threshold = 0.0)



.. categories::

.. sourcelink::
