.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

.. warning::
   This algorithm is being developed for a specific instrument. It might get changed or even 
   removed without a notification, should instrument scientists decide to do so.

.. warning::
   **Bugs:**

   - `wsOut.getNEvents()` gives the number of detectors (=122904), not the number of events
   - xDimension, id: Time-of-flight, Range: 0.00 to 0.00 microsecond
   - yDimension, id: Spectrum, Range: 1 to 122904 (=number of detectors) Hah?

This algorithm loads a DNS mesytec psd listmode file into an :ref:`EventWorkspace <EventWorkspace>`. The loader rotates the detector bank
in the position given in the data file.

**Output**

    :ref:`EventWorksp ace <EventWorkspace>` with the X-axis in the Time-of-flight units.

**ChopperChannel**

Discribes the input channel to wich the chopper was connected. It can be set to any value between 0 and 4. If the value is not set or is set to 0, the default value in the instrument definition file is used.

**MonitorChannel**

Discribes the input channel to wich the monitor was connected. It can be set to any value between 0 and 4. If the value is not set or is set to 0, the default value in the instrument definition file is used.

This parameter is unused at the moment.


Usage
-----

**Example 1 - Plot TOF Spectrum:**

.. testcode:: LoadDNSEventex1

    # data file.
    filename = "00550232.mdat"

    # load data to EventWorkspace
    wsOut = LoadDNSEvent(filename, 0, 0)

    # print output workspace information
    print("Output Workspace Type is:  {}".format(wsOut.id()))
    print("It has {0} events and {1} dimensions:".format(wsOut.getNEvents(), wsOut.getNumDims()))

    for i in range(wsOut.getNumDims()):
        dimension = wsOut.getDimension(i)
        print("Dimension {0} has name: {1}, id: {2}, Range: {3:.2f} to {4:.2f} {5}".format(i,
            dimension.getDimensionId(),
            dimension.name,
            dimension.getMinimum(),
            dimension.getMaximum(),
            dimension.getUnits()))
            
    # sum spectra of all pixels
    wsSum = SumSpectra(wsOut)
    # rebin to something usefull
    wsBinned = Rebin(wsSum, Params='0,12.8,2700')
    #plot the spectrum:
    try:
        plotSpectrum(wsBinned,[0],)
    except NameError:
        #plotSpectrum was not available, Mantidplot is probably not running
        pass

**Output:**

.. testoutput:: LoadDNSEventex1

   Output Workspace Type is:  EventWorkspace
   It has 122880 events and 2 dimensions:
   Dimension 0 has name: xDimension, id: Time-of-flight, Range: 0.00 to 0.00 microsecond
   Dimension 1 has name: yDimension, id: Spectrum, Range: 1.00 to 122880.00 


**Example 2 - Specify a different Chopper Channel**

.. testcode:: LoadDNSEventex2

    # data file.
    filename = "00550232.mdat"

    # load data to EventWorkspace
    wsOut = LoadDNSEvent(filename, 3, 0)



.. categories::

.. sourcelink::
