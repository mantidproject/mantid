.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This is an algorithm that crops each spectrum of a workspace
independently.

The minimum and maximum values that are specified are interpreted as follows:

* One value per spectrum. If there is only one value overall, it is used for all of the spectra.
* If the given value is smaller than the first data point, it will use the first data point.
* If the given value is larger than the last data point, it will use the last data point.

Usage
-----

**Example - Crop a workspace.**

.. testcode:: CropRagged

    ws = CreateWorkspace([0,1,2,3,4,2, 3,4,5,6],[0,1,2,3,4,3,4,5,6,7], NSpec=2)
    x_min=[]
    x_max =[]

    for j in range(ws.getNumberHistograms()):
        x_min.append(1.0+j)
        x_max.append(14.0)

    new=CropWorkspaceRagged(ws,x_min,x_max)

    print("The number of bins in spectrum 1 is: {}".format(new.readX(0).size))
    print("The number of bins in spectrum 2 is: {}".format(new.readX(1).size))

Output:

.. testoutput:: CropRagged

    The number of bins in spectrum 1 is: 4
    The number of bins in spectrum 2 is: 5

**Example - Crop a workspace as part of a workflow.**

.. include:: ../usagedata-note.txt

This is an example of how ``CropWorkspaceRagged`` would be used near
the end of a workflow to generate a real-space distribution of data
after it had been reduced into a number of "banks" or "spectra." As
mentioned above, ``numpy.nan`` or ``math.nan`` can both be used.

.. testcode:: RaggedWorkFlow

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
    gr=PDFFourierTransform(InputWorkspace='FQ', OutputWorkspace='Gr',
                        Direction="Backward", DeltaR=.02)
    for j in range(10,13):
            print("y values: {:.4f}".format(gr.readY(0)[j]))

Output:

.. testoutput:: RaggedWorkFlow

    y values: 0.6287
    y values: 0.6235
    y values: 0.6487

.. categories::

.. sourcelink::
