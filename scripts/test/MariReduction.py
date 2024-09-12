# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
"""Sample MARI reduction scrip used in testing ReductionWrapper"""

from Direct.AbsorptionShapes import *

from Direct.ReductionWrapper import *

try:
    import reduce_vars as web_var
except:
    web_var = None
try:
    import mantidplot as mpl
except:
    mpl = None


class ReduceMARI(ReductionWrapper):
    # -------------------------------------------------------------------------------------------------#
    @MainProperties
    def def_main_properties(self):
        """Define main properties used in reduction"""
        prop = {}
        prop["sample_run"] = 11001
        prop["wb_run"] = 11060
        prop["incident_energy"] = 12
        prop["energy_bins"] = [-11, 0.05, 11]

        # Absolute units reduction properties.
        prop["monovan_run"] = 11015
        prop["sample_mass"] = 10
        prop["sample_rmm"] = 435.96
        return prop

    # -------------------------------------------------------------------------------------------------#
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
        # Uncomment two following properties to correct for absorption
        # on sample or sample container during the experiment.
        # 1) Define the sample material and sample shape:
        #
        # prop['correct_absorption_on'] = Cylinder(['Fe'],{'Height':10,'Radius':2})
        #
        #  The shapes currently available are:
        #  Cylinder, FlatPlate, HollowCylinder and Sphere
        # Each class accepts two parameters, namely dictionary or list describing
        # sample material, as accepted by SetSampleMaterial algorithm
        # and Shape parameters as accepted by SetSample algorithm.

        # Go to iPython window, type from AbsorptionShapes import * and type
        # help(Cylinder),
        # help(Sphere) or help(anAbsorptionShape) to learn more about different
        # ways of setting the material and shape parameters:
        ##----------------------------
        # 2) Provide additional parameters defining speed and accuracy of the
        #    absorption corrections algorithm in abs_corr_info dictionary.
        #
        # prop['abs_corr_info'] = {'is_mc':True}
        #
        # Two generic algorithms are currently available to
        #    correct for absorption:
        # a) MonteCarloAbsorption (invoked by 'is_mc':True key of abs_corr_info
        #    property and by default)
        # and
        # b) AbsorptionCorrections (invoked by 'is_fast':True key of
        #    abs_corr_info property). This algorithm has number of optimizations
        #    for the special shapes cases and need further scientific validations

        # All other properties, to be provided in the input dictionary of this
        # property are the non-sample related properties of the
        # correspondent correction algorithm.
        #
        # Test the speed and the accuracy of the selected algorithm
        # reducing all your data by running eval_absorption_corrections method below.
        ##-------------------------------

        return prop
        #

    # -------------------------------------------------------------------------------------------------#
    @iliad
    def reduce(self, input_file=None, output_directory=None):
        """Method executes reduction over single file
        Modify only if custom pre or post-processing is needed, namely:
        """

        """
        Define custom preprocessing procedure, to be applied to the whole
        run, obtained from the instrument before diagnostics is applied
      1) Get access to the pointer to the input workspace
         (the workspace will be loaded  and calibrated at this point if it has not been done yet)
          using sample_run property class:
      run_ws = PropertyManager.sample_run.get_worksapce()
      2) Get access to any property defined in Instrument_Properties.xml file
         or redefined in the reduction script:
      properties = self.reducer.prop_man
        e.g:
      RunNumber = properties.sample_run
      ei = properties.incident_energy
        Perform custom preprocessing procedure (with the values you specified)
      preprocessed_ws = custom_preprocessing_procedure(run_ws,RunNumber,ei,...)
      3) Store preprocessed workspace in the sample_run property for further analysis from preprocessing
         for the case where the workspace name have changed in the Analysis Data Service
        (this situation is difficult to predict so better always do this operation)
      PropertyManager.sample_run.synchronize_ws(preprocessed_ws)
      """

        WS = ReductionWrapper.reduce(self, input_file, output_directory)

        """ Defined custrom post-processing procedure, in the way, similar to
      the preprocessing procedure, using the WS pointer, returned by the reduction procedure
      above. If the run is reduced in multirep mode, the WS is the list of the reduced
      workspace pointers, so the procedure should be applied to each workspace.

      At this stage standard saving had already been done, so save your results if you need them
      in a future:
      for i in range(0:len(WS)):
        SaveNexus(WS[i],Filename = 'CustomFilenameN%d.nxs'.format(i))
      """
        return WS
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

    #
    def set_custom_output_filename(self):
        """define custom name of output files if standard one is not satisfactory

        In addition to that, example of accessing complex reduction properties
        Simple reduction properties can be accessed as e.g.: value= prop_man.sum_runs
        """

        def custom_name(prop_man):
            """Sample function which builds filename from
            incident energy and run number and adds some auxiliary information
            to it.
            """
            # Note -- properties have the same names as the list of advanced
            # and   main properties
            # Note: the properties are stored in prop_man class and accessed as
            # below.
            ei = PropertyManager.incident_energy.get_current()
            # sample run is more then just list of runs, so we use
            # the formalization below to access its methods
            if self.reducer.prop_man.filename_prefix:
                return reduced_filename(0, ei, False, self.reducer.prop_man.filename_prefix)
            else:
                runs_list = PropertyManager.sample_run.get_run_list()
                return reduced_filename(runs_list, ei, self.reducer.prop_man.sum_runs)

        # Uncomment this to use custom filename function
        # return lambda : custom_name(self.reducer.prop_man)
        # Uncomment this to use standard file name generating function
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
        adsorption corrections.

        Change adsorption corrections parameters below to achieve best speed and
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
        # Define spectra list to test absorption on. Empty list will
        # define absorption on the whole workspace.
        check_spectra = [1, 200]
        # Evaluate corrections on the selected spectra of the workspace and the time to obtain
        # the corrections on the whole workspace.
        corrections, time_to_correct_abs = self.evaluate_abs_corrections(test_ws, check_spectra)
        # When accuracy and speed of the corrections is satisfactory, copy chosen abs_corr_info
        # properties from above to the advanced_porperties area to run in reduction.
        if mpl is not None:
            n_spectra = len(check_spectra)
            if n_spectra == 0:
                n_spectra = corrections.getNumberHistograms()
            mpl.plotSpectrum(corrections, range(0, n_spectra))
        #
        return corrections

    # ------------------------------------------------------------------------------------------------

    def __init__(self, web_var=None):
        """sets properties defaults for the instrument with Name"""
        ReductionWrapper.__init__(self, "MAR", web_var)
        Mt = MethodType(self.do_preprocessing, self.reducer)
        DirectEnergyConversion.__setattr__(self.reducer, "do_preprocessing", Mt)
        Mt = MethodType(self.do_postprocessing, self.reducer)
        DirectEnergyConversion.__setattr__(self.reducer, "do_postprocessing", Mt)


