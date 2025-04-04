.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm performs a moving-average smoothing of data by summing
spectra of nearest neighbours over the face of detectors. The output
workspace has the same number of spectra as the input workspace. This
works on both :ref:`EventWorkspaces <EventWorkspace>` and
:ref:`Workspace2D <Workspace2D>`'s. It has two main modes of operation.

Processing Either Generically or Assuming Rectangular Detectors
###############################################################

You may either specify properties for the Rectangular Detector Group, or
the Non-uniform Detector Group, but not both. If you provide inputs for
the Rectangular Detector group, then the algorithm execution will assume
that this is your desired processing route.

For All Instruments
###################

Going through the input workspace pixel-by-pixel, Mantid finds the
nearest-neighbours with the given Radius of each pixel. The spectra are
then summed together, and normalizing to unity (see the weighting
section below).

For Instruments With Rectangular Detectors
##########################################

The algorithm looks through the :ref:`Instrument <Instrument>` to find all
the :ref:`RectangularDetectors <RectangularDetector>` defined. For each
pixel in each detector, the AdjX\*AdjY neighboring spectra are summed
together and saved in the output workspace.

WeightedSum parameter
#####################

A weighting strategy can be applied to control how the weights are
calculated. This defaults to a flat weighting strategy. Weights are
summed and scaled so that they add up to 1.

Flat Weighting
##############

All weights are 1. This is completely position in-senitive.

Linear Weighting
################

Weights are calculated according to :math:`w = 1 - r/R`, where :math:`w` is the
weighting factor, :math:`r` is the distance from the detector and :math:`R` is the
cut-off radius.

Parabolic Weighting
###################

For rectangular detectors it may be used as follows: The radius must be
zero and a AdjX and AdjY parameter must be provided.
:math:`w = AdjX - abs(x) + AdjY - abs(y) + 1`

For non-rectangular detectors, the cut-off radius is used in the
calculation. :math:`w = R - abs(x) + R - abs(y) + 1`

Gaussian Weighting
##################

This weighting is calculated from the Gaussian distribution

:math:`w = e^{-r^2/(2\sigma^2)}`

where :math:`r^2 = x^2 + y^2 + z^2` and :math:`\sigma` is the number of
standard deviations controlling the width of the distribution curve

Important notes about this algorithm are that:

-  Distances are normalised by the radius cut-off to make them
   dimensionless and scaled to 1 at the boundaries.

For EventWorkspaces
###################

Both methods of smoothing will **significantly** increase the memory
usage of the workspace. For example, if AdjX=AdjY=1, the algorithm will
sum 9 nearest neighbours in most cases. This increases the memory used
by a factor of 9.

For Workspace2D's
#################

You can use PreserveEvents = false to avoid the memory issues with an
EventWorkspace input. Please note that the algorithm **does not check**
that the bin X boundaries match.

Neighbour Searching
###################

+--------+--------+--------+--------+--------+
| |Fig1| | |Fig2| | |Fig3| | |Fig4| | |Fig5| |
+--------+--------+--------+--------+--------+
|*Fig. 1*|*Fig. 2*|*Fig. 3*|*Fig. 4*|*Fig. 5*|
+--------+--------+--------+--------+--------+

.. |Fig1| image:: ../images/NNSearchByRadius.jpg
   :width: 100%
.. |Fig2| image:: ../images/NNSearchIrregularGrid.jpg
   :width: 100%
.. |Fig3| image:: ../images/NNSearchLimitByRadius.jpg
   :width: 100%
.. |Fig4| image:: ../images/NNSearchLimitByNNs.jpg
   :width: 100%
.. |Fig5| image:: ../images/NNSearchXY.jpg
   :width: 100%

Property Values of Examples
###########################

* **Fig. 1:**

  Requesting NumberOfNeighbours=36, Radius=3. Algorithm looks for
  36 nearest neighbours with a cut-off of 3 detector widths.

* **Fig. 2:**

  Requesting NumberOfNeighbours=46, Radius=2. Algorithm looks
  for 46 nearest neighbours with a cut-off of 2 detector widths.

* **Fig. 3:**

  Requesting NumberOfNeighbours=56, Radius=3. Algorithm looks
  for 56 nearest neighbours with a cut-off of 3 detector widths.

* **Fig. 4:**

  Requesting NumberOfNeighbours=8, Radius=3. Algorithm looks
  for 8 nearest neighbours with a cut-off of 3 detector widths.

* **Fig. 5:**

  Requesting AdjX=4, AdjY=2, Radius=0. Algorithm fetches
  neighbours in the specified pattern.

How it Works
############

