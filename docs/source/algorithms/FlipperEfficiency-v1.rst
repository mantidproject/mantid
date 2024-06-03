.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Calculates the efficiency of a single flipper  with regards to wavelength. The input workspace must be a group workspace
containing four known spin states, which can be given by the ``SpinStates`` parameter. It then uses these four spin
states to calculate the proportion of neutrons lost to the flipper with regards to wavelength.

The polarization of the flipper :math:`P_{F}` is given by:

.. math::

   P_F = \frac{T_{11} - T_{10}}{T_{00} - T_{01}}

Since the efficiency :math:`\epsilon_{F}` is equal to :math:`\frac{1 + P_{F}}{2}`, we can calculate the efficiency of
the flipper directly using:

.. math::

   \epsilon_{F} = \frac{T_{00} - T_{01} + T_{11} - T_{10}}{2(T_{00} - T_{01})}


Outputs
=======

If an output file path is provided, a NeXus file containing the output workspace will be saved. If an absolute path is
provided, the output workspace will be saved to the given path. If only a filename or a relative path is provided, the
output workspace will be saved with that filename into the location set in the ``Default Save Directory`` field of the
``File -> Manage User Directories`` window.

A workspace will not be output by the algorithm unless an output workspace name is provided.

An output workspace name, output file path, or both must be given.

Usage
-----

**Example - Calculate Flipper Efficiency**

.. testcode:: FlipperEfficiencyExample

    CreateSampleWorkspace(OutputWorkspace='out_00', Function='User Defined', UserDefinedFunction='name=Lorentzian, Amplitude=48000, PeakCentre=2.65, FWHM=1.2', XUnit='wavelength', NumBanks=1, BankPixelWidth=1, XMin=0, XMax=16.5, BinWidth=0.1)
    CreateSampleWorkspace(OutputWorkspace='out_11', Function='User Defined', UserDefinedFunction='name=Lorentzian, Amplitude=47000, PeakCentre=2.65, FWHM=1.2', XUnit='wavelength', NumBanks=1, BankPixelWidth=1, XMin=0, XMax=16.5, BinWidth=0.1)
    CreateSampleWorkspace(OutputWorkspace='out_10', Function='User Defined', UserDefinedFunction='name=Lorentzian, Amplitude=22685, PeakCentre=2.55, FWHM=0.6', XUnit='wavelength', NumBanks=1, BankPixelWidth=1, XMin=0, XMax=16.5, BinWidth=0.1)
    CreateSampleWorkspace(OutputWorkspace='out_01', Function='User Defined', UserDefinedFunction='name=Lorentzian, Amplitude=22685, PeakCentre=2.55, FWHM=0.6', XUnit='wavelength', NumBanks=1, BankPixelWidth=1, XMin=0, XMax=16.5, BinWidth=0.1)

    group = GroupWorkspaces(['out_00','out_11','out_10','out_01'])

    group = ConvertUnits(group, "Wavelength")

    out = FlipperEfficiency(group, SpinStates="00, 11, 10, 01")

    print("Flipper efficiency at a wavelength of " + str(mtd['out'].dataX(0)[3]) + " Å is " + str(mtd['out'].dataY(0)[3]))

Output:

.. testoutput:: FlipperEfficiencyExample
    :options: +ELLIPSIS +NORMALIZE_WHITESPACE

    Flipper efficiency at a wavelength of 0.3 Å is ...

.. categories::

.. sourcelink::