# -------------------------------------------------------------------------------------------------#
def main(input_file=None, output_directory=None):
    """This method is used to run code from web service
    and should not be touched unless you change the name of the
    particular ReductionWrapper class (e.g. ReduceMARI here)

    exception to change the output folder to save data to
    """
    # note web variables initialization
    rd = ReduceMARI(web_var)
    rd.reduce(input_file, output_directory)

    # Define folder for web service to copy results to
    output_folder = ""
    return output_folder


if __name__ == "__main__" or __name__ == "__builtin__" or __name__ == "mantidqt.widgets.codeeditor.execution":
    # -------------------------------------------------------------------------------------------------#
    # SECTION USED TO RUN REDUCTION FROM MANTID SCRIPT WINDOW #
    # -------------------------------------------------------------------------------------------------#
    ##### Here one sets up folders where to find input data and where to save results             #####
    # It can be done here or from Mantid GUI
    # Folder where map files are located:
    # map_mask_dir = 'd:/Data/MantidSystemTests/Data'
    # folder where input data can be found
    # data_dir = 'd:/Data/Mantid_Testing/14_11_27'
    # auxiliary folder with results
    # ref_data_dir = 'd:/Data/MantidSystemTests/SystemTests/AnalysisTests/ReferenceResults'
    # Set input path to
    # config.setDataSearchDirs('{0};{1};{2}'.format(data_dir,map_mask_dir,ref_data_dir))
    # use appendDataSearch directory to add to existing data search path
    # config.appendDataSearchDir('d:/Data/Mantid_GIT/Test/AutoTestData')
    # folder to save resulting spe/nxspe files.
    # config['defaultsave.directory'] = data_dir

    ###### Initialize reduction class above and set up reduction properties. Note no parameters  ######
    rd = ReduceMARI()
    # set up advanced and main properties
    rd.def_advanced_properties()
    rd.def_main_properties()

    # uncomment rows below to save web variables to use in web services.
    # run_dir = os.path.dirname(os.path.realpath(__file__))
    # file = os.path.join(run_dir,'reduce_vars.py')
    # rd.save_web_variables(file)

    # Web-like reduction over sequence of files
    # rd.reduce()
    ###### Run reduction on all files provided as parameters                                      ######
    rd.run_reduction()

###### Test absorption corrections to find optimal settings for corrections algorithm         ######
#     corr = rd.eval_absorption_corrections()
