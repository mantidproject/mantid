.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm filters events outside of the given values (in whatever
units the workspace possesses). This can be a one or two-sided filter
depending on which of xmin & xmax are given. This algorithm pays no
attention whatsoever to any binning that has been set on the input
workspace (though it will be carried over to the output). If you need to
affect the bin boundaries as well, or want to remove some
spectra/pixels, consider using :ref:`algm-CropWorkspace`
instead.

Usage
-----

**Example: Applying a Max in TOF**

.. testcode:: ExFilterTofByMax

    ws = CreateSampleWorkspace("Event",BankPixelWidth=1)
    print("%i events before filtering" % ws.getNumberEvents())
    wsOut = FilterByXValue(ws,XMax=15000)
    print("%i events after filtering" % wsOut.getNumberEvents())


Output:

.. testoutput:: ExFilterTofByMax

    1900 events before filtering
    1550 events after filtering

**Example: Applying Max and Min in Wavelength**

.. testcode:: ExFilterWavelengthByMinMax

    ws = CreateSampleWorkspace("Event",BankPixelWidth=1)
    ws = ConvertUnits(ws,"Wavelength")
    print("%i events before filtering" % ws.getNumberEvents())
    wsOut = FilterByXValue(ws,XMin=1,XMax=3)
    print("%i events after filtering" % wsOut.getNumberEvents())


Output:

.. testoutput:: ExFilterWavelengthByMinMax
   :options: +ELLIPSIS +NORMALIZE_WHITESPACE

    1900 events before filtering
    11... events after filtering


.. categories::

.. sourcelink::
