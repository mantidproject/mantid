#pylint: disable=invalid-name,no-init
from mantid.kernel import *
from mantid.api import *
from mantid.simpleapi import *
import numpy as np
import __builtin__


class CreateMD(DataProcessorAlgorithm):

    def __possible_emodes(self):
        return ['Elastic', 'Direct', 'Indirect']

    def __single_run(self, input_workspace, emode, alatt=[], angdeg=[], u=[], v=[], psi=None, gl=None, gs=None):
        import numpy as np
        ub_params = map(any, [alatt, angdeg, u, v])
        goniometer_params = [psi, gl, gs]
        if any(ub_params) and not all(ub_params):
            raise ValueError("Either specify all of alatt, angledeg, u, v or none of them")
        elif all(ub_params):
            SetUB(Workspace=input_workspace, a=alatt[0], b=alatt[1], c=alatt[2], alpha=angdeg[0], beta=angdeg[1], gamma=angdeg[2], u=u, v=v)
        if any(goniometer_params) and not all(goniometer_params):
            raise ValueError("Either specify all of psi, gl, gs or none of them")
        elif all(goniometer_params):
            if input_workspace.sample().hasOrientedLattice():
                self.g_log.warning("Goniometer has already been set. It will not be overwritten by %s ", self.name())
            else:
                AddSampleLog(Workspace=input_workspace, LogName='gl', LogText=gl, LogType='Number')
                AddSampleLog(Workspace=input_workspace, LogName='gs', LogText=gs, LogType='Number')
                AddSampleLog(Workspace=input_workspace, LogName='psi', LogText=psi, LogType='Number')
                
                axis0 = 'gl' + ','.join(map(str, [0,0,1]))
                axis1 = 'gs' + ','.join(map(str, [1,0,0]))
                axis2 = 'psi' + ','.join(map(str, [0,1,0]))
                
                SetGoniometer(Workspace=input_workspace, Axis0=axis0, Axis1=axis1, Axis2=axis2)
        
        min_extents, max_extents = ConvertToMDMinMaxLocal(InputWorkspace=input_workspace,QDimensions='Q3D',dEAnalysisMode=emode)
        output_run = ConvertToMD(InputWorkspace=input_workspace, QDimensions='Q3D', QConversionScales='HKL',dEAnalysisMode=emode, MinValues=min_extents, MaxValues=max_extents)
        return output_run

    def category(self):
        return 'MDAlgorithms'

    def summary(self):
        return 'Creates a mutlidimensional workspace by transforming and combining individual runs.'

    def PyInit(self):
        self.declareProperty(StringArrayProperty('InputWorkspaces',  values=[], direction=Direction.Input, validator=StringArrayMandatoryValidator()),
                             doc='Matrix workspace to slice')

        self.declareProperty('Emode', defaultValue='Direct', validator=StringListValidator(self.__possible_emodes()), direction=Direction.Input, doc='Analysis mode ' + str(self.__possible_emodes()) )

        self.declareProperty(FloatArrayProperty('Alatt', values=[], validator=FloatArrayMandatoryValidator(), direction=Direction.Input ), doc='Lattice parameters' )

        self.declareProperty(FloatArrayProperty('Angdeg', values=[], validator=FloatArrayMandatoryValidator(), direction=Direction.Input ), doc='Lattice angles' )

        self.declareProperty(FloatArrayProperty('u', values=[], validator=FloatArrayMandatoryValidator(), direction=Direction.Input ), doc='Lattice vector parallel to neutron beam' )

        self.declareProperty(FloatArrayProperty('v', values=[], validator=FloatArrayMandatoryValidator(), direction=Direction.Input ), doc='Lattice vector perpendicular to neutron beam in the horizontal plane' )

        self.declareProperty('psi', defaultValue=0.0, direction=Direction.Input, doc='Psi rotation in degrees' )

        self.declareProperty('gl', defaultValue=0.0, direction=Direction.Input, doc='gl rotation in degrees' )

        self.declareProperty('gs', defaultValue=0.0, direction=Direction.Input, doc='gs rotation in degrees' )

        self.declareProperty(IMDWorkspaceProperty('OutputWorkspace', '', direction=Direction.Output ), doc='Output MDWorkspace')

    def PyExec(self):

        logger.warning('You are running algorithm %s that is the beta stage of development' % (self.name()))

        emode = self.getProperty('Emode').value
        alatt = self.getProperty('Alatt').value
        angdeg = self.getProperty('Angdeg').value
        u = self.getProperty('u').value
        v = self.getProperty('v').value
        psi = self.getProperty('psi').value
        gl = self.getProperty('gl').value
        gs = self.getProperty('gs').value 

        input_workspaces = self.getProperty("InputWorkspaces").value
        if len(input_workspaces) < 1:
            raise ValueError("Need one or more input workspace")

        if not emode in self.__possible_emodes():
            raise ValueError("Unknown emode %s Allowed values are %s" % (emode, self.__possible_emodes()))
    
        output_workspace = None
        run_md = None
        for ws in input_workspaces:
                run_md = self.__single_run(input_workspace=ws, emode=emode, alatt=alatt, angdeg=angdeg, u=u, v=v, psi=psi, gl=gl, gs=gs)
                
                if not output_workspace:
                    output_workspace = run_md.rename()
                else:
                    print output_workspace.name()
                    print run_md.name()
                    output_workspace += run_md # Accumulate results via PlusMD. TODO, will need to find the best performance method for doing this.

        self.setProperty("OutputWorkspace", output_workspace)
                







AlgorithmFactory.subscribe(CreateMD)
