.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm will convert the output of :ref:`algm-LoadWANDSCD` in
either Q or HKL space. :ref:`algm-FindPeaksMD` can be run on the
output Q sample space, then the UB can be found and used to then
convert to HKL. The default binning ranges are good for converting to
Q sample with the default wavelength.

This algorithm will also work for data from DEMAND (HB3A).

The normalization is calculated in the same way as
:ref:`algm-MDNormSCD` but with the solid angle and flux coming from
the NormalisationWorkspace, normally vanadium. A brief introduction to
the multi-dimensional data normalization can be found :ref:`here
<MDNorm>`.

If a background workspace is provided, it will be used to calculate the
Output Workspace in the same manner as :ref:`MDNorm <MDNorm>`

When converting to HKL it will use the UB matrix from the UBWorkspace
if provided otherwise it will use the UB matrix from the
InputWorkspace. Uproj, Vproj and Wproj are only used when converting
to HKL Frame.

If the KeepTemporaryWorkspaces option is True the data and the
normalization in addition to the normalized data will be
outputted. This allows you to run separate instances of
ConvertWANDSCDtoQ and combine the results. They will have names
"ws_data" and "ws_normalization" respectively. If a background workspace
is provided, "ws_background_data" and "ws_background_normalization" will also
be outputted.

If a temporary workspace is provided for data, normalization, background data, or background
normalization, KeepTemporaryWorkspaces will be assumed 'True' and the provided workspace will
accumulate the results. If an output name is provided along with a temporary workspace for any of these
workspaces, the name will only be applied to the outputted workspace and not the temporary
accumulated workspace.

If SymmetryOperations are provided, the output workspace will be the sum of
each Symmetry Operation applied to the input workspace. More information about
Symmetry Operations can be found :ref:`here <Symmetry Groups>`

Usage
-----

**Convert to Q**

.. code-block:: python

    # Load Data and normalisation
    LoadWANDSCD(IPTS=7776, RunNumbers=26509, OutputWorkspace='norm',Grouping='4x4') # Vanadium
    LoadWANDSCD(IPTS=7776, RunNumbers='26640-27944', OutputWorkspace='data',Grouping='4x4')
    ConvertWANDSCDtoQ(InputWorkspace='data',
                      NormalisationWorkspace='norm',
                      OutputWorkspace='Q',
                      BinningDim1='-1,1,1')

    # Plot workspace
    import matplotlib.pyplot as plt
    from mantid import plots
    fig, ax = plt.subplots(subplot_kw={'projection':'mantid'})
    c = ax.pcolormesh(mtd['Q'], vmax=1)
    cbar=fig.colorbar(c)
    cbar.set_label('Intensity (arb. units)')
    #fig.savefig('ConvertWANDSCDtoQ_Q.png')

Output:

.. figure:: /images/ConvertWANDSCDtoQ_Q.png

**Convert to HKL**

.. code-block:: python

    # Load Data and normalisation
    LoadWANDSCD(IPTS=7776, RunNumbers=26509, OutputWorkspace='norm',Grouping='4x4') # Vanadium
    LoadWANDSCD(IPTS=7776, RunNumbers='26640-27944', OutputWorkspace='data',Grouping='4x4')
    SetUB('data', UB='0,0.1770619741,-0.00927942487,0.177304965,0,0,0,-0.00927942487,-0.177061974')
    ConvertWANDSCDtoQ(InputWorkspace='data',
                      NormalisationWorkspace='norm',
                      OutputWorkspace='HKL',
                      Frame='HKL',
                      BinningDim0='-1,1,1',
                      BinningDim1='-2.02,7.02,226',
                      BinningDim2='-6.52,2.52,226')

    # Plot workspace
    import matplotlib.pyplot as plt
    from mantid import plots
    fig, ax = plt.subplots(subplot_kw={'projection':'mantid'})
    c = ax.pcolormesh(mtd['HKL'], vmax=1)
    cbar=fig.colorbar(c)
    cbar.set_label('Intensity (arb. units)')
    #fig.savefig('ConvertWANDSCDtoQ_HKL.png')

Output:

.. figure:: /images/ConvertWANDSCDtoQ_HKL.png

.. categories::

.. sourcelink::
