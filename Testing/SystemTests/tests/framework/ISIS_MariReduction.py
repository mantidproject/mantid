# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=invalid-name
"""Sample MARI reduction scrip used in testing ReductionWrapper"""

import os

#
from Direct.ReductionWrapper import *

try:
    import reduce_vars as web_var
except ModuleNotFoundError:
    web_var = None


class ReduceMARIFromFile(ReductionWrapper):
    @MainProperties
    def def_main_properties(self):
        """Define main properties used in reduction"""
        prop = {}
        prop["sample_run"] = 11001
        prop["wb_run"] = 11060
        prop["incident_energy"] = 12
        prop["energy_bins"] = [-11, 0.05, 11]

        # prop['sum_runs'] = False

        # Absolute units reduction properties.
        prop["monovan_run"] = 11015
        prop["sample_mass"] = 10
        prop["sample_rmm"] = 435.96
        return prop

    @AdvancedProperties
    def def_advanced_properties(self):
        """separation between simple and advanced properties depends
        on scientist, experiment and user.
        main properties override advanced properties.
        """
        prop = {}
        prop["map_file"] = "mari_res.map"
        prop["monovan_mapfile"] = "mari_res.map"
        prop["hard_mask_file"] = "mar11015.msk"
        prop["det_cal_file"] = 11060
        prop["save_format"] = ""
        prop["nullify_negative_signal"] = True
        return prop

    #

    @iliad
    def reduce(self, input_file=None, output_directory=None):
        """Method executes reduction over single file
        Overload only if custom reduction is needed
        """
        converted_to_energy_transfer_ws = ReductionWrapper.reduce(self, input_file, output_directory)
        # SaveNexus(outWS,Filename = 'MARNewReduction.nxs')
        return converted_to_energy_transfer_ws

    def set_custom_output_filename(self):
        """define custom name of output files if standard one is not satisfactory
        In addition to that, example of accessing reduction properties
        Changing them if necessary
        """

        def custom_name(prop_man):
            """Sample function which builds filename from
            incident energy and run number and adds some auxiliary information
            to it.
            """
            # Note -- properties have the same names as the list of advanced and
            # main properties
            ei = prop_man.incident_energy
            # sample run is more then just list of runs, so we use
            # the formalization below to access its methods
            run_num = PropertyManager.sample_run.run_number()
            name = "RUN{0}atEi{1:<3.2f}meV_One2One".format(run_num, ei)
            return name

        # Uncomment this to use custom filename function
        # Note: the properties are stored in prop_man class accessed as
        # below.
        # return lambda : custom_name(self.reducer.prop_man)
        # use this method to use standard file name generating function
        return None

    #

    def do_preprocessing(self, reducer, ws):
        """Custom function, applied to each run or every workspace, the run is divided to
        in multirep mode
        Applied after diagnostics but before any further reduction is invoked.
        Inputs:
        self    -- initialized instance of the instrument reduction class
        reducer -- initialized instance of the reducer
                   (DirectEnergyConversion class initialized for specific reduction)
        ws         the workspace, describing the run or partial run in multirep mode
                   to preprocess

        By default, does nothing.
        Add code to do custom preprocessing.
        Must return pointer to the preprocessed workspace
        """
        return ws

    #

    def do_postprocessing(self, reducer, ws):
        """Custom function, applied to each reduced run or every reduced workspace,
        the run is divided into, in multirep mode.
        Applied after reduction is completed but before saving the result.

        Inputs:
        self    -- initialized instance of the instrument reduction class
        reducer -- initialized instance of the reducer
                   (DirectEnergyConversion class initialized for specific reduction)
        ws         the workspace, describing the run or partial run in multirep mode
                   after reduction to postprocess


        By default, does nothing.
        Add code to do custom postprocessing.
        Must return pointer to the postprocessed workspace.

        The postprocessed workspace should be consistent with selected save method.
        (E.g. if you decide to convert workspace units to wavelength, you can not save result as nxspe)
        """
        return ws

    def __init__(self, web_var_val=None):
        """sets properties defaults for the instrument with Name"""
        ReductionWrapper.__init__(self, "MAR", web_var_val)
        Mt = MethodType(self.do_preprocessing, self.reducer)
        DirectEnergyConversion.__setattr__(self.reducer, "do_preprocessing", Mt)
        Mt = MethodType(self.do_postprocessing, self.reducer)
        DirectEnergyConversion.__setattr__(self.reducer, "do_postprocessing", Mt)


