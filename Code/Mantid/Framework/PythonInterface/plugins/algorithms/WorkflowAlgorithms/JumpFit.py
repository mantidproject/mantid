from mantid.kernel import *
from mantid.api import *
from mantid.simpleapi import *
from mantid import logger, mtd
from IndirectCommon import *
from IndirectImport import import_mantidplot

import os.path


class JumpFit(PythonAlgorithm):


    def category(self):
        return 'Workflow\\Inelastic;PythonAlgorithms;Inelastic'


    def summary(self):
        return ''  ##TODO


    def PyInit(self):
        self.declareProperty(WorkspaceProperty('InputWorkspace', '', direction=Direction.Input),
                doc='Input workspace')

        valid_functions = ['ChudleyElliot', 'HallRoss', 'FickDiffusion', 'TeixeiraWater']
        self.declareProperty(name='JumpFunction', defaultValue=valid_functions[0],
                             validator=StringListValidator(valid_functions),
                             doc='')  ##TODO

        self.declareProperty(name='Width', defaultValue=0, validator=IntMandatoryValidator(), doc='')  ##TODO
        self.declareProperty(name='QMin', defaultValue=0.0, validator=FloatMandatoryValidator(), doc='')  ##TODO
        self.declareProperty(name='QMax', defaultValue=0.0, validator=FloatMandatoryValidator(), doc='')  ##TODO

        self.declareProperty(name='Output', defaultValue='', direction=Direction.InOut,
                doc='Output name')

        self.declareProperty(name='Verbose', defaultValue=False, doc='Output more verbose message to log')
        self.declareProperty(name='Plot', defaultValue=False, doc='Plot result workspace')
        self.declareProperty(name='Save', defaultValue=False, doc='Save result workspace to nexus file in the default save directory')


    def PyExec(self):
        in_ws = self.getPropertyValue('InputWorkspace')
        out_name = self.getPropertyValue('Output')

        jump_function = self.getProperty('JumpFunction').value
        width = self.getProperty('Width').value
        q_min = self.getProperty('QMin').value
        q_max = self.getProperty('QMax').value

        verbose = self.getProperty('Verbose').value
        plot = self.getProperty('Plot').value
        save = self.getProperty('Save').value

        workdir = getDefaultWorkingDirectory()

        StartTime('Jump fit : ' + jump_function + ' ; ')

        # Select the width we wish to fit
        spectrum_ws = "__" + in_ws
        ExtractSingleSpectrum(InputWorkspace=in_ws, OutputWorkspace=spectrum_ws, WorkspaceIndex=width)

        # Convert to HWHM
        Scale(InputWorkspace=spectrum_ws, Factor=0.5, OutputWorkspace=spectrum_ws)

        # Crop the workspace between the given ranges
        if verbose:
            logger.notice('Cropping from Q= ' + str(q_min) + ' to ' + str(q_max))

        # Give the user some extra infromation if required
        if verbose:
            in_run = mtd[in_ws].getRun()
            try:
                log = in_run.getLogData('fit_program')
                if log:
                    val = log.value
                    logger.notice('Fit program was : ' + val)
            except RuntimeError:
                # If we couldn't find the fit program, just pass
                pass

            logger.notice('Parameters in ' + in_ws)

        x_data = mtd[in_ws].readX(0)
        xmax = x_data[-1]

        # Select fit function to use
        if jump_function == 'ChudleyElliot':
            # Chudley-Elliott: HWHM=(1-sin*(Q*L)/(Q*L))/Tau
            # for Q->0 W=Q^2*L^2/(6*Tau)

            tval = 1.0 / xmax
            lval = 1.5
            func = 'name=ChudleyElliot, Tau=' + str(tval) + ', L=' + str(lval)

        elif jump_function == 'HallRoss':
            # Hall-Ross: HWHM=(1-exp(-L*Q^2))/Tau
            # for Q->0 W=A*Q^2*r

            tval = 1.0 / xmax
            lval = 1.5
            func = 'name=HallRoss, Tau=' + str(tval) + ', L=' + str(lval)

        elif jump_function == 'FickDiffusion':
            # Fick: HWHM=D*Q^2

            y_data = mtd[in_ws].readY(0)
            diff = (y_data[2] - y_data[0]) / ((x_data[2] - x_data[0]) * (x_data[2] - x_data[0]))
            func = 'name=FickDiffusion, D=' + str(diff)

        elif jump_function == 'TeixeiraWater':
            # Teixeira: HWHM=Q^2*L/((1+Q^2*L)*tau)
            # for Q->0 W=

            tval = 1.0 / xmax
            lval = 1.5
            func = 'name=TeixeiraWater, Tau=' + str(tval) + ', L=' + str(lval)

        # Run fit function
        if out_name is "":
            out_name = in_ws[:-10] + '_' + jump_function + 'fit'

        Fit(Function=func, InputWorkspace=spectrum_ws, CreateOutput=True, Output=out_name, StartX=q_min, EndX=q_max)
        fit_workspace = out_name + '_Workspace'

        # Populate sample logs
        CopyLogs(InputWorkspace=in_ws, OutputWorkspace=fit_workspace)
        AddSampleLog(Workspace=fit_workspace, LogName="jump_function", LogType="String", LogText=jump_function)
        AddSampleLog(Workspace=fit_workspace, LogName="q_min", LogType="Number", LogText=str(q_min))
        AddSampleLog(Workspace=fit_workspace, LogName="q_max", LogType="Number", LogText=str(q_max))

        # Process output options
        if save:
            fit_path = os.path.join(workdir, fit_workspace + '.nxs')
            SaveNexusProcessed(InputWorkspace=fit_workspace, Filename=fit_path)

            if verbose:
                logger.notice('Fit file is ' + fit_path)

        if plot:
            mtd_plot = import_mantidplot()
            mtd_plot.plotSpectrum(fit_workspace, [0, 1, 2], True)

        self.setProperty('Output', out_name)

        DeleteWorkspace(Workspace=spectrum_ws)

        EndTime('Jump fit : ' + jump_function + ' ; ')


# Register algorithm with Mantid
AlgorithmFactory.subscribe(JumpFit)
