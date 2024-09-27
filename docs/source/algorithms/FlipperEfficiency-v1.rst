.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Calculates the efficiency of a single flipper as a function of wavelength, as described by Wildes [#WILDES]_ and by Krycka et al. [#KRYCKA]_.
The input workspace must be a group of four single spectrum workspaces,
each representing the transmission of a known spin state as specified by the ``SpinStates`` parameter. These four spin
state transmissions are used to calculate the proportion of neutrons lost to the flipper as a function of wavelength.

The polarization of the flipper :math:`P_{F}` is given by:

.. math::

   P_F = \frac{(\frac{T_{11} - T_{10}}{T_{11} + T_{10}})}{(\frac{T_{00} - T_{01}}{T_{00} + T_{01}})}

Since the efficiency :math:`\epsilon_{F}` is equal to :math:`\frac{1 + P_{F}}{2}`, we can calculate the efficiency of
the flipper directly using:

.. math::

   \epsilon_{F} = \frac{T_{11}T_{00} - T_{10}T_{01}}{(T_{11} + T_{10})(T_{00} - T_{01})}

The errors are calculated as follows:

.. math::

   \sigma_{\epsilon_{F}} = \sqrt{| \frac{\partial \epsilon_{F}}{\partial T_{11}}|^2 * \sigma^2_{T_{11}} + | \frac{\partial \epsilon_{F}}{\partial T_{00}}|^2 * \sigma^2_{T_{00}} + | \frac{\partial \epsilon_{F}}{\partial T_{10}}|^2 * \sigma^2_{T_{10}} + | \frac{\partial \epsilon_{F}}{\partial T_{01}}|^2 * \sigma^2_{T_{01}}}

Where:

.. math::

   \frac{\partial \epsilon_{F}}{\partial T_{11}} = \frac{T_{10} * (T_{00} + T_{01})}{(T_{11} + T_{10})^2 * (T_{00} - T_{01})}

.. math::

   \frac{\partial \epsilon_{F}}{\partial T_{00}} = \frac{T_{01} * (T_{10} - T_{11})}{(T_{11} + T_{10}) * (T_{00} - T_{01})^2}

.. math::

   \frac{\partial \epsilon_{F}}{\partial T_{10}} = \frac{-T_{11} * (T_{00} + T_{01})}{(T_{11} + T_{10})^2 * (T_{00} - T_{01})}

.. math::

   \frac{\partial \epsilon_{F}}{\partial T_{01}} = \frac{T_{00} * (T_{11} - T_{10})}{(T_{11} + T_{10}) * (T_{00} - T_{01})^2}

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

    print("Flipper efficiency at a wavelength of {:.1f} Å is ".format(mtd['out'].dataX(0)[3]) + str(mtd['out'].dataY(0)[3]))

Output:

.. testoutput:: FlipperEfficiencyExample
    :options: +ELLIPSIS +NORMALIZE_WHITESPACE

    Flipper efficiency at a wavelength of 0.3 Å is ...

References
----------

.. [#WILDES] A. R. Wildes, *Neutron News*, **17** 17 (2006)
             `doi: 10.1080/10448630600668738 <https://doi.org/10.1080/10448630600668738>`_
.. [#KRYCKA] K. Krycka et al., *J. Appl. Crystallogr.*, **45** (2012)
             `doi: 10.1107/S0021889812003445 <https://doi.org/10.1107/S0021889812003445>`_

.. categories::

.. sourcelink::