# -------------------------------------------------------------------------------------------------#
# -------------------------------------------------------------------------------------------------#
# -------------------------------------------------------------------------------------------------#


def main(input_file=None, output_directory=None):
    """This method is used to run code from web service
    and should not be touched except changing the name of the
    particular ReductionWrapper class (e.g. ReduceMARI here)

    You can also change the output folder to save data to
    where web services will copy data

    This method will go when web service implements proper factory
    """
    # note web variables initialization
    rd = ReduceMARIFromFile(web_var)
    rd.reduce(input_file, output_directory)
    # change to the name of the folder to save data to
    return ""


# ----------------------------------------------------------------------------------------------------------------------


class ReduceMARIFromWorkspace(ReductionWrapper):
    @MainProperties
    def def_main_properties(self):
        """Define main properties used in reduction"""
        prop = {}
        prop["sample_run"] = Load(Filename="MAR11001.RAW", OutputWorkspace="MAR11001.RAW")
        # WB workspace
        prop["wb_run"] = Load(Filename="MAR11060.RAW", OutputWorkspace="MAR11060.RAW")
        prop["incident_energy"] = 12
        prop["energy_bins"] = [-11, 0.05, 11]

        # Absolute units reduction properties.
        prop["monovan_run"] = Load(Filename="MAR11015.RAW", OutputWorkspace="MAR11015.RAW")
        prop["sample_mass"] = 10
        prop["sample_rmm"] = 435.96

        return prop

    @AdvancedProperties
    def def_advanced_properties(self):
        """separation between simple and advanced properties depends
        on scientist, experiment and user.
        main properties override advanced properties.
        """
        prop = {}
        prop["map_file"] = "mari_res.map"
        prop["monovan_mapfile"] = "mari_res.map"
        prop["hard_mask_file"] = "mar11015.msk"
        # MARI calibration uses one of data files defined on instrument.  Here
        # vanadium run is used for calibration
        # TODO: Why not workspace?
        prop["det_cal_file"] = "11060"
        prop["save_format"] = ""
        prop["nullify_negative_signal"] = True
        return prop

    #

    @iliad
    def reduce(self, input_file=None, output_directory=None):
        """Method executes reduction over single file
        Overload only if custom reduction is needed
        """
        ws = ReductionWrapper.reduce(self, input_file, output_directory)
        # SaveNexus(ws,Filename = 'MARNewReduction.nxs')
        return ws

    #

    def do_preprocessing(self, reducer, ws):
        """Custom function, applied to each run or every workspace, the run is divided to
        in multirep mode
        Applied after diagnostics but before any further reduction is invoked.
        Inputs:
        self    -- initialized instance of the instrument reduction class
        reducer -- initialized instance of the reducer
                   (DirectEnergyConversion class initialized for specific reduction)
        ws         the workspace, describing the run or partial run in multirep mode
                   to preprocess

        By default, does nothing.
        Add code to do custom preprocessing.
        Must return pointer to the preprocessed workspace
        """
        return ws

    #

    def do_postprocessing(self, reducer, ws):
        """Custom function, applied to each reduced run or every reduced workspace,
        the run is divided into, in multirep mode.
        Applied after reduction is completed but before saving the result.

        Inputs:
        self    -- initialized instance of the instrument reduction class
        reducer -- initialized instance of the reducer
                   (DirectEnergyConversion class initialized for specific reduction)
        ws         the workspace, describing the run or partial run in multirep mode
                   after reduction to postprocess


        By default, does nothing.
        Add code to do custom postprocessing.
        Must return pointer to the postprocessed workspace.

        The postprocessed workspace should be consistent with selected save method.
        (E.g. if you decide to convert workspace units to wavelength, you can not save result as nxspe)
        """
        return ws

    def __init__(self, web_var_val=None):
        """sets properties defaults for the instrument with Name"""
        ReductionWrapper.__init__(self, "MAR", web_var_val)
        Mt = MethodType(self.do_preprocessing, self.reducer)
        DirectEnergyConversion.__setattr__(self.reducer, "do_preprocessing", Mt)
        Mt = MethodType(self.do_postprocessing, self.reducer)
        DirectEnergyConversion.__setattr__(self.reducer, "do_postprocessing", Mt)


