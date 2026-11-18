.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Integration sums up spectra in a :py:obj:`~mantid.api.Workspace` and outputs a
:py:obj:`~mantid.api.Workspace` that contains only 1 value per spectrum (i.e.
the sum). The associated errors are added in quadrature. The two X
values per spectrum are set to the limits of the range over which the
spectrum has been integrated. By default, the entire range is integrated
and all spectra are included.

Optional properties
###################

If only a portion of the workspace should be integrated then the
optional parameters may be used to restrict the range.
``StartWorkspaceIndex`` and ``EndWorkspaceIndex`` may be used to select a
contiguous range of spectra in the workspace (note that these parameters
refer to the workspace index value rather than spectrum numbers as taken
from the raw file). If only a certain range of each spectrum should be
summed then the ``RangeLower`` and ``RangeUpper`` properties, as well as their ``-List``
versions should be used. ``RangeLower`` and ``RangeUpper`` are single values
limiting the summing range over all histograms. ``RangeLowerList`` and
``RangeUpperList`` contain the ranges for each individual histogram. The
properties can be mixed: for instance, the histogram specific lower
integration limits can be given by ``RangeLowerList`` while all upper limits
can be set to the same value by ``RangeUpper``. If both list and non-list versions
are given, then the range is chosen which gives stricter limits for each
histogram.

No rebinning takes place as part of this algorithm. If the integration limits given
do not coincide with a bin boundary then the behaviour depends on the ``IncludePartialBins``
parameter. If ``IncludePartialBins=True`` then a contribution is calculated for any
bins that partially sit inside the integration limits. If ``IncludePartialBins=False``
then the integration only includes bins that sit entirely within the integration limits.
If an integration limit is given that is beyond the X range covered by the spectrum then
the integration will proceed up to final bin boundary. The data that falls outside any
integration limits set will not contribute to the output workspace.

Limits in a different unit
##########################

By default, the integration limits are in the X unit of the input workspace. The optional
``RangeUnit`` property allows ``RangeLower``, ``RangeUpper``, ``RangeLowerList`` and
``RangeUpperList`` to be given in another unit, for example to integrate a time-of-flight
workspace over a range of wavelengths.

Only the limits are converted, separately for each spectrum, so spectra from detectors at
different positions are integrated over different X ranges. The data is not converted and
the input workspace is not changed. The two X values of each output spectrum, and the unit of
the output X axis, remain in the X unit of the input workspace. If the unit runs in the
opposite direction to the X axis, such as energy on a time-of-flight axis, the lower limit in
``RangeUnit`` corresponds to the upper end of the integrated X range.

Unless the two units have a direct conversion, such as wavelength and energy, the workspace
needs an instrument with a source and a sample. The energy mode and any fixed energy are
taken from the workspace.

The conversion is checked for every spectrum being integrated before the algorithm runs, and
the algorithm will not run if any of them fails. This includes a missing value such as ``Ei``,
a spectrum without detectors and a monitor that has no equivalent of the limits. Use
``StartWorkspaceIndex`` and ``EndWorkspaceIndex`` to leave out such spectra. If ``RangeUnit``
differs from the X unit, at least one integration limit must be given.

If ``RangeUnit`` is used with an :py:obj:`EventWorkspace <mantid.dataobjects.EventWorkspace>`
then the existing bins of each spectrum are integrated, and ``RangeLowerList`` and
``RangeUpperList`` may be used. The limits follow the same rules for bin boundaries and
``IncludePartialBins`` as for histogram data, so :ref:`algm-Rebin` the workspace first if finer
bins are required.

EventWorkspaces
###############

If an :py:obj:`EventWorkspace <mantid.dataobjects.EventWorkspace>` is used as the input, the
output will be a :py:obj:`MatrixWorkspace <mantid.api.MatrixWorkspace>`.
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

Distribution Data
#################

