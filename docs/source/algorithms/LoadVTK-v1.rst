.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Loads a legacy binary format VTK uniform structured image as an MDWorkspace. Allows the user to provide the name of two scalar arrays expected to be
located on the PointData. One array is loaded as the MDWorkspace signal data and is mandatory. The other array is optional and provides the error squared data.
Both arrays are expected to be of the type vtkUnsignedShortArray.

Choosing Output Types
#####################

Direct Image Format
*******************

If the AdaptiveBinned parameter is off, the data is loaded into Mantid's multidimensional image format as an MDHistoWorkspace. All data
in the file is loaded verbatim. This is not a lossy process, so sparse regions of data are carried through to Mantid. This can lead to very large in-memory
object sizes. The algorithm will abort before the data is converted, if it is determined that you have insufficient resources. Loading data in this format
is suitable for usage with the Slice Viewer, but users should not try to visualise large workspaces of this type using the 3D visualisation tools
Vates Simple Interface, as this is designed for use with sparse datasets of moderate size.

Unless it is very important that all data is loaded, we recommend that you switch the AdaptiveBinned parameter on (see below).

Adaptive Rebinned Format
************************

For the majority of problems encountered with visualisation of neutron data, regions of interest occupy a very small fraction of otherwise empty/noisy space. It
therefore makes sense to focus high resolution in the regions of interest rather than wasting resources storing sparse data. The MDEventWorkspace format naturally
recursively splits itself up where there are high numbers of observations.

For imaging, we highly recommend using the AdaptiveBinned parameter set on, in combination with the KeepTopPercent parameter.

The MDEventWorkspace can be rebinned to a regular grid using SliceMD and BinMD both the Slice Viewer and the Vates Simple Interface supporting rebinning in-situ as part of the visualisation process.



Usage
-----

These example is for illustation only. The file used is very large and not publically available.

**Example: Adaptive Binning**  

.. code-block:: python

    outputs = LoadVTK(Filename='fly.vtk',SignalArrayName='volume_scalars',AdaptiveBinned=True)
    demo = outputs[0]
    plotSlice(source=demo)

**Example: Direct Conversion**  

.. code-block:: python

    outputs = LoadVTK(Filename='fly.vtk',SignalArrayName='volume_scalars',AdaptiveBinned=False)
    demo = outputs[0]
    plotSlice(source=demo)


.. categories::

.. sourcelink::