# ----------------------------------------------------------------------------------------------------------------------


class ReduceMARIMon2Norm(ReductionWrapper):
    @MainProperties
    def def_main_properties(self):
        """Define main properties used in reduction"""
        prop = {}
        prop["sample_run"] = Load(Filename="MAR11001.RAW", OutputWorkspace="MAR11001.RAW")
        # WB workspace
        prop["wb_run"] = Load(Filename="MAR11060.RAW", OutputWorkspace="MAR11060.RAW")
        prop["incident_energy"] = 12
        prop["energy_bins"] = [-11, 0.05, 11]

        # Absolute units reduction properties.
        prop["monovan_run"] = 11015  # Load(Filename='MAR11015.RAW',OutputWorkspace='MAR11015.RAW')
        prop["sample_mass"] = 10
        prop["sample_rmm"] = 435.96
        return prop

    @AdvancedProperties
    def def_advanced_properties(self):
        """separation between simple and advanced properties depends
        on scientist, experiment and user.
        main properties override advanced properties.
        """
        prop = {}
        prop["map_file"] = "mari_res.map"
        prop["monovan_mapfile"] = "mari_res.map"
        prop["hard_mask_file"] = "mar11015.msk"
        # prop['hardmaskOnly'] ="mar11015.msk"
        prop["normalise_method"] = "monitor-2"
        # reduction from workspace currently needs detector_calibration file
        # MARI calibration uses one of data files defined on instrument.  Here
        # vanadium run is used for calibration
        # TODO: Why not workspace?, check it
        prop["det_cal_file"] = "MAR11060.raw"
        prop["save_format"] = []
        prop["nullify_negative_signal"] = True
        return prop

    #

    @iliad
    def reduce(self, input_file=None, output_directory=None):
        """Method executes reduction over single file
        Overload only if custom reduction is needed
        """
        outWS = ReductionWrapper.reduce(self, input_file, output_directory)
        # SaveNexus(ws,Filename = 'MARNewReduction.nxs')
        return outWS

    #

    def do_preprocessing(self, reducer, ws):
        """Custom function, applied to each run or every workspace, the run is divided to
        in multirep mode
        Applied after diagnostics but before any further reduction is invoked.
        Inputs:
        self    -- initialized instance of the instrument reduction class
        reducer -- initialized instance of the reducer
                   (DirectEnergyConversion class initialized for specific reduction)
        ws         the workspace, describing the run or partial run in multirep mode
                   to preprocess

        By default, does nothing.
        Add code to do custom preprocessing.
        Must return pointer to the preprocessed workspace
        """
        return ws

    #

    def do_postprocessing(self, reducer, ws):
        """Custom function, applied to each reduced run or every reduced workspace,
        the run is divided into, in multirep mode.
        Applied after reduction is completed but before saving the result.

        Inputs:
        self    -- initialized instance of the instrument reduction class
        reducer -- initialized instance of the reducer
                   (DirectEnergyConversion class initialized for specific reduction)
        ws         the workspace, describing the run or partial run in multirep mode
                   after reduction to postprocess


        By default, does nothing.
        Add code to do custom postprocessing.
        Must return pointer to the postprocessed workspace.

        The postprocessed workspace should be consistent with selected save method.
        (E.g. if you decide to convert workspace units to wavelength, you can not save result as nxspe)
        """
        return ws

    def __init__(self, web_var_val=None):
        """sets properties defaults for the instrument with Name"""
        ReductionWrapper.__init__(self, "MAR", web_var_val)
        Mt = MethodType(self.do_preprocessing, self.reducer)
        DirectEnergyConversion.__setattr__(self.reducer, "do_preprocessing", Mt)
        Mt = MethodType(self.do_postprocessing, self.reducer)
        DirectEnergyConversion.__setattr__(self.reducer, "do_postprocessing", Mt)


# ----------------------------------------------------------------------------------------------------------------------


