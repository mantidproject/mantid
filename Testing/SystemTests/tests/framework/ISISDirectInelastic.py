# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=no-init
import systemtesting
from mantid.api import FileFinder
from mantid.kernel import config
from mantid.simpleapi import RenameWorkspace
from mantid.api import Workspace
import os
import shutil

from abc import ABCMeta, abstractmethod
from Direct.PropertyManager import PropertyManager

# ----------------------------------------------------------------------


class ISISDirectInelasticReduction(systemtesting.MantidSystemTest, metaclass=ABCMeta):
    """A base class for the ISIS direct inelastic tests

    The workflow is defined in the runTest() method, simply
    define an __init__ method and set the following properties
    on the object
        - instr_name: A string giving the instrument name for the test
        - sample_run: An integer run number of the sample or a a workspace
        - incident_energy: A float value for the Ei guess
        - bins: A list of rebin parameters
        - white_beam: An integer giving a white_beam_file or a workspace
        - mono_van: An integer giving a mono-vanadium run or a workspace or None
        - map_file: An optional string pointing to a map file
        - sample_mass: A float value for the sample mass or None
        - sample_rmm: A float value for the sample rmm or None
        - hard_mask: An hard mask file or None
    """

    tolerance = 0.0
    tolerance_is_rel_err = True

    @abstractmethod
    def get_reference_file(self):
        """Returns the name of the reference file to compare against"""
        raise NotImplementedError("Implement get_reference_file to return the name of the file to compare against.")

    @abstractmethod
    def get_result_workspace(self):
        """Returns the result workspace to be checked"""

    @abstractmethod
    def runTest(self):
        """Defines the workflow for the test"""

    # rename workspace to the name expected by unit test framework

    def validate(self):
        """Returns the name of the workspace & file to compare"""
        self.tolerance = 1e-6
        self.tolerance_is_rel_err = True
        self.disableChecking.append("SpectraMap")
        self.disableChecking.append("Instrument")
        self.disableChecking.append("Sample")
        result = self.get_result_workspace()
        reference = self.get_reference_file()
        return result, reference

    def _is_numeric(self, obj):
        """Returns true if the object is an int or float, false otherwise"""
        if not isinstance(obj, float) or not isinstance(obj, int):
            return True
        else:
            return False

    def _is_workspace(self, obj):
        """Returns True if the object is a workspace"""
        return isinstance(obj, Workspace)

    def __init__(self):
        systemtesting.MantidSystemTest.__init__(self)
        # this is temporary parameter
        self.scale_to_fix_abf = 1


# ------------------------- MARI tests -------------------------------------------------


class MARIReductionFromFile(ISISDirectInelasticReduction):
    def __init__(self):
        ISISDirectInelasticReduction.__init__(self)

        from ISIS_MariReduction import ReduceMARIFromFile

        self.red = ReduceMARIFromFile()
        self.red.def_advanced_properties()
        self.red.def_main_properties()
        # temporary fix to account for different monovan integral
        self.scale_to_fix_abf = 1

    def runTest(self):
        # self.red.run_reduction()
        # pylint: disable=unused-variable
        outWS = self.red.reduce()
        outWS *= self.scale_to_fix_abf

    def get_result_workspace(self):
        """Returns the result workspace to be checked"""
        return "outWS"

    def get_reference_file(self):
        return "MARIReduction.nxs"


class MARIReductionAutoEi(ISISDirectInelasticReduction):
    def __init__(self):
        ISISDirectInelasticReduction.__init__(self)

        from ISIS_MariReduction import ReduceMARIAutoEi

        self.red = ReduceMARIAutoEi()
        self.red.def_advanced_properties()
        self.red.def_main_properties()
        # temporary fix to account for different monovan integral
        self.scale_to_fix_abf = 1
        self.tolerance = 1e-6
        self.ws_name = "outWS"

    def runTest(self):
        # self.red.run_reduction()
        # pylint: disable=unused-variable
        outWS = self.red.reduce()
        outWS = outWS[0]
        outWS *= self.scale_to_fix_abf
        self.ws_name = outWS.name()

    def get_result_workspace(self):
        """Returns the result workspace to be checked"""
        return self.ws_name

    def get_reference_file(self):
        return "MARIReductionAutoEi.nxs"


