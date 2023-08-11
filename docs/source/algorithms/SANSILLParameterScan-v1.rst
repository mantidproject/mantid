.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm allows the integration of data along a parameter. It has been designed for D16 along the Omega axis, but
it can normally be applied to other instruments and techniques.

Given a file containing a scan, this algorithm performs usual reduction for SANS detectors, and then
produces a 2D ouput, with the x-axis being the x-axis of the detector, and the y-axis the parameter used for the scan.
The data is thus the sum of each column of pixels of the detector, stacked one above the others and sorted by the
integration parameter.
The OutputJoinedWorkspace contains, if asked for, the reduced data on one workspace, along the axis defined by the parameter.

**Example - full treatment of a sample**

.. testsetup:: ExSANSILLParameterScan

    default_facility_orig = config['default.facility']
    default_instrument_orig = config['default.instrument']
    config['default.facility'] = 'ILL'
    config['default.instrument'] = 'D16'
    config.appendDataSearchSubDir('ILL/D16/')

.. testcode:: ExSANSILLParameterScan

    # reduce part of an omega scan on D16.
    SANSILLParameterScan(SampleRun="025786.nxs",
                         OutputWorkspace="output2d",
                         OutputJoinedWorkspace="reduced",
                         Observable="Omega.value",
                         PixelYmin=3,
                         PixelYMax=189)

.. testcleanup:: ExSANSILLParameterScan

    config['default.facility'] = default_facility_orig
    config['default.instrument'] = default_instrument_orig
    mtd.clear()


.. categories::

.. sourcelink::
