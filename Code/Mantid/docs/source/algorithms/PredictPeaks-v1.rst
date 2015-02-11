.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

This algorithm will predict the position of single-crystal diffraction
peaks (both in detector position/TOF and Q-space) and create an output
`PeaksWorkspace <http://www.mantidproject.org/PeaksWorkspace>`_ containing the result.

This algorithm uses the InputWorkspace to determine the instrument in
use, as well as the UB Matrix and Unit Cell of the sample used. You can
use the :ref:`algm-CopySample` algorithm (with CopyLattice=1) to
copy a UB matrix from a PeaksWorkspace to another workspace.

The algorithm operates by calculating the scattering direction (given
the UB matrix) for a particular HKL, and determining whether that hits a
detector. The Max/MinDSpacing parameters are used to determine what
HKL's to try.

The parameters of WavelengthMin/WavelengthMax also limit the peaks
attempted to those that can be detected/produced by your instrument.

Using HKLPeaksWorkspace
#######################

If you specify the HKLPeaksWorkspace parameter, then the algorithm will
use the list of HKL in that workspace as the starting point of HKLs,
instead of doing all HKLs within range of Max/MinDSpacing and
WavelengthMin/WavelengthMax.

A typical use case for this method is to use
:ref:`algm-FindPeaksMD` followed by :ref:`algm-IndexPeaks` to
find the HKL of each peak. The HKLs found will be floats, so specify
RoundHKL=True in PredictPeaks to predict the position at the exact
integer HKL. This may help locate the center of peaks.

Another way to use this algorithm is to use
:ref:`algm-CreatePeaksWorkspace` to create a workspace
with the desired number of peaks. Use python or the GUI to enter the
desired HKLs. If these are fraction (e.g. magnetic peaks) then make sure
RoundHKL=False.

Similar algorithms
##################

:ref:`algm-PredictFractionalPeaks`

.. categories::
