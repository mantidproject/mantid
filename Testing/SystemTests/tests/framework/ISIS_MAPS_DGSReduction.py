# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=invalid-name
"""Sample MAPS reduction scrip"""

from Direct.AbsorptionShapes import Cylinder
from Direct.DirectEnergyConversion import DirectEnergyConversion
from Direct.ReductionWrapper import AdvancedProperties, iliad, MainProperties, MethodType, ReductionWrapper
from mantid.kernel import config, PropertyManager

try:
    import reduce_vars as web_var
except ModuleNotFoundError:
    web_var = None


class ReduceMAPS(ReductionWrapper):
    @MainProperties
    def def_main_properties(self):
        """Define main properties used in reduction"""
        prop = {}
        prop["sample_run"] = 17269
        prop["wb_run"] = 17186
        prop["incident_energy"] = 150
        prop["energy_bins"] = [-15, 3, 135]

        # Absolute units reduction properties.
        prop["monovan_run"] = 17589
        # -- this number allows to get approximately the same system test intensities for MAPS as the old test
        prop["sample_mass"] = 10 / (94.4 / 13)
        prop["sample_rmm"] = 435.96  #
        return prop

    @AdvancedProperties
    def def_advanced_properties(self):
        """separation between simple and advanced properties depends
        on scientist, experiment and user.
        main properties override advanced properties.
        """
        prop = {}
        prop["map_file"] = "default"
        # prop['monovan_mapfile'] = 'default' #'4to1_mid_lowang.map' # default
        prop["hard_mask_file"] = None
        # prop['det_cal_file'] = ?  default?
        prop["save_format"] = ""

        prop["diag_remove_zero"] = False

        # this are the parameters which were used in old MAPS_Parameters.xml test.
        prop["wb-integr-max"] = 300
        # prop['wb_integr_range']=[20,300]
        prop["bkgd-range-min"] = 12000
        prop["bkgd-range-max"] = 18000
        # prop['bkgd_range']=[12000,18000]

        prop["diag_samp_hi"] = 1.5
        prop["diag_samp_sig"] = 3.3
        prop["diag_van_hi"] = 2.0

        prop["abs_units_van_range"] = [-40, 40]

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

    def __init__(self, web_var=None):
        """sets properties defaults for the instrument with Name"""
        ReductionWrapper.__init__(self, "MAP", web_var)
        Mt = MethodType(self.do_preprocessing, self.reducer)
        DirectEnergyConversion.__setattr__(self.reducer, "do_preprocessing", Mt)
        Mt = MethodType(self.do_postprocessing, self.reducer)
        DirectEnergyConversion.__setattr__(self.reducer, "do_postprocessing", Mt)

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

    def set_custom_output_filename(self):
        """define custom name of output files if standard one is not satisfactory
        In addition to that, example of accessing reduction properties
        Changing them if necessary
        """

        def custom_name(prop_man):
            """sample function which builds filename from
            incident energy and run number and adds some auxiliary information
            to it.
            """
            # Note -- properties have the same names as the list of advanced and
            # main properties
            ei = prop_man.incident_energy
            # sample run is more then just list of runs, so we use
            # the formalization below to access its methods
            run_num = PropertyManager.sample_run.run_number()
            name = "RUN{0}atEi{1:<4.1f}meV_One2One".format(run_num, ei)
            return name

        # Uncomment this to use custom filename function
        # Note: the properties are stored in prop_man class accessed as
        # below.
        # return lambda : custom_name(self.reducer.prop_man)
        # use this method to use standard file name generating function
        return None

    #

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


if __name__ == "__main__":
    import os

    os.environ["PATH"] = r"c:\Mantid\Code\builds\br_master\bin\Release;" + os.environ["PATH"]

    data_root = r"d:\Data\MantidDevArea\Datastore\DataCopies"
    data_dir = os.path.join(data_root, r"Testing\Data\SystemTest")
    ref_data_dir = os.path.join(data_root, r"Testing\SystemTests\tests\framework\reference")
    result_dir = r"d:/Data/Mantid_Testing/14_12_15"

    config.setDataSearchDirs("{0};{1};{2}".format(data_dir, ref_data_dir, result_dir))
    # config.appendDataSearchDir('d:/Data/Mantid_GIT/Test/AutoTestData')
    config["defaultsave.directory"] = result_dir  # folder to save resulting spe/nxspe files.  Defaults are in

    # execute stuff from Mantid
    rd = ReduceMAPS()
    rd.def_advanced_properties()
    rd.def_main_properties()

    #### uncomment rows below to generate web variables and save then to transfer to ###
    ## web services.
    # run_dir = os.path.dirname(os.path.realpath(__file__))
    # file = os.path.join(run_dir,'reduce_vars.py')
    # rd.save_web_variables(file)

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
