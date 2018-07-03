.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This is a workflow algorithm that crops each spectrum of a workspace
independently. This is intended for workspaces with a relatively small
number of spectra (e.g. <10), but places no restrictions on the input
workspace.

The minimum and maximum values that are specified are interpreted as follows:

* One value per spectrum. If there is only one value overall, it is used for all of the spectra.
* ``numpy.nan``, ``math.nan``, and ``np.inf`` are interpreted to mean use the data's minimum or maximum x-value.

Usage
-----

.. include:: ../usagedata-note.txt

This is an example of how ``CropWorkspaceRagged`` would be used near
the end of a workflow to generate a real-space distribution of data
after it had been reduced into a number of "banks" or "spectra." As
mentioned above, ``numpy.nan`` or ``math.nan`` can both be used.

.. code-block:: python

    from numpy import nan
    NOM_91796 = LoadNexusProcessed(Filename='NOM_91796_banks.nxs')
    CropWorkspaceRagged(InputWorkspace=NOM_91796, OutputWorkspace='cropped',
                        Xmin=[0.67, 1.20, 2.42, 3.70, 4.12, 0.39],
                        Xmax=[10.20, 20.8, nan, nan, nan, 9.35])
    binning=(0.,.02,40.)
    Rebin(InputWorkspace='cropped', OutputWorkspace='cropped',
          Params=binning)
    # put into a single spectrum and Fourier transform
    SumSpectra(InputWorkspace='cropped', OutputWorkspace='FQ',
               WeightedSum=True, RemoveSpecialValues=True)
    PDFFourierTransform(InputWorkspace='FQ', OutputWorkspace='Gr',
                        InputSofQType='Q[S(Q)-1]', DeltaR=.02)


.. categories::

.. sourcelink::
