
.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

This algorithm extracts horizontal or vertical profiles averaged over given bins from a MatrixWorkspace. The profile is returned as a single histogram workspace. :ref:`Ragged workspaces <Ragged_Workspace>` are not supported.

The orientation of the profile is selected by the *Direction* property. The starting point is given by *Start* while the end point by *End*. Optionally, the end point can be omitted by specifying a line *Length*. A region over which the profile is averaged is given by *HalfWidth*. The width is rounded to full bins so that partial bins are included entirely.

Special values can be completely ignored by the *IgnoreNans* and *IgnoreInfs* properties. If a segment of the line contains special values only, it will be set to zero.

Usage
-----

**Example - Horizontal line profile**

.. testcode:: HorizontalLineProfileExample

    import numpy
    
    ws = CreateSampleWorkspace(
        Function='Quasielastic Tunnelling',
        NumBanks=1
    )
    
    # Horizontal profile over spectra 5-10
    horProfile = LineProfile(
        InputWorkspace=ws,
        Centre=7.5,
        HalfWidth=2.5,
        Start=3000,
        Length=10000
    )
    
    indexMax = numpy.argmax(horProfile.readY(0))
    epp = horProfile.readX(0)[indexMax]
    print('Elastic peak at {}'.format(epp))

Output:

.. testoutput:: HorizontalLineProfileExample

    Elastic peak at 10000.0

**Example - Vertical line profile**

.. testcode:: VerticalLineProfile

    import numpy
    
    ws = CreateSampleWorkspace(
        Function='Quasielastic Tunnelling',
        NumBanks=1
    )
    
    wsInTheta = ConvertSpectrumAxis(
        InputWorkspace=ws,
        Target='Theta'
    )
    
    # Verical cuts.
    
    tofs = numpy.arange(3000, 7000, 500)
    cutWSs = list()
    for tof in tofs:
        cutWS = LineProfile(
            InputWorkspace=wsInTheta,
            OutputWorkspace='cut-at-{}-us'.format(tof),
            Direction='Vertical',
            Centre=tof,
            HalfWidth=250,
            Start=0.5,  # Degrees
            End=0.9
        )
        cutWSs.append(cutWS)
    
    for cut in cutWSs:
        # Vertical axis holds the TOF bin edges of the cut
        axis = cut.getAxis(1)
        tofStart = axis.getValue(0)
        tofEnd = axis.getValue(1)
        # Notice the overlapping TOFs. This is because partial bins are
        # included in their entirety.
        print('Average intensity between {} and {} microsec: {:.03}'.format(
            tofStart, tofEnd, cut.readY(0)[0]))

Output:

.. testoutput:: VerticalLineProfile

    Average intensity between 2600.0 and 3400.0 microsec: 0.1
    Average intensity between 3200.0 and 3800.0 microsec: 0.1
    Average intensity between 3600.0 and 4400.0 microsec: 0.164
    Average intensity between 4200.0 and 4800.0 microsec: 0.1
    Average intensity between 4600.0 and 5400.0 microsec: 0.1
    Average intensity between 5200.0 and 5800.0 microsec: 0.1
    Average intensity between 5600.0 and 6400.0 microsec: 0.227
    Average intensity between 6200.0 and 6800.0 microsec: 0.1

.. categories::

.. sourcelink::

