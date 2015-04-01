#pylint: disable=invalid-name,no-init
from mantid.kernel import *
from mantid.api import *
from mantid.simpleapi import *
import numpy as np
import __builtin__


class CreateMD(DataProcessorAlgorithm):

    def _possible_emodes(self):
        return ['Elastic', 'Direct', 'Indirect']

    def _add_sample_log(self, workspace, log_name, log_number):
        add_log = self.createChildAlgorithm('AddSampleLog')
        add_log.setProperty('Workspace', workspace)
        add_log.setProperty('LogName', str(log_name))
        add_log.setProperty('LogText', str(log_number))
        add_log.setProperty('LogType', 'Number')
        add_log.execute()

    def _set_goniometer(self, workspace):

        axis0 =  ','.join(map(str, ['gl', 0, 0, 1, 1]))
        axis1 =  ','.join(map(str, ['gs', 1, 0, 0, 1]))
        axis2 =  ','.join(map(str, ['psi', 0, 1, 0, 1]))

        set_goniometer = self.createChildAlgorithm('SetGoniometer')
        set_goniometer.setProperty('Workspace', workspace)
        set_goniometer.setProperty('Axis0', axis0)
        set_goniometer.setProperty('Axis1', axis1)
        set_goniometer.setProperty('Axis2', axis2)
        set_goniometer.execute()

    def _set_ub(self, workspace, a, b, c, alpha, beta, gamma, u, v):
        set_ub = self.createChildAlgorithm('SetUB')
        set_ub.setProperty('Workspace', workspace)
        set_ub.setProperty('a', a)
        set_ub.setProperty('b', b)
        set_ub.setProperty('c', c)
        set_ub.setProperty('alpha', alpha)
        set_ub.setProperty('beta', beta)
        set_ub.setProperty('gamma', gamma)
        set_ub.setProperty('u', u)
        set_ub.setProperty('v', v)
        set_ub.execute()

    def _convert_to_md(self, workspace, analysis_mode):

        # Find the Min Max extents
        min_max_alg = self.createChildAlgorithm('ConvertToMDMinMaxGlobal')
        min_max_alg.setProperty('InputWorkspace', workspace)
        min_max_alg.setProperty('QDimensions', 'Q3D')
        min_max_alg.setProperty('dEAnalysisMode', analysis_mode)
        min_max_alg.execute()
        min_values = min_max_alg.getProperty('MinValues').value
        max_values = min_max_alg.getProperty('MaxValues').value

        # Convert to MD
        convert_alg = self.createChildAlgorithm('ConvertToMD')
        convert_alg.setProperty('InputWorkspace', workspace)
        convert_alg.setPropertyValue('OutputWorkspace', 'dummy')
        convert_alg.setProperty('QDimensions', 'Q3D')
        convert_alg.setProperty('QConversionScales', 'HKL')
        convert_alg.setProperty('dEAnalysisMode', analysis_mode)
        convert_alg.setProperty('MinValues', min_values)
        convert_alg.setProperty('MaxValues', max_values)
        convert_alg.execute()
        return convert_alg.getProperty('OutputWorkspace').value

    def _merge_runs(self, to_merge):

        merge_alg = self.createChildAlgorithm('MergeMD')
        merge_alg.setProperty('InputWorkspaces', to_merge)
        merge_alg.setPropertyValue('OutputWorkspace', 'dummy')
        merge_alg.execute()
        return merge_alg.getProperty('OutputWorkspace').value

    def _single_run(self, input_workspace, emode,  psi, gl, gs, alatt=None, angdeg=None, u=None, v=None,):
        import numpy as np
        ub_params = map(any, [alatt, angdeg, u, v])
        goniometer_params = [psi, gl, gs]
        if any(ub_params) and not all(ub_params):
            raise ValueError("Either specify all of alatt, angledeg, u, v or none of them")
        elif all(ub_params):
            if input_workspace.sample().hasOrientedLattice():
                logger.warning("Sample already has a UB. This will not be overwritten by %s. Use ClearUB and re-run."%self.name())
            else:
                self._set_ub(workspace=input_workspace, a=alatt[0], b=alatt[1], c=alatt[2], alpha=angdeg[0], beta=angdeg[1], gamma=angdeg[2], u=u, v=v)

        if any(goniometer_params):
            self._add_sample_log(workspace=input_workspace, log_name='gl', log_number=gl)
            self._add_sample_log(workspace=input_workspace, log_name='gs', log_number=gs)
            self._add_sample_log(workspace=input_workspace, log_name='psi', log_number=psi)
            self._set_goniometer(workspace=input_workspace)
        
        output_run = self._convert_to_md(workspace=input_workspace, analysis_mode=emode)
        return output_run
    

    def category(self):
        return 'MDAlgorithms'

    def summary(self):
        return 'Creates a mutlidimensional workspace by transforming and combining individual runs.'

    def PyInit(self):
        self.declareProperty(StringArrayProperty('InputWorkspaces',  values=[], direction=Direction.Input, validator=StringArrayMandatoryValidator()),
                             doc='Matrix workspace to slice')

        self.declareProperty('Emode', defaultValue='Direct', validator=StringListValidator(self._possible_emodes()), direction=Direction.Input, doc='Analysis mode ' + str(self._possible_emodes()) )

        self.declareProperty(FloatArrayProperty('Alatt', values=[], validator=FloatArrayMandatoryValidator(), direction=Direction.Input ), doc='Lattice parameters' )

        self.declareProperty(FloatArrayProperty('Angdeg', values=[], validator=FloatArrayMandatoryValidator(), direction=Direction.Input ), doc='Lattice angles' )

        self.declareProperty(FloatArrayProperty('U', values=[], validator=FloatArrayMandatoryValidator(), direction=Direction.Input ), doc='Lattice vector parallel to neutron beam' )

        self.declareProperty(FloatArrayProperty('V', values=[], validator=FloatArrayMandatoryValidator(), direction=Direction.Input ), doc='Lattice vector perpendicular to neutron beam in the horizontal plane' )

        self.declareProperty(FloatArrayProperty('Psi', values=[], direction=Direction.Input), doc='Psi rotation in degrees. Optional or one entry per run.' )

        self.declareProperty(FloatArrayProperty('Gl', values=[], direction=Direction.Input), doc='gl rotation in degrees. Optional or one entry per run.' )

        self.declareProperty(FloatArrayProperty('Gs', values=[], direction=Direction.Input), doc='gs rotation in degrees. Optional or one entry per run.' )

        self.declareProperty(IMDWorkspaceProperty('OutputWorkspace', '', direction=Direction.Output ), doc='Output MDWorkspace')

    def _validate_inputs(self):
    
        emode = self.getProperty('Emode').value
        alatt = self.getProperty('Alatt').value
        angdeg = self.getProperty('Angdeg').value
        u = self.getProperty('U').value
        v = self.getProperty('V').value
        psi = self.getProperty('Psi').value
        gl = self.getProperty('Gl').value
        gs = self.getProperty('Gs').value        

        input_workspaces = self.getProperty("InputWorkspaces").value
        
        ws_entries = len(input_workspaces)
        
        if ws_entries < 1:
            raise ValueError("Need one or more input workspace")
            
        if len(u) != 3:
            raise ValueError("u must have 3 components")
            
        if len(v) != 3:
            raise ValueError("v must have 3 components")
            
        if len(alatt) != 3:
            raise ValueError("lattice parameters must have 3 components")
            
        if len(angdeg) != 3:
            raise ValueError("Angle must have 3 components")

        if not emode in self._possible_emodes():
            raise ValueError("Unknown emode %s Allowed values are %s" % (emode, self._possible_emodes()))
            
        if len(psi) > 0 and len(psi) != ws_entries:
            raise ValueError("If Psi is given a entry should be provided for every input workspace")
        
        if len(gl) > 0 and len(gl) != ws_entries:
            raise ValueError("If Gl is given a entry should be provided for every input workspace")
            
        if len(gs) > 0 and len(gs) != ws_entries:
            raise ValueError("If Gs is given a entry should be provided for every input workspace")
         
            
    def PyExec(self):

        logger.warning('You are running algorithm %s that is the beta stage of development' % (self.name()))

        emode = self.getProperty('Emode').value
        alatt = self.getProperty('Alatt').value
        angdeg = self.getProperty('Angdeg').value
        u = self.getProperty('U').value
        v = self.getProperty('V').value
        psi = self.getProperty('Psi').value
        gl = self.getProperty('Gl').value
        gs = self.getProperty('Gs').value        
        
        input_workspaces = self.getProperty("InputWorkspaces").value
        
        ws_entries = len(input_workspaces)
        
        self._validate_inputs()
            
        if len(psi) == 0:
            psi = [None] * ws_entries
            
        if len(gl) == 0:
            gl = [None] * ws_entries
            
        if len(gs) == 0:
            gs = [None] * ws_entries
        
        output_workspace = None
        run_md = None

        to_merge_names = list()
        
        run_data = zip(input_workspaces, psi, gl, gs)
        for run_entry in run_data:
                ws_name, psi_entry, gl_entry, gs_entry = run_entry
                ws = AnalysisDataService.retrieve(ws_name)
                run_md = self._single_run(input_workspace=ws, emode=emode, alatt=alatt, angdeg=angdeg, u=u, v=v, psi=psi_entry, gl=gl_entry, gs=gs_entry)
                to_merge_name = ws_name + "_md"
                AnalysisDataService.addOrReplace(to_merge_name, run_md)
                to_merge_names.append(to_merge_name)

        if len(to_merge_names) > 1:
            output_workspace = self._merge_runs(to_merge_names)
        else:
            output_workspace = AnalysisDataService.retrieve(to_merge_names[0])

        # Clear out temporary workspaces.
        for ws in to_merge_names:
            DeleteWorkspace(ws)

        self.setProperty("OutputWorkspace", output_workspace)
                







AlgorithmFactory.subscribe(CreateMD)
