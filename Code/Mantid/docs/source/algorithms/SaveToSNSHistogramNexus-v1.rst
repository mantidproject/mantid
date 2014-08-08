.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

The algorithm essentially copies the InputFilename into OutputFilename,
except that it replaces the data field with whatever the specified
workspace contains. The histograms do not need to be the same size (in
number of bins), but the number of pixels needs to be the same.

In addition, this only works for instruments that use
`RectangularDetectors <http://www.mantidproject.org/RectangularDetector>`__ (SNAP, TOPAZ, POWGEN, for
example); in addition, the name in the instrument definition file must
match the name in the NXS file.

Usage
-----

.. code-block:: python

    # Needs an SNS nexus file with rectangular detectors available from system tests
    ws = Load(Filename='/home/vel/workspace/TOPAZ_3132_event.nxs',  LoaderName='LoadEventNexus', LoaderVersion=1)
    SaveToSNSHistogramNexus(InputFilename="TOPAZ_3132_event.nxs",InputWorkspace=ws,OutputFilename=TOPAZ_3132_copy.nxs")



.. categories::
