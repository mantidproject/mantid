.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

This algorithm takes a a `MatrixWorkspace <http://www.mantidproject.org/MatrixWorkspace>`__ and
converts it into a `MDEventWorkspace <http://www.mantidproject.org/MDEventWorkspace>`__ that can be
viewed in the `SliceViewer <http://www.mantidproject.org/SliceViewer>`__.

The algorithm currently only works for instruments with
`RectangularDetectors <http://www.mantidproject.org/InstrumentDefinitionFile#Creating_Rectangular_Area_Detectors>`__. The coordinates of the
output workspace are:

-  Pixel X coordinate (integer starting at 0)
-  Pixel Y coordinate (integer starting at 0)
-  The center of the bin of the spectrum in that pixel (e.g.
   time-of-flight)

Each MDEvent created has a weight given by the number of counts in that
bin. Zero bins are not converted to events (saving memory).

Once created, the `MDEventWorkspace <http://www.mantidproject.org/MDEventWorkspace>`__ can be viewed
in the `SliceViewer <http://www.mantidproject.org/SliceViewer>`__. It can also be rebinned with
different parameters using :ref:`algm-BinMD`. This allows you to view
the data in detector-space. For example, you might use this feature to
look at your detector's sensitivity as a function of position, as well
as a function of TOF. You can also do line plots of the data. See this
screenshot for example:

.. figure:: /images/SliceViewer-DetectorFace.png
   :alt: SliceViewer-DetectorFace.png
   :align: center
   :width: 600 px

   SliceViewer-DetectorFace.png
BankNumbers Parameter
#####################

If your instrument has several `RectangularDetectors <http://www.mantidproject.org/InstrumentDefinitionFile#Creating_Rectangular_Area_Detectors>`__, you can use the
*BankNumbers* property to specify which one(s) to convert. The algorithm
looks for `RectangularDetectors <http://www.mantidproject.org/InstrumentDefinitionFile#Creating_Rectangular_Area_Detectors>`__ with the name 'bankXX' where XX is the
bank number.

If you specify more than one bank number, then the algorithm will create
a 4D `MDEventWorkspace <http://www.mantidproject.org/MDEventWorkspace>`__. The fourth dimension will be equal to the bank
number, allowing you to easily pick a bank to view.

.. categories::