The algorithm will fetch neighbours using the intersection of those inside
the radius cut-off and those less than the NumberOfNeighbours specified.
*Fig. 1* illustrates this process. Searching is relative to the central
detector, those constrained by both specified number of neighbours have
been highlighted. In this case the radius cut-off and the number of
neighbours constrain the same number of detectors.

Searching via the number of neighbours will not necessarily return the
neighbours in a grid with the same number of detectors in each axis.
*Fig. 2* shows how neighbours might be returned if distances are
non-uniform. If RectangularDetectors are available, you may force the
searching to occur in rectangular manner (described below).

The SmoothingNeighbours algorithm will only take those neighbours which
are in the intersection between those constrained by the cut-off and
those constrained by the specified number of neighbours. If the radius
cut-off is the limiting factor, then those neighbours outside will not
be considered. This is illustrated in *Fig. 3* where the blue detectors
will not be considered, but will not with this radius cut-off, while the
green ones will. Likewise, in *Fig. 4* the effect of reducing the
NumberOfNeighbours property can be seen.

If the radius is set to 0, the instrument is treated as though it has
rectangular detectors. AdjX and AdjY can then be used to control the
number of neighbours independently in x and y using the AdjX and AdjY
properties. *Fig. 5* Shows the effect of this type of searching.

Ignore Masks
############

The algorithm will ignore masked detectors if this flag is set.

Usage
-----

**Example - Smooth using radius:**

.. testcode:: ExRadius

   # Create a workspace with a single rectangular bank of size 3x3. One bin only.
   ws = CreateSampleWorkspace(Function="Flat background",
                              NumBanks=1,
                              BankPixelWidth=3,
                              XMin=0,
                              XMax=1,
                              BinWidth=1)

   # Initial values
   values = [1.0, 1.0, 3.0,
             4.0, 2.0, 2.0,
             1.0, 3.0, 3.0]

   for i in range(0,9):
     ws.dataY(i)[0] = values[i]

   # Run the algorithm
   output = SmoothNeighbours(ws, Radius = 1.0,
                             RadiusUnits = 'NumberOfPixels')

   print('Number of histograms: {0}'.format(output.getNumberHistograms()))
   print('Smoothed values:')
   print(output.extractY())

Output:

.. testoutput:: ExRadius

   Number of histograms: 9
   Smoothed values:
   [[2.        ]
    [2.16666667]
    [2.        ]
    [2.        ]
    [2.22222222]
    [2.33333333]
    [2.5       ]
    [2.5       ]
    [2.5       ]]

**Example -  Smooth using AdjX and AdjY:**

.. testcode:: ExAdj

   # Create a workspace with a single rectangular bank of size 3x3. One bin only.
   ws = CreateSampleWorkspace(Function="Flat background",
                              NumBanks=1,
                              BankPixelWidth=3,
                              XMin=0,
                              XMax=1,
                              BinWidth=1)

   # Initial values
   values = [1.0, 1.0, 3.0,
             4.0, 2.0, 2.0,
             1.0, 3.0, 3.0]

   for i in range(0,9):
     ws.dataY(i)[0] = values[i]

   # Run the algorithm
   output = SmoothNeighbours(ws, AdjX=1, AdjY=3)

   print('Number of histograms: {}'.format(output.getNumberHistograms()))
   print('Smoothed values:')
   print(output.extractY())

Output:

.. testoutput:: ExAdj

   Number of histograms: 9
   Smoothed values:
   [[2.16666667]
    [2.16666667]
    [2.16666667]
    [2.22222222]
    [2.22222222]
    [2.22222222]
    [2.5       ]
    [2.5       ]
    [2.5       ]]

**Example -  Smooth and sum using SumPixelsX and SumPixelsY:**

.. testcode:: ExSum

   # Create a workspace with a single rectangular bank of size 3x3. One bin only.
   ws = CreateSampleWorkspace(Function="Flat background",
                              NumBanks=1,
                              BankPixelWidth=3,
                              XMin=0,
                              XMax=1,
                              BinWidth=1)

   # Initial values
   values = [1.0, 1.0, 3.0,
             4.0, 2.0, 2.0,
             1.0, 3.0, 3.0]

   for i in range(0,9):
     ws.dataY(i)[0] = values[i]

   # Run the algorithm
   output = SmoothNeighbours(ws, SumPixelsX=3, SumPixelsY=1)

   print('Number of histograms: {}'.format(output.getNumberHistograms()))
   print('Smoothed values:')
   print(output.extractY())

Output:

.. testoutput:: ExSum

   Number of histograms: 3
   Smoothed values:
   [[6.]
    [6.]
    [8.]]

.. categories::

.. sourcelink::
