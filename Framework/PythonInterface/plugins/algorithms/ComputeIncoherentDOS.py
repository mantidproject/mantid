# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import numpy as np
from scipy import constants
from mantid.kernel import CompositeValidator, Direction, FloatBoundedValidator
from mantid.api import AlgorithmFactory, CommonBinsValidator, HistogramValidator, MatrixWorkspaceProperty, PythonAlgorithm
from mantid.simpleapi import CloneWorkspace, Rebin, Rebin2D, ScaleX, Transpose


class UnitError(ValueError):
    def __init__(self):
        super(ValueError, self).__init__(
            "Input workspace must be in (Q,E) [momentum and energy transfer] or (2theta, E) [scattering angle and energy transfer.]"
        )


def cut1D(dos2d, qqgrid, spectrum_binning, dosebin):
    if qqgrid.shape[1] > 1:
        dSpectrum = spectrum_binning[1] - spectrum_binning[0]
        dos1d = Rebin2D(
            dos2d, [spectrum_binning[0], dSpectrum, spectrum_binning[1]], dosebin, UseFractionalArea=True, Transpose=True, StoreInADS=False
        )
    else:
        dos2d = Transpose(dos2d, StoreInADS=False)
        dos1d = Rebin(dos2d, dosebin, StoreInADS=False)
    return dos1d


def evaluateEbin(Emin, Emax, Ei, strn):
    splits = strn.split(",")
    if len(splits) < 2 or len(splits) > 3:
        raise ValueError("EnergyBinning must be a comma separated string with two or three values.")
    try:
        out = [eval(estr, None, {"Emax": Emax, "Emin": Emin, "Ei": Ei}) for estr in splits]
    except NameError:
        raise ValueError("Malformed EnergyBinning. Only the variables 'Emin', 'Emax' or 'Ei' are allowed.")
    except:
        raise SyntaxError("Syntax error in EnergyBinning")
    return out


def evaluateQRange(Qmin, Qmax, strn):
    splits = strn.split(",")
    if len(splits) != 2:
        raise ValueError("QSumRange must be a comma separated string with two values.")
    try:
        out = [eval(qstr, None, {"Qmin": Qmin, "Qmax": Qmax}) for qstr in splits]
    except NameError:
        raise ValueError("Malformed QSumRange. Only the variables 'Qmin' and 'Qmax' are allowed.")
    except:
        raise SyntaxError("Syntax error in QSumRange.")
    return out


def evaluateTwoThetaRange(Twothetamin, Twothetamax, strn):
    splits = strn.split(",")
    if len(splits) != 2:
        raise ValueError("TwoThetaSumRange must be a comma separated string with two values.")
    try:
        out = [eval(tstr, None, {"Twothetamin": Twothetamin, "Twothetamax": Twothetamax}) for tstr in splits]
    except NameError:
        raise ValueError("Malformed TwoThetaSumRange. Only the variables 'Twothetamin' and 'Twothetamax' are allowed.")
    except:
        raise SyntaxError("Syntax error in TwoThetaSumRange.")
    return out


def binPreservingRebinninParams(Xs, Xmin, Xmax):
    Xs = Xs[Xs > Xmin]
    Xs = Xs[Xs < Xmax]
    Xs = np.concatenate(([Xmin], Xs, [Xmax]))
    dXs = np.diff(Xs)
    params = np.empty(2 * len(Xs) - 1)
    params[0::2] = Xs
    params[1:-1:2] = dXs
    return params


def qGridFromTwoTheta(two_theta, en, ei):
    en = en * constants.e * 1e-3
    ei = ei * constants.e * 1e-3
    ef = ei - en
    kisq = 2.0 * constants.m_n * ei / (constants.hbar**2)
    kfsq = 2.0 * constants.m_n * ef / (constants.hbar**2)
    a = -2.0 * np.sqrt(kisq * kfsq)
    b = np.cos(two_theta)
    # This builds the 2D array
    q = np.multiply.outer(a, b)
    q += np.reshape((kisq + kfsq), (len(kfsq), 1))
    return np.sqrt(q) * 1e-10


def scaleUnits(dos1d, cm, input_en_in_meV, mev2cm):
    if cm and input_en_in_meV:
        dos1d.getAxis(0).setUnit("DeltaE_inWavenumber")
        dos1d = ScaleX(dos1d, mev2cm, StoreInADS=False)
    elif not cm and not input_en_in_meV:
        dos1d.getAxis(0).setUnit("DeltaE")
        dos1d = ScaleX(dos1d, 1 / mev2cm, StoreInADS=False)
    return dos1d


def usingQ(spectrum_unit):
    if spectrum_unit == "MomentumTransfer":
        using_q = True
    elif spectrum_unit == "Degrees":
        using_q = False
    else:
        raise UnitError()
    return using_q


def yLabel(absunits, cm):
    label = "g^{neutron}(E) (arb. units)"
    if absunits:
        if cm:
            label = "g(E) (states/cm^-1)"
        else:
            label = "g(E) (states/meV)"
    return label


