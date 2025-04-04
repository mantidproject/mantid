.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This is a workflow algorithm that rebins each spectrum of a workspace independently.
This is intended for workspaces with a relatively small number of spectra (e.g. <10), but places no restrictions on the input workspace.

The minimum and maximum values that are specified are interpreted as follows:

* One value per spectrum. If there is only one value overall, it is used for all of the spectra.
* ``numpy.nan``, ``math.nan``, and ``np.inf`` are interpreted to mean use the data's minimum or maximum x-value.

The ``Delta`` parameter is required and can either be a single number which is common to all, or one number per spectra.
Positive values are interpreted as constant step-size. Negative are logarithmic.

Please refer to :ref:`Rebin <algm-Rebin>` for a more analytical explanation of `FullBinsOnly`.

Usage
-----

.. include:: ../usagedata-note.txt

This is an example of how ``RebinRagged`` would be used near the end of a workflow to generate a real-space distribution of data after it had been reduced into a number of "banks" or "spectra."
As mentioned above, ``numpy.nan`` or ``math.nan`` can both be used.
This particular use-case, which uses the input workspace's binning, could be done with :ref:`CropWorkspaceRagged <algm-CropWorkspaceRagged>` with similar results.

.. code-block:: python

    from numpy import nan
    NOM_91796 = LoadNexusProcessed(Filename='NOM_91796_banks.nxs')
    RebinRagged(InputWorkspace=NOM_91796, OutputWorkspace='cropped',
                        Xmin=[0.67, 1.20, 2.42, 3.70, 4.12, 0.39],
                        Delta=0.02,
                        Xmax=[10.20, 20.8, nan, nan, nan, 9.35])


Sometimes due to the data or logarithmic rebinning, there are incomplete bins left over at the end of the spectrum.  These incomplete bins may result in artifacts at the tail end.  This can be removed by setting the `FullBinsOnly` parameter to `True`.

.. code-block:: python

    from mantid.simpleapi import *

    from time import time


    ## create a workspace to be rebin-ragged
    wsname = "ws"
    CreateSampleWorkspace(
        OutputWorkspace=wsname,
        BankPixelWidth=3,
    )
    GroupDetectors(
        InputWorkspace=wsname,
        OutputWorkspace=wsname,
        GroupingPattern="0-3,4-5,6-8,9-12,13-14,15-17",
    )

    # rebin the workspace raggedly
    xMin = [0.05,0.06,0.1,0.07,0.04, 0.04]
    xMax = [0.36,0.41,0.64,0.48,0.48,0.48]
    delta = [-0.000401475,-0.000277182,-0.000323453,-0.000430986,-0.000430986,-0.000430986]
    RebinRagged(
        InputWorkspace=wsname,
        XMin=xMin,
        XMax=xMax,
        Delta=delta,
        FullBinsOnly=True,
        OutputWorkspace=wsname,
    )

.. categories::

.. sourcelink::
