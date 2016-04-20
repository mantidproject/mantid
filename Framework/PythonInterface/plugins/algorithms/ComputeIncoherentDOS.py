#pylint: disable = no-init, invalid-name, line-too-long, eval-used, unused-argument, too-many-locals, too-many-branches, too-many-statements
from mantid.kernel import *
from mantid.api import *
from mantid.simpleapi import *
import numpy as np

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
        self.declareProperty(name='Wavenumbers', defaultValue=False,
                             doc='Should the output be in Wavenumbers (cm^-1)?')
        self.declareProperty(name='StatesPerEnergy', defaultValue=False,
                             doc='Should the output be in states per unit energy rather than mb/sr/fu/energy?\n'+
                             '(Only for pure elements, need to set the sample material information)')
        self.declareProperty(MatrixWorkspaceProperty(name='OutputWorkspace', defaultValue='', direction=Direction.Output),
                             doc='Output workspace name.')

    def PyExec(self):
        inws = mtd[self.getPropertyValue('InputWorkspace')]
        Temperature = float(self.getPropertyValue('Temperature'))
        msd = float(self.getPropertyValue('MeanSquareDisplacement'))
        QSumRange = self.getPropertyValue('QSumRange')
        EnergyBinning = self.getPropertyValue('EnergyBinning')
        cm = int(self.getPropertyValue('Wavenumbers'))
        absunits = int(self.getPropertyValue('StatesPerEnergy'))

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
            raise ValueError('Only the variables ''Qmin'' and ''Qmax'' is allowed in QSumRange.')
        # Checks energy bins are ok.
        if EnergyBinning.count(',') != 2:
            raise ValueError('EnergyBinning must be a comma separated string with three values.')
        try:
            ei = inws.getEFixed(1)
        except RuntimeError:
            ei = max(en)
        try:
            # Do this in a function to make sure no other variables can be evaluated other than Emax and Ei.
            if cm:
                dosebin = evaluateEbin(min(en*8.066), max(en*8.066), ei*8.066, EnergyBinning)
            else:
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
        idm = np.where(engrid < 0)
        idp = np.where(engrid >= 0)
        expm = np.exp(-engrid[idm]*11.604/Temperature)
        expp = np.exp(-engrid[idp]*11.604/Temperature)
        Bose = DWF*0
        Bose[idm] = expm / (1 - expm)      # n energy gain, phonon annihilation
        Bose[idp] = expp / (1 - expp) + 1  # n energy loss, phonon creation
        # Calculates the density of states from S(q,w)
        y = y * (engrid/qqgrid**2) / (Bose*DWF)
        e = e * (engrid/qqgrid**2) / (Bose*DWF)

        # Checks if the user wants output in States/energy.
        atoms, _ = inws.sample().getMaterial().chemicalFormula()
        ylabel = 'g^{neutron}(E) (arb. units)'
        if absunits:
            if len(atoms) != 1:
                self.log().warning('Sample material information not set, or sample is not a pure element. Ignoring StatesPerEnergy property.')
                absunits = False
            else:
                if cm:
                    ylabel = 'g(E) (states/cm^-1)'
                else:
                    ylabel = 'g(E) (states/meV)'

        # Outputs the calculated density of states to another workspace
        # Convert to wavenumbers if requested (y-axis is in mbarns/sr/fu/meV -> mb/sr/fu/cm^-1)
        if cm:
            print "Converting to cm"
            en = en*8.066
            y = y/8.066
            e = e/8.066
            dos2d = CreateWorkspace(qq, y, e, Nspec=len(en), VerticalAxisUnit='DeltaE_inWavenumber', VerticalAxisValues=en,
                                    UnitX='MomentumTransfer', YUnitLabel=ylabel, WorkSpaceTitle='Density of States',
                                    ParentWorkspace=inws)
        else:
            dos2d = CreateWorkspace(qq, y, e, Nspec=len(en), VerticalAxisUnit='DeltaE', VerticalAxisValues=en,
                                    UnitX='MomentumTransfer', YUnitLabel=ylabel, WorkSpaceTitle='Density of States',
                                    ParentWorkspace=inws)

        # Make a 1D (energy dependent) cut
        dos1d = Rebin2D(dos2d, [dq[0], dq[1]-dq[0], dq[1]], dosebin, True, True)

        if absunits:
            print "Converting to states/energy"
            # cross-section information is given in barns, but data is in milibarns.
            sigma = atoms[0].neutron()['tot_scatt_xs'] * 1000
            mass = atoms[0].mass
            dos1d = dos1d * (mass/sigma)

        self.setProperty("OutputWorkspace", dos1d)
        DeleteWorkspace(dos2d)
        DeleteWorkspace(dos1d)

AlgorithmFactory.subscribe(ComputeIncoherentDOS)