class ComputeIncoherentDOS(PythonAlgorithm):
    """Calculates the neutron weighted generalised phonon density of states in the incoherent approximation from a
        measured powder inelastic neutron scattering spectrum.

    ComputeIncoherentDOS(InputWorkspace = iws, Temperature = T, MeanSquareDisplacement = msd,
                         QSumRange = qrange, EnergyBinning = ebins, Wavenumbers = cm, StatesPerEnergy = states,
                         OutputWorkspace = ows)

    Input:
            iws     -   An input MatrixWorkspace (must contain the reduced powder INS data)
            T       -   The sample temperature in Kelvin (default: 300 K)
            msd     -   The average mean-squared displacement of all species in the sample in Angstrom^2 (default: 0)
            qrange  -   Range in |Q| to integrate over (default: 0.5Qmax to Qmax)
            ebins   -   Energy bins in Energy transfer to rebin the data into (default: 0 to Emax in 50 steps)
            cm      -   (True/False) whether the output should be in meV or cm^{-1}
            states  -   (True/False) whether the output should be states/unit energy if the sample is a pure element.
            ows     -   An output spectrum workspace (1D)
    """

    # Based on code in PySlice by Jon Taylor

    def category(self):
        return "Inelastic"

    def name(self):
        return "ComputeIncoherentDOS"

    def summary(self):
        return (
            "Calculates the neutron weighted generalised phonon density of states "
            + "in the incoherent approximation from a measured powder INS MatrixWorkspace"
        )

    def PyInit(self):
        validators = CompositeValidator()
        validators.add(HistogramValidator(True))
        validators.add(CommonBinsValidator())
        self.declareProperty(
            MatrixWorkspaceProperty(name="InputWorkspace", defaultValue="", direction=Direction.Input, validator=validators),
            doc="Input MatrixWorkspace containing the reduced inelastic neutron spectrum in (Q,E) or (2theta,E) space.",
        )
        self.declareProperty(
            name="Temperature", defaultValue=300.0, validator=FloatBoundedValidator(lower=0), doc="Sample temperature in Kelvin."
        )
        self.declareProperty(
            name="MeanSquareDisplacement",
            defaultValue=0.0,
            validator=FloatBoundedValidator(lower=0),
            doc="Average mean square displacement in Angstrom^2.",
        )
        self.declareProperty(name="QSumRange", defaultValue="0,Qmax", doc="Range in Q (in Angstroms^-1) to sum data over.")
        self.declareProperty(
            name="EnergyBinning",
            defaultValue="0,Emax/50,Emax*0.9",
            doc="Energy binning parameters [Emin, Emax] or [Emin, Estep, Emax] in meV.",
        )
        self.declareProperty(name="Wavenumbers", defaultValue=False, doc="Should the output be in Wavenumbers (cm^-1)?")
        self.declareProperty(
            name="StatesPerEnergy",
            defaultValue=False,
            doc="Should the output be in states per unit energy rather than mb/sr/fu/energy?\n"
            + "(Only for pure elements, need to set the sample material information)",
        )
        self.declareProperty(
            MatrixWorkspaceProperty(name="OutputWorkspace", defaultValue="", direction=Direction.Output), doc="Output workspace name."
        )
        self.declareProperty(
            name="TwoThetaSumRange", defaultValue="Twothetamin, Twothetamax", doc="Range in 2theta (in degrees) to sum data over."
        )

    def PyExec(self):
        inws = self.getProperty("InputWorkspace").value
        Temperature = self.getProperty("Temperature").value
        msd = self.getProperty("MeanSquareDisplacement").value
        QSumRange = self.getProperty("QSumRange").value
        EnergyBinning = self.getProperty("EnergyBinning").value
        cm = self.getProperty("Wavenumbers").value
        TwoThetaSumRange = self.getProperty("TwoThetaSumRange").value

        # Checks if the units are valid, and which way around the input workspace is
        # (Q or 2theta along horizontal or vertical axes).
        energy_axis_index = 0
        spectrum_axis_index = 1
        energy_unit = inws.getAxis(energy_axis_index).getUnit().unitID()
        spectrum_unit = inws.getAxis(spectrum_axis_index).getUnit().unitID()
        if energy_unit not in ["DeltaE", "DeltaE_inWavenumber"]:
            if spectrum_unit in ["DeltaE", "DeltaE_inWavenumber"]:
                # Input workspace is transposed.
                (energy_unit, spectrum_unit) = (spectrum_unit, energy_unit)
                energy_axis_index = 1
                spectrum_axis_index = 0
            else:
                raise UnitError()
        using_q = usingQ(spectrum_unit)

        eBins = inws.getAxis(energy_axis_index).extractValues()
        if energy_axis_index == 0 or len(eBins) != inws.getNumberHistograms():
            en = (eBins[1:] + eBins[:-1]) / 2

        # Gets meV to cm^-1 conversion
        mev2cm = (constants.elementary_charge / 1000) / (constants.h * constants.c * 100)
        # Gets meV to Kelvin conversion
        mev2k = (constants.elementary_charge / 1000) / constants.k

        # Converts energy to meV for bose factor later
        input_en_in_meV = energy_unit == "DeltaE"
        if not input_en_in_meV:
            en = en / mev2cm

        # Gets the incident energy from the workspace - either if it is set as an attribute or from the energy axis.
        try:
            ei = inws.getEFixed(1)
        except RuntimeError:
            ei = np.amax(en)
            self.log().warning("Could not find EFixed. Using {}mev".format(ei))
        if not input_en_in_meV:
            dosebin = evaluateEbin(np.amin(eBins * mev2cm), np.amax(eBins * mev2cm), ei * mev2cm, EnergyBinning)
        else:
            dosebin = evaluateEbin(np.amin(eBins), np.amax(eBins), ei, EnergyBinning)
        if len(dosebin) == 2:
            dosebin = binPreservingRebinninParams(eBins, dosebin[0], dosebin[-1])

        y = inws.extractY()
        e = inws.extractE()
        if spectrum_axis_index == 1:
            y = np.transpose(y)
            e = np.transpose(e)

        # Creates a grid the same size as the intensity (y) array populated by the q and energy values
        if using_q:
            qq = inws.getAxis(spectrum_axis_index).extractValues()
            spectrum_binning = evaluateQRange(np.amin(qq), np.amax(qq), QSumRange)
            if spectrum_axis_index == 0 or len(qq) != inws.getNumberHistograms():
                qq = (qq[1:] + qq[:-1]) / 2.0
            qqgrid = np.tile(qq, (np.shape(y)[0], 1))
        else:
            theta_axis = inws.getAxis(spectrum_axis_index)
            two_theta = theta_axis.extractValues()
            spectrum_binning = evaluateTwoThetaRange(np.amin(two_theta), np.amax(two_theta), TwoThetaSumRange)
            if spectrum_axis_index == 0 or len(two_theta) != inws.getNumberHistograms():
                two_theta = (two_theta[1:] + two_theta[:-1]) / 2.0
            two_theta = np.deg2rad(two_theta)
            qqgrid = qGridFromTwoTheta(two_theta, en, ei)

        engrid = np.transpose(np.tile(en, (np.shape(y)[1], 1)))
        # Calculates the Debye-Waller and Bose factors from the Temperature and mean-squared displacements
        qqgridsq = qqgrid**2
        DWF = np.exp(-(qqgridsq * msd))
        idm = np.where(engrid < 0)
        idp = np.where(engrid >= 0)
        # The expression for the population (Bose) factor and phonon energy dependence below actually refer to the phonon
        # energy (always defined to be positive) rather than the neutron energy transfer. So we need the absolute value here
        engrid = np.abs(engrid)
        expm = np.exp(-engrid[idm] * mev2k / Temperature)
        expp = np.exp(-engrid[idp] * mev2k / Temperature)
        Bose = np.empty_like(DWF)
        Bose[idm] = expm / (1 - expm)  # n energy gain, phonon annihilation
        Bose[idp] = expp / (1 - expp) + 1  # n energy loss, phonon creation
        # Calculates the density of states from S(q,w)
        f = (engrid / qqgridsq) / (Bose * DWF)
        y *= f
        e *= f

        # Checks if the user wants output in States/energy.
        atoms, _ = inws.sample().getMaterial().chemicalFormula()
        absunits = self.absoluteUnits(atoms)
        ylabel = yLabel(absunits, cm)

        # Convert to wavenumbers if requested (y-axis is in mbarns/sr/fu/meV -> mb/sr/fu/cm^-1)
        if cm:
            y /= mev2cm
            e /= mev2cm
            # yunit = 'DeltaE_inWavenumber'
        # else:
        # yunit = 'DeltaE'

        # Outputs the calculated density of states to another workspace
        dos2d = CloneWorkspace(inws, StoreInADS=False)
        if spectrum_axis_index == 1:
            dos2d = Transpose(dos2d, StoreInADS=False)
        for i in range(len(y)):
            dos2d.setY(i, y[i, :])
            dos2d.setE(i, e[i, :])
        dos2d.setYUnitLabel(ylabel)

        dos1d = cut1D(dos2d, qqgrid, spectrum_binning, dosebin)
        dos1d = scaleUnits(dos1d, cm, input_en_in_meV, mev2cm)

        if absunits:
            self.log().notice("Converting to states/energy")
            # cross-section information is given in barns, but data is in millibarns.
            sigma = atoms[0].neutron()["tot_scatt_xs"] * 1000
            mass = atoms[0].mass
            dos1d *= (mass / sigma) * 4 * np.pi

        self.setProperty("OutputWorkspace", dos1d)

    def absoluteUnits(self, atoms):
        absunits = self.getProperty("StatesPerEnergy").value
        if absunits and len(atoms) != 1:
            self.log().warning(
                "Sample material information not set, or sample is not a pure element. " "Ignoring StatesPerEnergy property."
            )
            absunits = False
        return absunits


AlgorithmFactory.subscribe(ComputeIncoherentDOS)
