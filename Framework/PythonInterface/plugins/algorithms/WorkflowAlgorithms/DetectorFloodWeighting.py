from mantid.api import DataProcessorAlgorithm, AlgorithmFactory, MatrixWorkspaceProperty, WorkspaceUnitValidator, \
    Progress

from mantid.kernel import Direction, FloatArrayProperty, FloatArrayBoundedValidator

import numpy as np

class DetectorFloodWeighting(DataProcessorAlgorithm):

    def __init__(self):
        DataProcessorAlgorithm.__init__(self)

    def category(self):
        return 'Workflow\\SANS'


    def summary(self):
        return 'Generates a Detector flood weighting, or sensitivity workspace'


    def PyInit(self):

        self.declareProperty(MatrixWorkspaceProperty('InputWorkspace', '',
                                                     direction=Direction.Input, validator=WorkspaceUnitValidator("Wavelength")),
                             doc='Flood weighting measurement')

        validator = FloatArrayBoundedValidator()
        validator.setLower(0.)
        self.declareProperty(FloatArrayProperty('Bands', [], direction=Direction.Input, validator=validator),
                             doc='Wavelength bands to use. Single pair min to max.')

        self.declareProperty(MatrixWorkspaceProperty('OutputWorkspace', '',
                                                     direction=Direction.Output),
                             doc='Normalized flood weighting measurement')

        self.declareProperty("SolidAngleCorrection", True, direction=Direction.Input, doc="Perform final solid angle correction")


    def validateInputs(self):
        """
        Validates input ranges.
        """
        issues = dict()

        bands = self.getProperty('Bands').value

        if not any(bands):
            issues['Bands'] = 'Bands must be supplied'
            return issues # Abort early. Do not continue

        if not len(bands)%2 == 0:
            issues['Bands'] = 'Even number of Bands boundaries expected'
            return issues # Abort early. Do not continue

        all_limits=list()
        for i in range(0, len(bands), 2):
            lower = bands[i]
            upper = bands[i+1]
            limits = np.arange(lower, upper)
            unique = set(limits)
            for existing_lims in all_limits:
                if unique.intersection(set(existing_lims)):
                    issues['Bands'] = 'Bands must not intersect'
                    break

            all_limits.append(limits)
            if lower >= upper:
                issues['Bands'] = 'Bands should form lower, upper pairs'

        return issues

    def _divide(self, lhs, rhs):
        divide = self.createChildAlgorithm("Divide")
        divide.setProperty("LHSWorkspace", lhs)
        divide.setProperty("RHSWorkspace", rhs)
        divide.execute()
        return divide.getProperty("OutputWorkspace").value
        
    def _add(self, lhs, rhs):
        divide = self.createChildAlgorithm("Plus")
        divide.setProperty("LHSWorkspace", lhs)
        divide.setProperty("RHSWorkspace", rhs)
        divide.execute()
        return divide.getProperty("OutputWorkspace").value


    def PyExec(self):

        progress = Progress(self, 0, 1, 4) # Four coarse steps

        in_ws = self.getProperty('InputWorkspace').value
        bands = self.getProperty('Bands').value
        x_range = max(bands)-min(bands)
        sum_steps = 0

        # Formulate bands, integrate and sum
        accumulated_output = None
        for i in range(0, len(bands), 2):
            lower = bands[i]
            upper = bands[i+1]
            step = upper - lower
            sum_steps += step
            
            rebin = self.createChildAlgorithm("Rebin")
            rebin.setProperty("Params", [lower, step, upper])
            rebin.setProperty("InputWorkspace", in_ws) # Always integrating the same input workspace
            rebin.execute()
            integrated = rebin.getProperty("OutputWorkspace").value
            
            if accumulated_output:
                accumulated_output = self._add(accumulated_output, integrated)
            else:
                # First band
                accumulated_output = integrated
            
        progress.report()


        # Perform solid angle correction. Calculate solid angle then divide through.
        normalized=accumulated_output
        if self.getProperty("SolidAngleCorrection").value:
            solidAngle = self.createChildAlgorithm("SolidAngle")
            solidAngle.setProperty("InputWorkspace", accumulated_output)
            solidAngle.execute()
            solid_angle_weighting = solidAngle.getProperty("OutputWorkspace").value
            normalized = self._divide(normalized, solid_angle_weighting)
        progress.report()
        
        # Determine the max across all spectra
        y_values = normalized.extractY()
        mean_val = np.mean(y_values)

        # Create a workspace from the single max value
        create = self.createChildAlgorithm("CreateSingleValuedWorkspace")
        create.setProperty("DataValue", mean_val)
        create.execute()
        mean_ws = create.getProperty("OutputWorkspace").value
        
        # Divide each entry by mean
        normalized = self._divide(normalized, mean_ws)
        progress.report()
        
        # Fix-up ranges
        for i in range(normalized.getNumberHistograms()):
            normalized.dataX(i)[0] = bands[0]
            normalized.dataX(i)[1] = bands[-1]

        self.setProperty('OutputWorkspace', normalized)


# Register alg
AlgorithmFactory.subscribe(DetectorFloodWeighting)
