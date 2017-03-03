.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Integration sums up spectra in a :ref:`Workspace <Workspace>` and outputs a
:ref:`Workspace <Workspace>` that contains only 1 value per spectrum (i.e.
the sum). The associated errors are added in quadrature. The two X
values per spectrum are set to the limits of the range over which the
spectrum has been integrated. By default, the entire range is integrated
and all spectra are included.

Optional properties
###################

If only a portion of the workspace should be integrated then the
optional parameters may be used to restrict the range.
StartWorkspaceIndex & EndWorkspaceIndex may be used to select a
contiguous range of spectra in the workspace (note that these parameters
refer to the workspace index value rather than spectrum numbers as taken
from the raw file). If only a certain range of each spectrum should be
summed then the RangeLower and RangeUpper properties, as well as their -List
versions should be used. RangeLower and RangeUpper are single values
limiting the summing range over all histograms. RangeLowerList and
RangeUpperList contain the ranges for each individual histogram. The
properties can be mixed: for instance, the histogram specific lower
integration limits can be given by RangeLowerList while all upper limits
can be set to the same value by RangeUpper. If both list and non-list versions
are given, then the range is chosen which gives stricter limits for each
histogram.

No rebinning takes place as part of this algorithm: if the ranges given do
not coincide with a bin boundary then the first bin boundary within the
range is used. If a value is given that is beyond the limit covered by
the spectrum then it will be integrated up to its limit. The data that
falls outside any values set will not contribute to the output
workspace.

EventWorkspaces
###############

If an :ref:`EventWorkspace <EventWorkspace>` is used as the input, the
output will be a :ref:`MatrixWorkspace <MatrixWorkspace>`.
:ref:`algm-Rebin` is recommended if you want to keep the workspace as an
EventWorkspace.

**Integration for event workspaces refers to internal binning, provided by** 
:ref:`algm-Rebin` **or load algorithm and may ignore limits, provided as algorithm 
input.**  For example, attempt to integrate loaded ISIS event workspace in the 
range [18000,20000] yields workspace integrated in the range [0,200000],
assuming the data were collected in the time range [0,20000]. This happens because
the event data would have single histogram workspace bin in range [0,20000].
To obtain integral in the desired range, user have to :ref:`algm-Rebin` first, 
and one of the binning intervals have to start from 18000 and another (or the same) 
end at 20000.



Usage
-----

**Example - Integration over limited number of histograms:**  

.. testcode:: ExWsIndices

    # Create a workspace filled with a constant value = 1.0
    ws=CreateSampleWorkspace('Histogram','Flat background')
    # Integrate 10 spectra over all X values
    intg=Integration(ws,StartWorkspaceIndex=11,EndWorkspaceIndex=20)
    
    # Check the result
    print('The result workspace has {0} spectra'.format(intg.getNumberHistograms()))
    print('Integral of spectrum 11 is {0}'.format(intg.readY(0)[0]))
    print('Integral of spectrum 12 is {0}'.format(intg.readY(1)[0]))
    print('Integral of spectrum 13 is {0}'.format(intg.readY(2)[0]))
    print('Integration range is [ {0}, {1} ]'.format(intg.readX(0)[0], intg.readX(0)[1]))

Output:

.. testoutput:: ExWsIndices

    The result workspace has 10 spectra
    Integral of spectrum 11 is 100.0
    Integral of spectrum 12 is 100.0
    Integral of spectrum 13 is 100.0
    Integration range is [ 0.0, 20000.0 ]

**Example - Total peak intensity:**  

.. testcode:: ExRangeLists

    from mantid.kernel import DeltaEModeType, UnitConversion
    import numpy
    ws = CreateSampleWorkspace(
        Function='Flat background',
        XMin=0,
        XMax=12000,
        BinWidth=20)
    nHisto = ws.getNumberHistograms()
    
    # Add elastic peaks to 'ws'. They will be at different TOFs
    # since the detector banks will be 5 and 10 metres from the sample.
    
    # First, a helper function for the peak shape
    def peak(shift, xs):
        xs = (xs[:-1] + xs[1:]) / 2.0  # Convert to bin centres.
        return 50 * numpy.exp(-numpy.square(xs - shift) / 1200)
    
    # Now, generate the elastic peaks.
    Ei = 23.0  # Incident energy, meV
    L1 = 10.0 # Source-sample distance, m
    sample = ws.getInstrument().getSample()
    for i in range(nHisto):
        detector = ws.getDetector(i)
        L2 = sample.getDistance(detector)
        tof = UnitConversion.run('Energy', 'TOF', Ei, L1, L2, 0.0, DeltaEModeType.Direct, Ei)
        ys = ws.dataY(i)
        ys += peak(tof, ws.readX(i))
    
    # Fit Gaussians to the workspace.
    # Fit results will be put into a table workspace 'epps'.
    epps = FindEPP(ws)
    
    # Integrate the peaks over +/- 3*sigma
    lowerLimits = numpy.empty(nHisto)
    upperLimits = numpy.empty(nHisto)
    for i in range(nHisto):
        peakCentre = epps.cell('PeakCentre', i)
        sigma = epps.cell('Sigma', i)
        lowerLimits[i] = peakCentre - 3 * sigma
        upperLimits[i] = peakCentre + 3 * sigma
    
    totalIntensity = Integration(ws,
        RangeLowerList=lowerLimits,
        RangeUpperList=upperLimits)
    
    print('Intensity of the first peak: {:.5}'.format(totalIntensity.dataY(0)[0]))
    print('Intensity of the last peak: {:.5}'.format(totalIntensity.dataY(nHisto-1)[0]))

Output:

.. testoutput:: ExRangeLists

    Intensity of the first peak: 159.99
    Intensity of the last peak: 164.5

.. categories::

.. sourcelink::
    :cpp: Framework/Algorithms/src/Integration.cpp
    :h: Framework/Algorithms/inc/MantidAlgorithms/Integration.h
