# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#pylint: disable=no-init,invalid-name
import mantid
import os


class SaveGSSCW(mantid.api.PythonAlgorithm):

    def category(self):
        return "DataHandling\\Nexus"

    def seeAlso(self):
        return ["SaveGSSCW"]

    def name(self):
        return "SaveGSSCW"

    def summary(self):
        return "Save constant wavelength powder diffraction data to a GSAS file in FXYE format"

    def PyInit(self):
        self.declareProperty(mantid.api.WorkspaceProperty('InputWorkspace',
                                                          '',
                                                          mantid.kernel.Direction.Input),
                             "Workspace to save")
        self.declareProperty(mantid.api.FileProperty('OutputFilename',
                                                     '',
                                                     action=mantid.api.FileAction.Save,
                                                     extensions=['.gss']),
                             doc='Name of the GSAS file to save to')

    def validateInputs(self):
        """Validate input properties
        (virtual method)

        """
        issues = dict()

        wkspParamName = 'InputWorkspace'
        # FIXME - need to find out the unit of the workspace
        allowedUnits = ['Degrees']

        # check for units
        wksp = self.getProperty(wkspParamName).value
        units = wksp.getAxis(0).getUnit().unitID()
        if units not in allowedUnits:
            allowedUnits = ["'%s'" % unit for unit in allowedUnits]
            allowedUnits = ', '.join(allowedUnits)
            issues[wkspParamName] = "Only support units %s" % allowedUnits
            print(f'[ERROR] Workspace {wksp.name()} with unit {units} is not supported.'
                  f'Allowed units are {allowedUnits}')

        # check output file name: whether user can access and write files
        output_file_name_property = 'OutputFilename'
        gsas_name = self.getProperty(output_file_name_property).value
        if os.path.exists(gsas_name):
            # file exists: check write permission
            if not os.access(gsas_name, os.W_OK):
                issues[output_file_name_property] = f'Output GSAS file {gsas_name} exists and cannot be overwritten'
        else:
            # file does not exist: check write permission to directory
            gsas_dir = os.path.dirname(gsas_name)
            if not os.access(gsas_dir, os.W_OK):
                issues[output_file_name_property] = f'User has not write permission to directory {gsas_dir} ' \
                                                    f'for output file {gsas_name}'

        return issues

    def PyExec(self):
        """
        Main method to execute the algorithm
        """
        # Get input
        wksp = self.getProperty('InputWorkspace').value
        assert wksp, 'Input workspace cannot be None'

        gsas_content = self._write_gsas_fxye(wksp)

        # Get output and write file
        gsas_name = self.getProperty('OutputFilename').value
        with open(gsas_name, 'w') as gsas_file:
            gsas_file.write(gsas_content)

    def _write_gsas_fxye(self, workspace):
        """
        Write GSAS file from a workspace with FXYE format
        """
        gsas_content = ''

        return gsas_content


# Register the algorithm
mantid.api.AlgorithmFactory.subscribe(SaveGSSCW)
