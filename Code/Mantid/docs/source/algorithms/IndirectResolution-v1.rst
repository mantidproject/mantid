.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Creates a resolution workspace for an inelastic indirect sample run.

See `Indirect:Calibration <http://www.mantidproject.org/Indirect:Calibration>`_.

Usage
-----

**Example - Running IndirectResolution.**

.. testcode:: ExIndirectResolutionSimple

    resolution = IndirectResolution(InputFiles='IRS26173.raw',
                                    Instrument='IRIS',
                                    Analyser='graphite',
                                    Reflection='002',
                                    DetectorRange=[3, 53],
                                    BackgroundRange=[-0.16, -0.14],
                                    RebinParam='-0.175,0.002,0.175')

    print mtd.doesExist('resolution')

Output:

.. testoutput:: ExIndirectResolutionSimple

    True

.. categories::
