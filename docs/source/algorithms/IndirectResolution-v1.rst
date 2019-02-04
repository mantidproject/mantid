.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Creates a resolution workspace for an inelastic indirect sample run by
summing all spectra in the energy transfer and subtracting a flat background to
give a single resolution curve.

Rebinning and intensity scaling can optionally be applied to the result.

Workflow
--------

.. diagram:: IndirectResolution-v1_wkflw.dot

Usage
-----

.. include:: ../usagedata-note.txt

**Example - Running IndirectResolution.**

.. testcode:: ExIndirectResolutionSimple

    resolution = IndirectResolution(InputFiles='IRS26173.raw',
                                    Instrument='IRIS',
                                    Analyser='graphite',
                                    Reflection='002',
                                    DetectorRange=[3, 53],
                                    BackgroundRange=[-0.16, -0.14],
                                    RebinParam='-0.175,0.002,0.175')

    print('Number of histograms: {:d}'.format(resolution.getNumberHistograms()))
    print('Number of bins: {:d}'.format(resolution.blocksize()))

Output:

.. testoutput:: ExIndirectResolutionSimple

    Number of histograms: 1
    Number of bins: 175

.. categories::

.. sourcelink::