class MARIReductionSum(ReductionWrapper):
    @MainProperties
    def def_main_properties(self):
        """Define main properties used in reduction"""
        prop = {}
        prop["sample_run"] = [11001, 11015]
        prop["wb_run"] = 11060
        prop["incident_energy"] = 11
        prop["energy_bins"] = [-11, 0.05, 11]
        prop["sum_runs"] = True

        # Absolute units reduction properties.
        # prop['monovan_run'] = 11015
        # prop['sample_mass'] = 32.58
        # prop['sample_rmm'] = 50.9415# 435.96
        return prop

    @AdvancedProperties
    def def_advanced_properties(self):
        """separation between simple and advanced properties depends
        on scientist, experiment and user.
        main properties override advanced properties.
        """
        prop = {}
        prop["map_file"] = "mari_res.map"
        prop["monovan_mapfile"] = "mari_res.map"
        prop["hard_mask_file"] = "mar11015.msk"
        # prop['det_cal_file'] =11060
        prop["save_format"] = ""
        prop["nullify_negative_signal"] = True
        return prop

    #

    @iliad
    def reduce(self, input_file=None, output_directory=None):
        """Method executes reduction over single file
        Overload only if custom reduction is needed
        """
        ws = ReductionWrapper.reduce(self, input_file, output_directory)
        # SaveNexus(ws,Filename = 'MARNewReduction.nxs')
        return ws

    #

    def do_preprocessing(self, reducer, ws):
        """Custom function, applied to each run or every workspace, the run is divided to
        in multirep mode
        Applied after diagnostics but before any further reduction is invoked.
        Inputs:
        self    -- initialized instance of the instrument reduction class
        reducer -- initialized instance of the reducer
                   (DirectEnergyConversion class initialized for specific reduction)
        ws         the workspace, describing the run or partial run in multirep mode
                   to preprocess

        By default, does nothing.
        Add code to do custom preprocessing.
        Must return pointer to the preprocessed workspace
        """
        return ws

    #

    def do_postprocessing(self, reducer, ws):
        """Custom function, applied to each reduced run or every reduced workspace,
        the run is divided into, in multirep mode.
        Applied after reduction is completed but before saving the result.

        Inputs:
        self    -- initialized instance of the instrument reduction class
        reducer -- initialized instance of the reducer
                   (DirectEnergyConversion class initialized for specific reduction)
        ws         the workspace, describing the run or partial run in multirep mode
                   after reduction to postprocess


        By default, does nothing.
        Add code to do custom postprocessing.
        Must return pointer to the postprocessed workspace.

        The postprocessed workspace should be consistent with selected save method.
        (E.g. if you decide to convert workspace units to wavelength, you can not save result as nxspe)
        """
        return ws

    def __init__(self, web_var_val=None):
        """sets properties defaults for the instrument with Name"""
        ReductionWrapper.__init__(self, "MAR", web_var_val)
        Mt = MethodType(self.do_preprocessing, self.reducer)
        DirectEnergyConversion.__setattr__(self.reducer, "do_preprocessing", Mt)
        Mt = MethodType(self.do_postprocessing, self.reducer)
        DirectEnergyConversion.__setattr__(self.reducer, "do_postprocessing", Mt)


# ----------------------------------------------------------------------------------------------------------------------


