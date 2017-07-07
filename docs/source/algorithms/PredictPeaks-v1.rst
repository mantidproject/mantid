.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

This algorithm will predict the position of single-crystal diffraction
peaks (both in detector position/TOF and Q-space) and create an output
:ref:`PeaksWorkspace <PeaksWorkspace>` containing the result.

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

Furthermore it's possible to calculate structure factors for the
predicted peaks by activating the CalculateStructureFactors-option.
For this to work the sample needs to have a crystal structure stored,
which can currently be achieved as in the following example:

.. testcode:: ExPredictPeaksCrystalStructure

    from mantid.geometry import CrystalStructure
    import numpy as np

    # Generate a workspace with an instrument definition
    ws = CreateSimulationWorkspace(Instrument='WISH',
                                   BinParams='0,10000,20000',
                                   UnitX='TOF')

    # Set a random UB, in this case for an orthorhombic structure
    SetUB(ws, a=5.5, b=6.5, c=8.1, u='12,1,1', v='0,4,9')

    # Set an arbitrary crystal structure with 2 atoms and some
    # systematic absences due to space group Pbca
    cs = CrystalStructure('5.5 6.5 8.1', 'P b c a',
                          """Ni 0.232 0.114 0.543 1.0 0.00843;
                             Al 0.434 0.041 0.854 1.0 0.01120""")
    ws.sample().setCrystalStructure(cs)

    predicted = PredictPeaks(InputWorkspace=ws,
                             CalculateStructureFactors=True,
                             MinDSpacing=0.5,
                             WavelengthMin=0.9, WavelengthMax=6.0)

    print 'There are', predicted.getNumberPeaks(), 'detectable peaks.'

    intensities = np.array(predicted.column('Intens'))
    maxIntensity = np.max(intensities)
    relativeIntensities = intensities / maxIntensity

    print 'Maximum intensity: {0:.2f}'.format(maxIntensity)
    print 'Peaks with relative intensity < 1%:', len([x for x in relativeIntensities if x < 0.01])

    absences = [i for i, x in enumerate(intensities) if x < 1e-9]
    print 'Number of absences:', len(absences)
    print 'Absent HKLs:', [predicted.getPeak(i).getHKL() for i in absences]

The script provides some information about the predicted peaks and
their structure factors. Additionally it prints out the HKL of peaks
with predicted structure factor very close to 0, which are absent:

.. testoutput:: ExPredictPeaksCrystalStructure

    There are 295 detectable peaks.
    Maximum intensity: 6101.93
    Peaks with relative intensity < 1%: 94
    Number of absences: 16
    Absent HKLs: [[2,0,-1], [3,0,-1], [4,0,-1], [5,0,-1], [6,0,-3], [6,0,-1], [7,0,-3], [7,0,-1], [8,0,-3], [8,0,-1], [9,-1,0], [9,0,-3], [9,0,-1], [10,0,-5], [10,0,-3], [10,0,-1]]

All absent HKLs have the form H0L with odd L. This fits with the reflection
conditions given for :math:`Pbca` in the International Tables for Crystallography A.

Please note that the calculated structure factors are currently not
corrected for any instrument effects, so depending on instrument
geometry and other factors (detector efficiency etc.) measured intensities
will deviate from these values. They can however provide an estimate
for which reflections might be especially strong or weak.

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

.. seealso :: Algorithm :ref:`algm-PredictFractionalPeaks`

.. categories::

.. sourcelink::
