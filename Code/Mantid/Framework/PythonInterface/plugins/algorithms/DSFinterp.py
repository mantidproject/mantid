#pylint: disable=no-init,invalid-name
from mantid.api import PythonAlgorithm, AlgorithmFactory
from mantid.simpleapi import CloneWorkspace, mtd
from mantid.kernel import StringListValidator, FloatArrayProperty, FloatArrayMandatoryValidator,\
    StringArrayProperty, StringArrayMandatoryValidator, Direction, logger, EnabledWhenProperty, PropertyCriterion

class DSFinterp(PythonAlgorithm):

    channelgroup = None

    def category(self):
        return "Transforms\\Smoothing;Utility;PythonAlgorithms"

    def name(self):
        return 'DSFinterp'

    def summary(self):
        return 'Given a set of parameter values {Ti} and corresponding structure factors S(Q,E,Ti), this algorithm '\
        'interpolates S(Q,E,T) for any value of parameter T within the range spanned by the {Ti} set.'

    def PyInit(self):
        arrvalidator = StringArrayMandatoryValidator()
        lrg='Input'
        self.declareProperty(StringArrayProperty('Workspaces', values=[], validator=arrvalidator,\
            direction=Direction.Input), doc='list of input workspaces')
        self.declareProperty('LoadErrors', True, direction=Direction.Input,\
            doc='Do we load error data contained in the workspaces?')
        self.declareProperty(FloatArrayProperty('ParameterValues', values=[],\
            validator=FloatArrayMandatoryValidator(),direction=Direction.Input), doc='list of input parameter values')
        self.setPropertyGroup('Workspaces', lrg)
        self.setPropertyGroup('LoadErrors', lrg)
        self.setPropertyGroup('ParameterValues', lrg)

        self.declareProperty('LocalRegression', True, direction=Direction.Input, doc='Perform running local-regression?')
        condition = EnabledWhenProperty("LocalRegression", PropertyCriterion.IsDefault)
        self.declareProperty('RegressionWindow', 6, direction=Direction.Input, doc='window size for the running local-regression')
        self.setPropertySettings("RegressionWindow", condition)
        regtypes = [ 'linear', 'quadratic']
        self.declareProperty('RegressionType', 'quadratic', StringListValidator(regtypes),\
            direction=Direction.Input, doc='type of local-regression; linear and quadratic are available')
        self.setPropertySettings("RegressionType", condition)
        lrg = 'Running Local Regression Options'
        self.setPropertyGroup('LocalRegression', lrg)
        self.setPropertyGroup('RegressionWindow', lrg)
        self.setPropertyGroup('RegressionType', lrg)

        lrg='Output'
        self.declareProperty(FloatArrayProperty('TargetParameters', values=[], ), doc="Parameters to interpolate the structure factor")
        self.declareProperty(StringArrayProperty('OutputWorkspaces', values=[], validator=arrvalidator),\
            doc='list of output workspaces to save the interpolated structure factors')
        self.setPropertyGroup('TargetParameters', lrg)
        self.setPropertyGroup('OutputWorkspaces', lrg)
        self.channelgroup = None

    def areWorkspacesCompatible(self, a, b):
        sizeA = a.blocksize() * a.getNumberHistograms()
        sizeB = b.blocksize() * b.getNumberHistograms()
        return sizeA == sizeB

    def PyExec(self):
    # Check congruence of workspaces
        workspaces = self.getProperty('Workspaces').value
        fvalues = self.getProperty('ParameterValues').value
        if len(workspaces) != len(fvalues):
            mesg = 'Number of Workspaces and ParameterValues should be the same'
      #logger.error(mesg)
            raise IndexError(mesg)
        for workspace in workspaces[1:]:
            if not self.areWorkspacesCompatible(mtd[workspaces[0]],mtd[workspace]):
                mesg = 'Workspace {0} incompatible with {1}'.format(workspace, workspaces[0])
                logger.error(mesg)
                raise ValueError(mesg)
    # Load the workspaces into a group of dynamic structure factors
        from dsfinterp.dsf import Dsf
        from dsfinterp.dsfgroup import DsfGroup
        from dsfinterp.channelgroup import ChannelGroup
        dsfgroup = DsfGroup()
        for idsf in range(len(workspaces)):
            dsf = Dsf()
            dsf.Load( mtd[workspaces[idsf]] )
            if not self.getProperty('LoadErrors').value:
                dsf.errors = None # do not incorporate error data
            dsf.SetFvalue( fvalues[idsf] )
            dsfgroup.InsertDsf(dsf)
    # Create the intepolator if not instantiated before
        if not self.channelgroup:
            self.channelgroup = ChannelGroup()
            self.channelgroup.InitFromDsfGroup(dsfgroup)
            localregression = self.getProperty('LocalRegression').value
            if localregression:
                regressiontype = self.getProperty('RegressionType').value
                windowlength = self.getProperty('RegressionWindow').value
                self.channelgroup.InitializeInterpolator(running_regr_type=regressiontype, windowlength=windowlength)
            else:
                self.channelgroup.InitializeInterpolator(windowlength=0)
    # Invoke the interpolator and generate the output workspaces
        targetfvalues = self.getProperty('TargetParameters').value
        for targetfvalue in targetfvalues:
            if targetfvalue < min(fvalues) or targetfvalue > max(fvalues):
                mesg = 'Target parameters should lie in [{0}, {1}]'.format(min(fvalues),max(fvalues))
                logger.error(mesg)
                raise ValueError(mesg)
        outworkspaces = self.getProperty('OutputWorkspaces').value
        if len(targetfvalues) != len(outworkspaces):
            mesg = 'Number of OutputWorkspaces and TargetParameters should be the same'
            logger.error(mesg)
            raise IndexError(mesg)
        for i in range(len(targetfvalues)):
            outworkspace = outworkspaces[i]
            dsf = self.channelgroup( targetfvalues[i] )
            outws = CloneWorkspace( mtd[workspaces[0]], OutputWorkspace=outworkspaces[i])
            dsf.Save(outws) # overwrite dataY and dataE

#############################################################################################
#pylint: disable=unused-import
try:
    import dsfinterp
    AlgorithmFactory.subscribe(DSFinterp)
except:
    logger.debug('Failed to subscribe algorithm DSFinterp; Python package dsfinterp'\
        'may be missing (https://pypi.python.org/pypi/dsfinterp)')

