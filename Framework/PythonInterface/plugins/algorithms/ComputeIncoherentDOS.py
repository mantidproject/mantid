#pylint: disable = no-init, invalid-name, line-too-long, eval-used, unused-argument
from mantid.kernel import *
from mantid.api import *
from mantid.simpleapi import *
import numpy as np
from scipy import constants

def evaluateEbin(Emin, Emax, Ei, string):
    return [eval(estr) for estr in string.split(',')]

def evaluateQRange(Qmin, Qmax, string):
    return [eval(qstr) for qstr in string.split(',')]


class ComputeIncoherentDOS(PythonAlgorithm):
    """ Calculates the neutron weighted generalised phonon density of states in the incoherent approximation from a
        measured powder inelastic neutron scattering spectrum.

    ComputeIncoherentDOS(InputWorkspace = iws, Temperature = T, MeanSquareDisplacement = msd,
                         QSumRange = qrange, EnergyBinning = ebins, OutputWorkspace = ows)

    Input:
            iws     -   An input MatrixWorkspace (must contain the reduced powder INS data)
            T       -   The sample temperature in Kelvin (default: 300 K)
            msd     -   The average mean-squared displacement of all species in the sample in Angstrom^2 (default: 0.01A^2)
            qrange  -   Range in |Q| to integrate over (default: 0.5Qmax to Qmax)
            ebins   -   Energy bins in Energy transfer to rebin the data into (default: 0 to Emax in 50 steps)
            ows     -   An output spectrum workspace (1D)
    """

    # By Duc Le (2016)
    # Based on code in PySlice by Jon Taylor

    def category(self):
        return 'Inelastic;PythonAlgorithms'

    def name(self):
        return 'ComputeIncoherentDOS'

    def summary(self):
        return 'Calculates the neutron weighted generalised phonon density of states in the incoherent approximation from a measured powder INS MatrixWorkspace'

    def PyInit(self):
        validators = CompositeValidator()
        validators.add(HistogramValidator(True))
        validators.add(CommonBinsValidator())
        validators.add(WorkspaceUnitValidator('MomentumTransfer'))
        self.declareProperty(MatrixWorkspaceProperty(name='InputWorkspace', defaultValue='', direction=Direction.Input, validator=validators),
                             doc='Input MatrixWorkspace containing the reduced inelastic neutron spectrum in (Q,E) space.')
        self.declareProperty(name='Temperature', defaultValue=300., validator=FloatBoundedValidator(lower=0),
                             doc='Sample temperature in Kelvin.')
        self.declareProperty(name='MeanSquareDisplacement', defaultValue=0.01, validator=FloatBoundedValidator(lower=0),
                             doc='Average mean square displacement in Angstrom^2.')
        self.declareProperty(name='QSumRange', defaultValue='Qmax/2,Qmax',
                             doc='Range in |Q| (in Angstroms^-1) to sum data over.')
        self.declareProperty(name='EnergyBinning', defaultValue='0,Emax/50,Emax',
                             doc='Energy binning parameters [Emin, Estep, Emax] in meV.')
        self.declareProperty(name='SampleFormula', defaultValue='',
                             doc='Sample chemical formula, with spaces between elements. E.g. "Al2 O3"')
        self.declareProperty(MatrixWorkspaceProperty(name='OutputWorkspace', defaultValue='', direction=Direction.Output),
                             doc='Output workspace name.')

    def PyExec(self):
        inws = mtd[self.getPropertyValue('InputWorkspace')]
        Temperature = float(self.getPropertyValue('Temperature'))
        msd = float(self.getPropertyValue('MeanSquareDisplacement'))
        QSumRange = self.getPropertyValue('QSumRange')
        EnergyBinning = self.getPropertyValue('EnergyBinning')
        SampleFormula = self.getPropertyValue('SampleFormula')

        # (Mantid stores the bin boundaries by default rather than bin centers)
        qq = inws.getAxis(0).extractValues()
        qq = (qq[1:len(qq)]+qq[0:len(qq)-1])/2
        en = inws.getAxis(1).extractValues()
        en = (en[1:len(en)]+en[0:len(en)-1])/2

        # Checks qrange is valid
        if QSumRange.count(',') != 1:
            raise ValueError('QSumRange must be a comma separated string with two values.')
        try:
            # Do this in a member function to make sure no other variables can be evaluated other than Emax and Ei.
            dq = evaluateQRange(min(qq), max(qq), QSumRange)
        except NameError:
            raise ValueError('Only the variable ''Qmin'' or ''Qmax'' are allowed in QSumRange.')
        # Checks energy bins are ok.
        if EnergyBinning.count(',') != 2:
            raise ValueError('EnergyBinning must be a comma separated string with three values.')
        try:
            ei = inws.getEFixed(1)
        except RuntimeError:
            ei = max(en)
        try:
            # Do this in a function to make sure no other variables can be evaluated other than Emax and Ei.
            dosebin = evaluateEbin(min(en), max(en), ei, EnergyBinning)
        except NameError:
            raise ValueError('Only the variables ''Emin'', ''Emax'' or ''Ei'' are allowed in EnergyBinning.')

        # Extracts the intensity (y) and errors (e) from inws.
        y = inws.extractY()
        e = inws.extractE()
        # Creates a grid the same size as the intensity (y) array populated by the q and energy values
        qqgrid = np.tile(np.array(qq), (np.shape(y)[0], 1))
        engrid = np.transpose(np.tile(np.array(en), (np.shape(y)[1], 1)))

        # Calculates the Debye-Waller and Bose factors from the Temperature and mean-squared displacements
        DWF = np.exp(-2*(qqgrid**2*msd))
        idm = np.where(engrid<0)
        idp = np.where(engrid>=0)
        expm = np.exp(-engrid[idm]*11.604/Temperature)
        expp = np.exp(-engrid[idp]*11.604/Temperature)
        Bose = DWF*0
        Bose[idm] = expm / (1 - expm)      # n energy gain, phonon annihilation
        Bose[idp] = expp / (1 - expp) + 1  # n energy loss, phonon creation
        # Calculates the density of states from S(q,w)
        y = y * (engrid/qqgrid**2) / (Bose*DWF)
        e = e * (engrid/qqgrid**2) / (Bose*DWF)
        # Outputs the calculated density of states to another workspace
        dos2d = CreateWorkspace(qq, y, e, Nspec=len(en), VerticalAxisUnit='DeltaE', VerticalAxisValues=en,
                                UnitX='MomentumTransfer', YUnitLabel=ylab, WorkSpaceTitle='Density of States',
                                ParentWorkspace=inws)
        # Make a 1D (energy dependent) cut
        dos1d = Rebin2D(dos2d, [dq[0], dq[1]-dq[0], dq[1]], dosebin, True, True)

        self.setProperty("OutputWorkspace", dos1d)
        DeleteWorkspace(dos2d)
        DeleteWorkspace(dos1d)

AlgorithmFactory.subscribe(ComputeIncoherentDOS)