class ReduceMARIMonitorsSeparate(ReductionWrapper):
    @MainProperties
    def def_main_properties(self):
        """Define main properties used in reduction"""
        prop = {}
        prop["sample_run"] = 11001  #
        # WB workspace Simulate workspace without monitors
        prop["wb_run"] = Load(Filename="MAR11060.RAW", OutputWorkspace="MAR11060.RAW", LoadMonitors="Exclude")
        prop["incident_energy"] = 12
        prop["energy_bins"] = [-11, 0.05, 11]

        # Absolute units reduction properties.
        prop["monovan_run"] = 11015  #
        prop["sample_mass"] = 10
        prop["sample_rmm"] = 435.96

        return prop

    @AdvancedProperties
    def def_advanced_properties(self):
        """separation between simple and advanced properties depends
        on scientist, experiment and user.
        main properties override advanced properties.
        """
        prop = {}
        prop["map_file"] = "mari_res.map"
        prop["monovan_mapfile"] = "mari_res.map"
        prop["hard_mask_file"] = "mar11015.msk"
        # MARI calibration uses one of data files defined on instrument.  Here
        # vanadium run is used for calibration
        # TODO: Why not workspace?
        prop["det_cal_file"] = "11060"
        prop["save_format"] = ""
        prop["load_monitors_with_workspace"] = False
        prop["nullify_negative_signal"] = False
        prop["mapmask_ref_ws"] = Load(Filename="MAR11001.RAW", OutputWorkspace="MAR11001.RAW", LoadMonitors="Include")

        return prop

    #

    @iliad
    def reduce(self, input_file=None, output_directory=None):
        """Method executes reduction over single file
        Overload only if custom reduction is needed
        """
        outWS = ReductionWrapper.reduce(self, input_file, output_directory)
        # SaveNexus(outWS,Filename = 'MARNewReduction.nxs')
        return outWS

    #

    def do_preprocessing(self, reducer, ws):
        """Custom function, applied to each run or every workspace, the run is divided to
        in multirep mode
        Applied after diagnostics but before any further reduction is invoked.
        Inputs:
        self    -- initialized instance of the instrument reduction class
        reducer -- initialized instance of the reducer
                   (DirectEnergyConversion class initialized for specific reduction)
        ws         the workspace, describing the run or partial run in multirep mode
                   to preprocess

        By default, does nothing.
        Add code to do custom preprocessing.
        Must return pointer to the preprocessed workspace
        """
        return ws

    #

    def do_postprocessing(self, reducer, ws):
        """Custom function, applied to each reduced run or every reduced workspace,
        the run is divided into, in multirep mode.
        Applied after reduction is completed but before saving the result.

        Inputs:
        self    -- initialized instance of the instrument reduction class
        reducer -- initialized instance of the reducer
                   (DirectEnergyConversion class initialized for specific reduction)
        ws         the workspace, describing the run or partial run in multirep mode
                   after reduction to postprocess


        By default, does nothing.
        Add code to do custom postprocessing.
        Must return pointer to the postprocessed workspace.

        The postprocessed workspace should be consistent with selected save method.
        (E.g. if you decide to convert workspace units to wavelength, you can not save result as nxspe)
        """
        return ws

    def __init__(self, web_var_val=None):
        """sets properties defaults for the instrument with Name"""
        ReductionWrapper.__init__(self, "MAR", web_var_val)
        Mt = MethodType(self.do_preprocessing, self.reducer)
        DirectEnergyConversion.__setattr__(self.reducer, "do_preprocessing", Mt)
        Mt = MethodType(self.do_postprocessing, self.reducer)
        DirectEnergyConversion.__setattr__(self.reducer, "do_postprocessing", Mt)