class MARIReductionFromFileCache(ISISDirectInelasticReduction):
    _counter = 0

    def __init__(self):
        ISISDirectInelasticReduction.__init__(self)
        self.tolerance = 1e-9
        from ISIS_MariReduction import ReduceMARIFromFile

        self._file_to_clear = None
        self._test_log_file = ""
        self.red = ReduceMARIFromFile()
        self.red.def_advanced_properties()
        self.red.def_main_properties()
        self.counter = 0

    def prepare_test_file(self):
        """This method will run instead of pause and
        would copy run file 11001 into 11002 emulating
        appearance of this file from instrument
        """

        self._counter += 1
        if self._counter == 1:
            # first step -- nothing happens. No target file
            # the script reports "waiting for file"
            print("******  Fake pause. step: 1. Preparing test file")
            return
        if self._counter == 2:
            # second step. File is there but log have not been updated
            # nothing happens, the script reports "waiting for file"
            print("******  Fake pause. step: 2. Fake nxs file created to ignore")
            source = FileFinder.findRuns("MAR11001")[0]
            targ_path = config["defaultsave.directory"]
            targ_file = os.path.join(targ_path, "MAR11002.nxs")
            shutil.copy2(source, targ_file)
            self._file_to_clear += [targ_file]
            return
        if self._counter == 3:
            # this is fake step as logs should be updated
            # when both files are added to archive,
            # but let's run it to test additional branch
            # of the code: invalid extension found
            print("******  Fake pause. step: 3. Run log updated but file is not there")
            test_log = self._test_log_file
            with open(test_log, "w") as fh:
                fh.write("MAR 11002 0 \n")
            m_time = os.path.getmtime(test_log)
            # Update modification time manually as Unix may
            # run the test too fast and mtime will be the same
            # as the log has been generated initially
            m_time = m_time + 1
            os.utime(test_log, (m_time, m_time))
            return

        if self._counter >= 4:
            print("******  Fake pause. step: {0}. Created fake source. Reduction should run".format(self._counter))

            source = FileFinder.findRuns("MAR11001")[0]
            targ_path = config["defaultsave.directory"]
            targ_file = os.path.join(targ_path, "MAR11002.raw")
            shutil.copy2(source, targ_file)
            # Reduction should run now

            self._file_to_clear += [targ_file]
            # self._counter = 0

    def runTest(self):
        self.red.wait_for_file = 10
        self.red._debug_wait_for_files_operation = self.prepare_test_file
        self._counter = 0

        # Add test log files, which mimicks information,
        # recorded when file have been added to archive
        test_dir = config.getString("defaultsave.directory")
        test_log = os.path.join(test_dir, "lastrun.txt")
        self._test_log_file = test_log
        with open(test_log, "w") as fh:
            fh.write("MAR 11001 0 \n")
        self._file_to_clear = [test_log]

        self.red.reducer.prop_man.archive_upload_log_file = test_log
        self.red.reducer.prop_man.sample_run = [11001, 11002]
        # self.red.reducer.background_range = (10000,12000)
        MARreducedRuns = self.red.run_reduction()

        RenameWorkspace(InputWorkspace=MARreducedRuns[0], OutputWorkspace="MARreducedFromFile")
        RenameWorkspace(InputWorkspace=MARreducedRuns[1], OutputWorkspace="MARreducedWithCach")

        self.red.wait_for_file = 0
        self.red._debug_wait_for_files_operation = None
        for file in self._file_to_clear:
            os.remove(file)

    def validate(self):
        """Returns the name of the workspace & file to compare"""
        super(MARIReductionFromFileCache, self).validate()
        self.tolerance = 1e-9
        return "MARreducedFromFile", "MARreducedWithCach"

    def validateMethod(self):
        return "validateWorkspaceToWorkspace"

    def get_result_workspace(self):
        """Returns the result workspace to be checked"""
        return "outWS"

    def get_reference_file(self):
        return "MARIReduction.nxs"


class MARIReductionFromWorkspace(ISISDirectInelasticReduction):
    def __init__(self):
        ISISDirectInelasticReduction.__init__(self)

        from ISIS_MariReduction import ReduceMARIFromWorkspace

        self.red = ReduceMARIFromWorkspace()
        self.red.def_advanced_properties()
        self.red.def_main_properties()

        self.scale_to_fix_abf = 1.0

    def runTest(self):
        """Defines the workflow for the test"""
        # pylint: disable=unused-variable
        outWS = self.red.reduce()
        # temporary fix to account for different monovan integral
        outWS *= self.scale_to_fix_abf

    def get_result_workspace(self):
        """Returns the result workspace to be checked"""
        return "outWS"

    def get_reference_file(self):
        return "MARIReduction.nxs"


