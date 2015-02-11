import os
import unittest

from mantid.simpleapi import *
from mantid.api import MatrixWorkspace, WorkspaceGroup
from IndirectDataAnalysis import abscorFeeder

def setup_corrections_test(using_can=False):
    """ Decorator to setup a test to use a corrections workspace """
    def _decorator(test_case):
        def _inner_decorator(self, *args, **kwargs):
            self._corrections_workspace = self.make_corrections_workspace(using_can)
            self._reference_corrections = CloneWorkspace(self._corrections_workspace,
                                                         OutputWorkspace='ref_corrections')
            self._using_corrections = True
            return test_case(self, *args, **kwargs)
        return _inner_decorator
    return _decorator

def setup_can_test(test_case):
    """ Decorator to setup a test to use a can workspace """
    def _decorator(self, *args, **kwargs):
        self._can_workspace = self.make_can_workspace()
        self._reference_can = CloneWorkspace(self._can_workspace,
                                             OutputWorkspace='ref_can')
        return test_case(self, *args, **kwargs)
    return _decorator


class ApplyCorrectionsTests(unittest.TestCase):

    def setUp(self):
        self._sample_workspace = self.make_sample_workspace()
        self._can_workspace = ''
        self._corrections_workspace = ''
        self._can_geometry = 'cyl'
        self._using_corrections = False

        self._kwargs = {'RebinCan':False, 'ScaleOrNotToScale':False,
                        'factor':1, 'Save':False, 'PlotResult':'None', 'PlotContrib':False}

        self._saved_workspaces = []

        self._reference_sample = CloneWorkspace(self._sample_workspace, OutputWorkspace='ref_sample')
        self._reference_can = None
        self._reference_corrections = None

    def tearDown(self):
        mtd.clear()
        self.clean_up_saved_workspaces()

    @setup_can_test
    def test_with_can_workspace(self):
        output_workspaces = self.run_apply_corrections()

        self.assert_workspaces_exist(output_workspaces)
        self.assert_workspaces_have_correct_types(output_workspaces)
        self.assert_workspaces_have_correct_units(output_workspaces)
        self.assert_input_workspaces_not_modified()

    @setup_can_test
    def test_scale_can_workspace(self):
        self._kwargs['ScaleOrNotToScale'] = True
        self._kwargs['factor'] = 2

        output_workspaces = self.run_apply_corrections()

        self.assert_workspaces_exist(output_workspaces)
        self.assert_workspaces_have_correct_types(output_workspaces)
        self.assert_workspaces_have_correct_units(output_workspaces)
        self.assert_input_workspaces_not_modified()

    @setup_can_test
    def test_rebin_can_workspace(self):
        self._can_workspace = self.make_can_workspace(bin_width=0.001)
        ScaleX(self._can_workspace, Factor=0.1, OutputWorkspace=self._can_workspace)
        self._reference_can = CloneWorkspace(self._can_workspace, OutputWorkspace='ref_can')

        #should fail because the binning of the sample and can don't match
        self.assertRaises(ValueError, self.run_apply_corrections)

        #try again, but rebin the can before the subraction
        self._kwargs['RebinCan'] = True
        try:
            output_workspaces = self.run_apply_corrections()
        except ValueError, ex:
            self.fail("Apply Corrections raised a ValueError when it shouldn't! \
                      \nException was: %s" % ex)

        self.assert_workspaces_exist(output_workspaces)
        self.assert_workspaces_have_correct_types(output_workspaces)
        self.assert_workspaces_have_correct_units(output_workspaces)
        self.assert_input_workspaces_not_modified()

    @setup_can_test
    def test_save_apply_corrections_output(self):
        self._kwargs['Save'] = True

        output_workspaces = self.run_apply_corrections()

        self.assert_workspaces_exist(output_workspaces)
        self.assert_workspaces_have_correct_types(output_workspaces)
        self.assert_workspaces_have_correct_units(output_workspaces)
        self.assert_input_workspaces_not_modified()
        self.assert_workspace_was_saved(output_workspaces['result_workspace'])
        self.assert_workspace_was_saved(output_workspaces['reduced_workspace'])

        #append saved workspaces to list for cleanup
        self._saved_workspaces.append(output_workspaces['result_workspace'])
        self._saved_workspaces.append(output_workspaces['reduced_workspace'])

    @setup_can_test
    @setup_corrections_test(using_can=True)
    def test_with_corrections_workspace(self):
        output_workspaces = self.run_apply_corrections()

        self.assert_workspaces_exist(output_workspaces)
        self.assert_workspaces_have_correct_types(output_workspaces)
        self.assert_workspaces_have_correct_units(output_workspaces)
        self.assert_input_workspaces_not_modified()

    @setup_corrections_test
    def test_with_corrections_no_can(self):
        output_workspaces = self.run_apply_corrections()

        self.assert_workspaces_exist(output_workspaces)
        self.assert_workspaces_have_correct_types(output_workspaces)
        self.assert_workspaces_have_correct_units(output_workspaces)
        self.assert_input_workspaces_not_modified()

    #----------------------------------------------------------------
    # Custom assertion function defintions
    #----------------------------------------------------------------

    def assert_workspaces_exist(self, output_workspaces):
        for ws in output_workspaces.itervalues():
            self.assertTrue(mtd.doesExist(ws), "%s does not exist in the ADS" % ws)

    def assert_workspaces_have_correct_units(self, output_workspaces):
        self.assert_units_match(output_workspaces['reduced_workspace'], 'DeltaE')
        self.assert_units_match(output_workspaces['rqw_workspace'], 'DeltaE')

        self.assert_units_match(output_workspaces['reduced_workspace'], 'Label', axis=1)
        self.assert_units_match(output_workspaces['rqw_workspace'], 'MomentumTransfer', axis=1)

    def assert_workspaces_have_correct_types(self, output_workspaces):
        self.assertTrue(isinstance(mtd[output_workspaces['reduced_workspace']], MatrixWorkspace),
                        "Reduced workspace should be a matrix workspace")

        self.assertTrue(isinstance(mtd[output_workspaces['rqw_workspace']], MatrixWorkspace),
                        "R(Q,w) workspace should be a matrix workspace")

        if 'result_workspace' in output_workspaces:
            self.assertTrue(isinstance(mtd[output_workspaces['result_workspace']], WorkspaceGroup),
                            "Result workspace should be a group workspace")
            result_workspace = mtd[output_workspaces['result_workspace']]
            self.assertEquals(1, result_workspace.size())

            for workspace in result_workspace.getNames():
                self.assertTrue(isinstance(mtd[workspace], MatrixWorkspace),
                                "%s should be a matrix workspace" % workspace)

    def assert_units_match(self, workspace, unit_id, axis=0):
        unit = mtd[workspace].getAxis(axis).getUnit()
        self.assertEquals(unit_id, unit.unitID(),
                          "The units of axis %d in workspace %s do not match. (%s != %s)"
                          % (axis, workspace, unit_id, unit.unitID()))

    def assert_workspace_was_saved(self, workspace):
        working_directory = config['defaultsave.directory']
        file_name = workspace + '.nxs'
        file_path = os.path.join(working_directory, file_name)

        self.assertTrue(os.path.exists(file_path),
                        "%s does not exist in the default save directory." % file_name)
        self.assertTrue(os.path.isfile(file_path),
                        "%s should be a file but it is not" % file_name)

    def assert_input_workspaces_not_modified(self):

        result = CheckWorkspacesMatch(self._reference_sample, self._sample_workspace)
        self.assertEquals("Success!", result)

        if self._can_workspace != '':
            result = CheckWorkspacesMatch(self._reference_can, self._can_workspace)
            self.assertEquals("Success!", result)

        if self._corrections_workspace != '':
            result = CheckWorkspacesMatch(self._reference_corrections, self._corrections_workspace)
            self.assertEquals("Success!", result)

    #----------------------------------------------------------------
    # Functions for creating a dummy workspaces that look like
    # apply corrections input data
    #----------------------------------------------------------------

    def make_dummy_workspace(self, function, output_name, x_unit='DeltaE', num_spectra=1, bin_width=0.001):
        """ Create a dummy workspace that looks like IRIS QENS data"""
        dummy_workspace = CreateSampleWorkspace(Random=True, XMin=0, XMax=1, BinWidth=bin_width, XUnit=x_unit,
                                                Function="User Defined", UserDefinedFunction=function,
                                                OutputWorkspace=output_name)
        ScaleX(dummy_workspace, -0.5, Operation="Add", OutputWorkspace=output_name)
        dummy_workspace = CropWorkspace(dummy_workspace, StartWorkspaceIndex=0, EndWorkspaceIndex=num_spectra-1,
                                        OutputWorkspace=output_name)

        idf = os.path.join(config['instrumentDefinition.directory'], "IRIS_Definition.xml")
        ipf = os.path.join(config['instrumentDefinition.directory'], "IRIS_graphite_002_Parameters.xml")
        LoadInstrument(Workspace=dummy_workspace, Filename=idf)
        LoadParameterFile(Workspace=dummy_workspace, Filename=ipf)

        AddSampleLog(dummy_workspace, LogName='run_number', LogType='Number', LogText='00001')

        return dummy_workspace.name()

    def make_sample_workspace(self, **kwargs):
        """ Create a dummy workspace that looks like a QENS sample run"""
        function = "name=LinearBackground, A0=0.1;name=Lorentzian, PeakCentre=0.5, Amplitude=2, FWHM=0.1"
        sample = self.make_dummy_workspace(function, output_name='sample', **kwargs)
        return sample

    def make_can_workspace(self, **kwargs):
        """ Create a dummy workspace that looks like a QENS can run"""
        function = "name=LinearBackground, A0=0.1;name=Lorentzian, PeakCentre=0.5, Amplitude=0.5, FWHM=0.02"
        can = self.make_dummy_workspace(function, output_name='can', **kwargs)
        return can

    def make_corrections_workspace(self, using_can=False):
        """
        Creates a dummy workspace that looks like a workspace of corrections output from the
        indirect calculate corrections routine. The naming convention must match and uses the formalism
        for absoprtion corrections outlined in Paalman and Pings (1962).
        """
        workspace_names = []
        ass_workspace = self.make_dummy_workspace("name=LinearBackground, A0=0.922948, A1=0;",
                                                  x_unit='Wavelength', output_name='corr_ass')
        AddSampleLog(ass_workspace, LogName='sample_shape', LogType='String', LogText='cylinder')

        if using_can:
            workspace_names.append(ass_workspace)

            assc_workspace = self.make_dummy_workspace("name=LinearBackground, A0=0.921233, A1=-0.007078;",
                                                       x_unit='Wavelength', output_name='corr_assc')
            AddSampleLog(assc_workspace, LogName='sample_shape', LogType='String', LogText='cylinder')
            workspace_names.append(assc_workspace)

            acsc_workspace = self.make_dummy_workspace("name=LinearBackground, A0=0.933229, A1=-0.010020;",
                                                       x_unit='Wavelength', output_name='corr_acsc')
            AddSampleLog(acsc_workspace, LogName='sample_shape', LogType='String', LogText='cylinder')
            workspace_names.append(acsc_workspace)

            acc_workspace = self.make_dummy_workspace("name=LinearBackground, A0=0.995029, A1=-0.010694;",
                                                      x_unit='Wavelength', output_name='corr_acc')
            AddSampleLog(acc_workspace, LogName='sample_shape', LogType='String', LogText='cylinder')
            workspace_names.append(acc_workspace)

            workspace_names = ','.join(workspace_names)
            corrections_workspace = GroupWorkspaces(workspace_names, OutputWorkspace='corrections_workspace')

        return corrections_workspace.name()

    #----------------------------------------------------------------
    # Misc helper functions
    #----------------------------------------------------------------

    def run_apply_corrections(self):
        abscorFeeder(self._sample_workspace, self._can_workspace, self._can_geometry,
                     self._using_corrections, self._corrections_workspace, **self._kwargs)
        return self.get_output_workspace_names()

    def get_output_workspace_names(self):
        """
        abscorFeeder doesn't return anything, these names should exist in the ADS
        apply corrections uses the following naming convention:
        <instrument><sample number>_<analyser><reflection>_<mode>_<can number>
        """
        mode = ''
        if self._corrections_workspace != '' and self._can_workspace != '':
            mode = 'Correct_1'
        elif self._corrections_workspace != '':
            mode = 'Corrected'
        else:
            mode = 'Subtract_1'

        workspace_name_stem = 'irs1_graphite002_%s' % mode

        output_workspaces = {
            'reduced_workspace': workspace_name_stem + '_red',
            'rqw_workspace': workspace_name_stem + '_rqw',
        }

        if self._can_workspace != '':
            output_workspaces['result_workspace'] = workspace_name_stem + '_Result'

        return output_workspaces

    def clean_up_saved_workspaces(self):
        for name in self._saved_workspaces:
            file_path = os.path.join(config['defaultsave.directory'], name + '.nxs')
            if os.path.isfile(file_path):
                os.remove(file_path)

if __name__ == '__main__':
    unittest.main()