class ReduceMARIAutoEi(ReductionWrapper):
    @MainProperties
    def def_main_properties(self):
        """Define main properties used in reduction"""
        prop = {}
        ws = Load(Filename="MAR11001.RAW", OutputWorkspace="MAR11001")
        # Add these log values to simulated workspace to represent real sample logs
        AddTimeSeriesLog(ws, Name="fermi_delay", Time="2005-01-13T22:55:56", Value=6530, DeleteExisting=True)
        AddTimeSeriesLog(ws, Name="fermi_delay", Time="2005-01-14T09:23:34", Value=6530)
        AddTimeSeriesLog(ws, Name="fermi_speed", Time="2005-01-13T22:55:56", Value=400, DeleteExisting=True)
        AddTimeSeriesLog(ws, Name="fermi_speed", Time="2005-01-14T09:23:34", Value=400)
        # AddTimeSeriesLog(mon_ws, Name="is_running", Time="2010-01-01T00:00:00", Value=1 ,DeleteExisting=True)
        # AddTimeSeriesLog(mon_ws, Name="is_running", Time="2010-01-01T00:30:00", Value=1 )

        prop["sample_run"] = ws
        prop["wb_run"] = 11060
        prop["incident_energy"] = "auto"
        # Relative energy ranges
        prop["energy_bins"] = [-11.0 / 13.0, 0.05 / 13.0, 11.0 / 13.0]

        # prop['sum_runs'] = False

        # Absolute units reduction properties.
        prop["monovan_run"] = 11015
        prop["sample_mass"] = 10
        prop["sample_rmm"] = 435.96
        return prop

    @AdvancedProperties
    def def_advanced_properties(self):
        """separation between simple and advanced properties depends
        on scientist, experiment and user.
        main properties override advanced properties.
        """
        prop = {}
        prop["map_file"] = "mari_res.map"
        prop["monovan_mapfile"] = "mari_res.map"
        prop["hard_mask_file"] = "mar11015.msk"
        prop["det_cal_file"] = 11060
        prop["save_format"] = ""
        # prop['check_background'] = False
        prop["nullify_negative_signal"] = True
        return prop

    #

    @iliad
    def reduce(self, input_file=None, output_directory=None):
        """Method executes reduction over single file
        Overload only if custom reduction is needed
        """
        converted_to_energy_transfer_ws = ReductionWrapper.reduce(self, input_file, output_directory)
        # SaveNexus(outWS,Filename = 'MARNewReduction.nxs')
        return converted_to_energy_transfer_ws

    def set_custom_output_filename(self):
        """define custom name of output files if standard one is not satisfactory
        In addition to that, example of accessing reduction properties
        Changing them if necessary
        """

        def custom_name(prop_man):
            """Sample function which builds filename from
            incident energy and run number and adds some auxiliary information
            to it.
            """
            # Note -- properties have the same names as the list of advanced and
            # main properties
            ei = prop_man.incident_energy
            # sample run is more then just list of runs, so we use
            # the formalization below to access its methods
            run_num = PropertyManager.sample_run.run_number()
            name = "RUN{0}atEi{1:<3.2f}meV_One2One".format(run_num, ei)
            return name

        # Uncomment this to use custom filename function
        # Note: the properties are stored in prop_man class accessed as
        # below.
        # return lambda : custom_name(self.reducer.prop_man)
        # use this method to use standard file name generating function
        return None

    #

    def do_preprocessing(self, reducer, ws):
        """Custom function, applied to each run or every workspace, the run is divided to
        in multirep mode
        Applied after diagnostics but before any further reduction is invoked.
        Inputs:
        self    -- initialized instance of the instrument reduction class
        reducer -- initialized instance of the reducer
                   (DirectEnergyConversion class initialized for specific reduction)
        ws         the workspace, describing the run or partial run in multirep mode
                   to preprocess

        By default, does nothing.
        Add code to do custom preprocessing.
        Must return pointer to the preprocessed workspace
        """
        return ws

    #

    def do_postprocessing(self, reducer, ws):
        """Custom function, applied to each reduced run or every reduced workspace,
        the run is divided into, in multirep mode.
        Applied after reduction is completed but before saving the result.

        Inputs:
        self    -- initialized instance of the instrument reduction class
        reducer -- initialized instance of the reducer
                   (DirectEnergyConversion class initialized for specific reduction)
        ws         the workspace, describing the run or partial run in multirep mode
                   after reduction to postprocess


        By default, does nothing.
        Add code to do custom postprocessing.
        Must return pointer to the postprocessed workspace.

        The postprocessed workspace should be consistent with selected save method.
        (E.g. if you decide to convert workspace units to wavelength, you can not save result as nxspe)
        """
        return ws

    def eval_absorption_corrections(self, test_ws=None):
        """The method to evaluate the speed and efficiency of the absorption corrections procedure,
        before applying your corrections to the whole workspace and all sample runs.

        The absorption correction procedure invoked with excessive accuracy can run for too
        long providing no real improvements in accuracy. This is why it is recommended to
        run this procedure evaluating absorption on selected detectors and
        deploy the corrections to the whole runs only after achieving satisfactory accuracy
        and execution time.

        The procedure evaluate and prints the expected time to run the absorption corrections
        on the whole run.

        Input:
        If provided, the pointer or the name of the workspace available in analysis data service.
        If it is not, the workspace is taken from PropertyManager.sample_run property

        Usage:
        Reduce single run and uncomment this method in the __main__ area to evaluate
        absorption corrections.

        Change absorption corrections parameters below to achieve best speed and
        acceptable accuracy
        """

        # Gain access to the property manager:
        propman = rd.reducer.prop_man
        # Set up Sample as one of:
        # 1) Cylinder([Chem_formula],[Height,Radius])
        # 2) FlatPlate([Chem_formula],[Height,Width,Thick])
        # 3) HollowCylinder([Chem_formula],[Height,InnerRadius,OuterRadius])
        # 4) Sphere([[Chem_formula],Radius)
        # The units are in cm
        propman.correct_absorption_on = Cylinder("Fe", [10, 2])  # Will be taken from def_advanced_properties
        #                                prop['correct_absorption_on'] =  if not defined here
        #
        # Use Monte-Carlo integration.  Take sparse energy points and a few integration attempts
        # to increase initial speed. Increase these numbers to achieve better accuracy.
        propman.abs_corr_info = {"EventsPerPoint": 3000}  # ,'NumberOfWavelengthPoints':30}
        # See MonteCarloAbsorption for all possible properties description and possibility to define
        # a sparse instrument for speed.
        #
        # Gain access to the workspace. The workspace should contain Ei log, containing incident energy
        # (or be reduced)
        if test_ws is None:
            test_ws = PropertyManager.sample_run.get_workspace()
        # Define spectra list to test absorption on
        check_spectra = [1, 200]
        # Evaluate corrections on the selected spectra of the workspace and the time to obtain
        # the corrections on the whole workspace.
        corrections, time_to_correct_abs = self.evaluate_abs_corrections(test_ws, check_spectra)
        # When accuracy and speed of the corrections is satisfactory, copy chosen abs_corr_info
        # properties from above to the advanced_porperties area to run in reduction.
        #
        return corrections

    def __init__(self, web_var_val=None):
        """sets properties defaults for the instrument with Name"""
        ReductionWrapper.__init__(self, "MAR", web_var_val)
        Mt = MethodType(self.do_preprocessing, self.reducer)
        DirectEnergyConversion.__setattr__(self.reducer, "do_preprocessing", Mt)
        Mt = MethodType(self.do_postprocessing, self.reducer)
        DirectEnergyConversion.__setattr__(self.reducer, "do_postprocessing", Mt)