class MARIReductionMon2Norm(ISISDirectInelasticReduction):
    def __init__(self):
        ISISDirectInelasticReduction.__init__(self)

        from ISIS_MariReduction import ReduceMARIMon2Norm

        self.red = ReduceMARIMon2Norm()
        self.red.def_advanced_properties()
        self.red.def_main_properties()

    def runTest(self):
        """Defines the workflow for the test"""
        # pylint: disable=unused-variable
        outWS = self.red.reduce()
        # As we compare with workspace, normalized by current, this is the difference
        # between current and monitor-2 normalization in this particular case
        # (well within round-off)
        outWS *= 0.991886

    def get_result_workspace(self):
        """Returns the result workspace to be checked"""
        return "outWS"

    def get_reference_file(self):
        return "MARIReduction.nxs"

    def validate(self):
        result, reference = super(MARIReductionMon2Norm, self).validate()
        self.tolerance = 1e-3
        return result, reference


class MARIReductionMonSeparate(ISISDirectInelasticReduction):
    def __init__(self):
        ISISDirectInelasticReduction.__init__(self)
        # This test has not been run properly so reference file is kind-of
        # arbitrary. It just checks that this reduction works.
        # Mari reduction masks are not correct for monitors loaded separately,
        # This explains all the difference encountered.
        from ISIS_MariReduction import ReduceMARIMonitorsSeparate

        self.red = ReduceMARIMonitorsSeparate()
        self.red.def_advanced_properties()
        self.red.def_main_properties()

    def runTest(self):
        """Defines the workflow for the test"""

        # temporary fix cross-influence of tests for MARI. changes to nex ticket make this unnecessary
        PropertyManager.mono_correction_factor.set_cash_mono_run_number(None)
        # pylint: disable=unused-variable
        outWS = self.red.reduce()
        # temporary fix to account for different monovan integral
        # outWS*=0.997966051169129
        self.ws_name = outWS.name()

    def get_result_workspace(self):
        """Returns the result workspace to be checked"""
        return self.ws_name

    def get_reference_file(self):
        # monitor separate for MARI needs new maps and masks so, it is easier to redefine
        # reference file for the time being
        return "MARIReductionMonSeparate.nxs"


class MARIReductionSum(ISISDirectInelasticReduction):
    def __init__(self):
        ISISDirectInelasticReduction.__init__(self)
        from ISIS_MariReduction import MARIReductionSum

        self.red = MARIReductionSum()
        self.red.def_advanced_properties()
        self.red.def_main_properties()

    def runTest(self):
        """Defines the workflow for the test
        It verifies operation on summing two files on demand. No absolute units
        """
        # pylint: disable=unused-variable
        outWS = self.red.reduce()
        # outWS*=1.00001556766686
        self.ws_name = outWS.name()

    def get_result_workspace(self):
        """Returns the result workspace to be checked"""
        return self.ws_name

    def get_reference_file(self):
        return "MARIReductionSum.nxs"


class MARIReductionWaitAndSum(ISISDirectInelasticReduction):
    def __init__(self):
        ISISDirectInelasticReduction.__init__(self)
        from ISIS_MariReduction import MARIReductionSum

        self.red = MARIReductionSum()
        self.red.def_advanced_properties()
        self.red.def_main_properties()
        self._counter = 0
        self._file_to_clear = ""

    def prepare_test_file(self):
        """This method will run instead of pause and
        would copy run file 11015 into 11002 emulating
        appearance of this file from instrument
        """
        self._counter += 1
        if self._counter >= 3:
            source = FileFinder.findRuns("MAR11015")[0]
            targ_path = config["defaultsave.directory"]
            targ_file = os.path.join(targ_path, "MAR11002.raw")
            shutil.copy2(source, targ_file)

            self._file_to_clear = targ_file
            self._counter = 0

    def runTest(self):
        """Defines the workflow for the test
        It verifies operation on summing two files on demand. with wait for
        files appearing on data search path
        """
        targ_path = config["defaultsave.directory"]
        self._file_to_clear = os.path.join(targ_path, "MAR11002.raw")
        if os.path.exists(self._file_to_clear):
            os.remove(self._file_to_clear)
            self._file_to_clear = ""

        self.red.wait_for_file = 100
        self.red._debug_wait_for_files_operation = self.prepare_test_file
        self._counter = 0

        self.red.reducer.prop_man.sample_run = [11001, 11002]
        # pylint: disable=unused-variable
        outWS = self.red.run_reduction()
        self.ws_name = outWS.name()

        self.red.wait_for_file = 0
        self.red._debug_wait_for_files_operation = None
        os.remove(self._file_to_clear)

    def get_result_workspace(self):
        """Returns the result workspace to be checked"""
        return self.ws_name

    def get_reference_file(self):
        return "MARIReductionSum.nxs"