Mantid workspaces store their data internally in one of two formats: as *counts* or as
*frequencies* (counts divided by bin-width). When the :math:`y` values are stored as
frequencies, the workspace is called a *distribution*.
The algorithms :ref:`ConvertToDistribution <algm-ConvertToDistribution>` and
:ref:`ConvertFromDistribution <algm-ConvertFromDistribution>` converts the internal
representation from counts to frequencies or vice versa.
The ``Integration`` algorithm will correctly deal with the data to give the total
**counts** as output. That is, if you integrate a distribution workspace directly or
convert it first to counts and then call ``Integration`` the output workspace will have
the same :math:`y` values.

Note that the un-integrated axis (say the :math:`x` axis) may still be binned,
in which case the result of integrating distribution vs non-distribution data will not be equivalent.
That is, integrating a distribution will create a new distribution where the internal :math:`y`
values represent the summed counts per :math:`x`-bin-width. Whereas,
integrating a non-distribution workspace will yield the same internal :math:`y` values but
these now represent counts (not counts per :math:`x`-bin-width).


Fractional Rebinning
####################

Some algorithms create a special type of :py:obj:`Workspace2D <mantid.dataobjects.Workspace2D>` called a ``RebinnedOutput`` workspace,
in which each bin contains both a value and the fractional overlap area of this bin over
that of the original data. There is more discussion of this in the :ref:`FractionalRebinning <FractionalRebinning>`
concepts page.

The Integration algorithm differs for **RebinnedOutput** workspaces, please consult the
page :ref:`FractionalRebinning <FractionalRebinning>` for more information.


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
    print('Integral of spectrum 11 is {0}'.format(intg.y(0)[0]))
    print('Integral of spectrum 12 is {0}'.format(intg.y(1)[0]))
    print('Integral of spectrum 13 is {0}'.format(intg.y(2)[0]))
    print('Integration range is [ {0}, {1} ]'.format(intg.x(0)[0], intg.x(0)[1]))

Output:

.. testoutput:: ExWsIndices

    The result workspace has 10 spectra
    Integral of spectrum 11 is 100.0
    Integral of spectrum 12 is 100.0
    Integral of spectrum 13 is 100.0
    Integration range is [ 0.0, 20000.0 ]

**Example - Integrating a time-of-flight workspace over a wavelength range:**

.. testcode:: ExRangeUnit

    # Create a workspace in time-of-flight filled with a constant value = 1.0
    ws = CreateSampleWorkspace('Histogram', 'Flat background', NumBanks=1, BankPixelWidth=1, BinWidth=100)
    # Integrate between 1 and 2 Angstroms
    intg = Integration(ws, RangeLower=1.0, RangeUpper=2.0, RangeUnit='Wavelength')

    # The output X values are still in time-of-flight
    print('Output X unit is {0}'.format(intg.getAxis(0).getUnit().unitID()))
    print('Integration range is [ {0}, {1} ]'.format(intg.x(0)[0], intg.x(0)[1]))
    print('Integral of spectrum 0 is {0}'.format(intg.y(0)[0]))

Output:

.. testoutput:: ExRangeUnit

    Output X unit is TOF
    Integration range is [ 3800.0, 7500.0 ]
    Integral of spectrum 0 is 37.0

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
    spectrum_info = ws.spectrumInfo()
    for i in range(nHisto):
        L2 = spectrum_info.l2(i)
        tof = UnitConversion.run('Energy', 'TOF', Ei, L1, L2, 0.0, DeltaEModeType.Direct, Ei)
        ys = ws.mutableY(i)
        ys += peak(tof, ws.x(i))

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

    print('Intensity of the first peak: {:.5}'.format(totalIntensity.y(0)[0]))
    print('Intensity of the last peak: {:.5}'.format(totalIntensity.y(nHisto-1)[0]))

Output:

.. testoutput:: ExRangeLists

    Intensity of the first peak: ...
    Intensity of the last peak: ...

.. categories::

.. sourcelink::
    :cpp: Framework/Algorithms/src/Integration.cpp
    :h: Framework/Algorithms/inc/MantidAlgorithms/Integration.h