# -------------------------------------------------------------------------------------------------#


if __name__ == "__main__":
    data_root = r"d:\Data\MantidDevArea\Datastore\DataCopies"
    # data_dir = os.path.join(data_root,r'Testing\Data\SystemTest')
    # ref_data_dir = os.path.join(data_root,r'Testing\SystemTests\tests\analysis\reference')
    # result_dir = r'd:/Data/Mantid_Testing/14_12_15'
    # config.setDataSearchDirs('{0};{1};{2}'.format(data_dir,ref_data_dir,result_dir))
    # config.appendDataSearchDir('d:/Data/Mantid_GIT/Test/AutoTestData')
    # config['defaultsave.directory'] = result_dir # folder to save resulting spe/nxspe files.  Defaults are in

    # execute stuff from Mantid
    rd = ReduceMARIFromFile()
    # rd= ReduceMARIMon2Norm()
    # rd = ReduceMARIMonitorsSeparate()
    # rd = ReduceMARIFromWorkspace()
    rd.def_advanced_properties()
    rd.def_main_properties()

    #### uncomment rows below to generate web variables and save then to transfer to ###
    ## web services.
    run_dir = os.path.dirname(os.path.realpath(__file__))
    filename = os.path.join(run_dir, "reduce_vars.py")
    rd.save_web_variables(filename)

    #### Set up time interval (sec) for reducer to check for input data file.  ####
    #  If this file is not present and this value is 0,reduction fails
    #  if this value >0 the reduction wait until file appears on the data
    #  search path checking after time specified below.
    rd.wait_for_file = 0  # waiting time interval

    ####get reduction parameters from properties above, override what you want locally ###
    # and run reduction.  Overriding would have form:
    # rd.reducer.property_name (from the dictionary above) = new value e.g.
    # rd.reducer.energy_bins = [-40,2,40]
    # or
    ## rd.reducer.sum_runs = False

    ###### Run reduction over all run numbers or files assigned to ######
    # sample_run variable

    # return output workspace only if you are going to do
    # something with it here.  Running range of runs will return the array
    # of workspace pointers.
    # red_ws = rd.run_reduction()
    # usual way to go is to reduce workspace and save it internally
    rd.run_reduction()

###### Test absorption corrections to find optimal settings for corrections algorithm
#     corr = rd.eval_absorption_corrections()