# ------------------------- MAPS tests -------------------------------------------------


class MAPSDgreduceReduction(ISISDirectInelasticReduction):
    def requiredMemoryMB(self):
        """Far too slow for managed workspaces. They're tested in other places. Requires 10Gb"""
        return 10000

    def __init__(self):
        ISISDirectInelasticReduction.__init__(self)

        from ISIS_MAPS_DGSReduction import ReduceMAPS

        self.ws_name = ""
        self.red = ReduceMAPS()
        self.red.def_advanced_properties()
        self.red.def_main_properties()

    def runTest(self):
        # pylint: disable=unused-variable
        outWS = self.red.reduce()
        # New WBI value 0.02720959162181584
        # Old WBI Value 0.027209867107187088
        # fix old system test.
        # outWS*=0.02720959162181584/0.027209867107187088

        # rename workspace to the name expected by unit test framework
        # RenameWorkspace(InputWorkspace=outWS,OutputWorkspace=wsName)
        self.ws_name = outWS.name()

    def get_reference_file(self):
        return "MAPSDgreduceReduction.nxs"

    def get_result_workspace(self):
        """Returns the result workspace to be checked"""
        return self.ws_name


# ------------------------- MERLIN tests -------------------------------------------------


class MERLINReduction(ISISDirectInelasticReduction):
    def requiredMemoryMB(self):
        """Far too slow for managed workspaces. They're tested in other places. Requires 16Gb"""
        return 16000

    def __init__(self):
        """Test relies on MERLIN_Parameters.xml file introduced in July 2014"""
        ISISDirectInelasticReduction.__init__(self)

        from ISIS_MERLINReduction import ReduceMERLIN

        self.red = ReduceMERLIN()
        self.red.def_advanced_properties()
        self.red.def_main_properties()

    def runTest(self):
        # pylint: disable=unused-variable
        outWS = self.red.reduce()
        self.ws_name = outWS.name()

    def get_reference_file(self):
        return "MERLINReduction.nxs"

    def get_result_workspace(self):
        """Returns the result workspace to be checked"""
        return self.ws_name

    def validate(self):
        self.tolerance = 1e-6
        self.tolerance_is_rel_err = True
        self.disableChecking.append("SpectraMap")
        self.disableChecking.append("Instrument")
        result = self.get_result_workspace()
        reference = self.get_reference_file()
        return result, reference


# ------------------------- LET tests -------------------------------------------------
#


class LETReduction(systemtesting.MantidSystemTest):
    tolerance = 1e-6
    tolerance_is_rel_err = True

    def requiredMemoryMB(self):
        """Far too slow for managed workspaces. They're tested in other places. Requires 2Gb"""
        return 2000

    def runTest(self):
        """
        Run the LET reduction with event NeXus files

        Relies on LET_Parameters.xml file from June 2013
        """
        from ISIS_LETReduction import ReduceLET_OneRep

        red = ReduceLET_OneRep()
        red.def_main_properties()
        red.def_advanced_properties()
        # pylint: disable=unused-variable
        outWS = red.reduce()
        self.ws_name = outWS.name()

    def validate(self):
        self.tolerance = 1e-6
        self.tolerance_is_rel_err = True
        self.disableChecking.append("SpectraMap")
        self.disableChecking.append("Instrument")

        return self.ws_name, "LETReduction.nxs"


class LETReductionEvent2015Multirep(systemtesting.MantidSystemTest):
    """
    written in a hope that most of the stuff find here will eventually find its way into main reduction routines
    """

    tolerance = 1e-6
    tolerance_is_rel_err = True

    def requiredMemoryMB(self):
        """Far too slow for managed workspaces. They're tested in other places. Requires 20Gb"""
        return 20000

    def runTest(self):
        """
        Run the LET reduction with event NeXus files

        Relies on LET_Parameters.xml file from June 2013
        """
        from ISIS_LETReduction import ReduceLET_MultiRep2015

        red = ReduceLET_MultiRep2015()

        red.def_advanced_properties()
        red.def_main_properties()

        # pylint: disable=unused-variable
        out_ws_list = red.run_reduction()
        self.ws_names = [ws.name() for ws in out_ws_list]

        # for ind,ws in enumerate(out_ws_list):
        #  ws *=mults[ind]

    def validate(self):
        self.tolerance = 1e-6
        self.tolerance_is_rel_err = False
        self.disableChecking.append("SpectraMap")
        self.disableChecking.append("Instrument")

        return self.ws_names[0], "LET14305_3_4meV2015.nxs", self.ws_names[1], "LET14305_8_0meV2015.nxs"
