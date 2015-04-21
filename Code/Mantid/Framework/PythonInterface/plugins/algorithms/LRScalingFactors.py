#pylint: disable=invalid-name
import numpy
import os
from mantid.api import *
from mantid.simpleapi import *
from mantid.kernel import *

class LRScalingFactors(PythonAlgorithm):
    """
        This algorithm runs through a sequence of direct beam data sets
        to extract scaling factors. The method was developed by J. Ankner (ORNL).

        As we loop through, we find matching data sets with the only
        difference between the two is an attenuator.
        The ratio of those matching data sets allows use to rescale
        a direct beam run taken with a larger number of attenuators
        to a standard data set taken with tighter slit settings and
        no attenuators.

        The normalization run for a data set taken in a given slit setting
        configuration can then be expressed in terms of the standard 0-attenuator
        data set with:
            D_i = F_i D_0

        Here's an example of runs and how they are related to F.

        run: 55889, att: 0, s1: 0.26, s2: 0.26
        run: 55890, att: 1, s1: 0.26, s2: 0.26
        run: 55891, att: 1, s1: 0.33, s2: 0.26 --> F = 55891 / 55890
        run: 55892, att: 1, s1: 0.45, s2: 0.26 --> F = 55892 / 55890
        run: 55895, att: 1, s1: 0.81, s2: 0.26
        run: 55896, att: 2, s1: 0.81, s2: 0.26
        run: 55897, att: 2, s1: 1.05, s2: 0.35 --> F = 55897 / 55896 * 55895 / 55890

    """
    def category(self):
        return "Reflectometry\\SNS"

    def name(self):
        return "LiquidsReflectometryScalingFactors"

    def version(self):
        return 1

    def summary(self):
        return "Liquids Reflectometer (REFL) scaling factor calculation"

    def PyInit(self):
        self.declareProperty(IntArrayProperty("DirectBeamRuns", []),
                             "Run number of the signal run to use")
        self.declareProperty(IntArrayProperty("Attenuators", []),
                             "Number of attenuators for each run")
        self.declareProperty(FloatArrayProperty("TOFRange", [10000., 35000.],
                                                FloatArrayLengthValidator(2),
                                                direction=Direction.Input),
                                                "TOF range to use")
        self.declareProperty(IntArrayProperty("SignalPeakPixelRange", [150, 160]),
                                              "Pixel range defining the data peak")
        self.declareProperty(IntArrayProperty("SignalBackgroundPixelRange", [147, 163]),
                                              "Pixel range defining the background")
        self.declareProperty(IntArrayProperty("LowResolutionPixelRange", [94, 160],
                                              IntArrayLengthValidator(2),
                                              direction=Direction.Input),
                                              "Pixel range defining the region to use in the low-resolution direction")
        self.declareProperty("IncidentMedium", "Air", doc="Name of the incident medium")
        self.declareProperty("FrontSlitName", "S1", doc="Name of the front slit")
        self.declareProperty("BackSlitName", "Si", doc="Name of the back slit")
        self.declareProperty("TOFSteps", 50.0, doc="TOF step size")
        self.declareProperty(FileProperty("ScalingFactorFile","",
                                          action=FileAction.Save,
                                          extensions=['cfg']))

    def PyExec(self):
        # Verify whether we have a sorted list of runs.
        data_runs = self.getProperty("DirectBeamRuns").value

        # Check whether the supplied attenuation array is of the same length,
        # otherwise we will deduce the number of attenuators.
        attenuators = self.getProperty("Attenuators").value
        have_attenuator_info = False
        if len(attenuators)==0:
            logger.notice("No attenuator information supplied: will be determined.")
        elif not len(attenuators) == len(data_runs):
            logger.error("Attenuation list should be of the same length as the list of runs")
        else:
            have_attenuator_info = True
        if have_attenuator_info is False:
            attenuators = len(data_runs)*[0]
            
        # Get the slit information
        front_slit = self.getProperty("FrontSlitName").value
        back_slit = self.getProperty("BackSlitName").value
        medium = self.getProperty("IncidentMedium").value

        # Get peak ranges
        peak_range = self.getProperty("SignalPeakPixelRange").value
        background_range = self.getProperty("SignalBackgroundPixelRange").value

        # Supply good values for peak ranges
        # If we supplied two values, use those boundaries for each run
        if len(peak_range)==2:
            x_min = int(peak_range[0])
            x_max = int(peak_range[1])
            peak_range = 2*len(data_runs)*[0]
            for i in range(len(data_runs)):
                peak_range[2*i] = x_min
                peak_range[2*i+1] = x_max
        elif len(peak_range) < 2:
            raise RuntimeError, "SignalPeakPixelRange should have a length of at least 2."

        if len(background_range)==2:
            x_min = int(background_range[0])
            x_max = int(background_range[1])
            background_range = 2*len(data_runs)*[0]
            for i in range(len(data_runs)):
                background_range[2*i] = x_min
                background_range[2*i+1] = x_max
        elif len(background_range) < 2:
            raise RuntimeError, "SignalBackgroundPixelRange should have a length of at least 2."

        # Check that the peak range arrays are of the proper length
        if not (len(peak_range) == 2*len(data_runs) \
                and len(background_range) == 2*len(data_runs)):
            raise RuntimeError, "Supplied peak/background arrays should be of the same length as the run array."

        # Slit information for the previous run (see loop below)
        previous_slits = None

        # Previous processed workspace
        previous_ws = None

        # Number of attenuators for the run being considered
        n_attenuator = 0

        # Transition references used to propagate the attenuation
        references = {}

        # Wavelength value
        wavelength = None

        # Scaling factor output
        scaling_factors = []

        # Run through the runs
        for i in range(len(data_runs)):
            run = data_runs[i]
            workspace_name = "REF_L_%s" % int(run)
            workspace = LoadEventNexus("REF_L_%s" % run,
                                       OutputWorkspace = workspace_name)

            # Get S1H, S2H, S1W, S2W
            s1h = abs(workspace.getRun().getProperty("%sVHeight" % front_slit).value[0])
            s1w = abs(workspace.getRun().getProperty("%sHWidth" % front_slit).value[0])
            try:
                s2h = abs(workspace.getRun().getProperty("%sVHeight" % back_slit).value[0])
                s2w = abs(workspace.getRun().getProperty("%sHWidth" % back_slit).value[0])
            except:
                # For backward compatibility with old code
                logger.error("Specified slit could not be found: %s  Trying S2" % back_slit)
                s2h = abs(workspace.getRun().getProperty("S2VHeight").value[0])
                s2w = abs(workspace.getRun().getProperty("S2HWidth").value[0])

            # Get wavelength, to make sure they all match across runs
            wl = workspace.getRun().getProperty('LambdaRequest').value[0]
            if wavelength is None:
                wavelength = wl
            elif abs(wl-wavelength) > 0.2:
                raise RuntimeError, "Supplied runs don't have matching wavelengths."

            peak = [int(peak_range[2*i]), int(peak_range[2*i+1])]
            background = [int(background_range[2*i]), int(background_range[2*i+1])]
            self.process_data(workspace,
                              peak_range=peak,
                              background_range=background)
            
            # If we don't have the attenuator information and we have the
            # same slit settings as the previous run, it means we added an
            # attenuator.
            if have_attenuator_info is True:
                if attenuators[i] < n_attenuator:
                    raise RuntimeError, "Runs were not supplied in increasing number of attenuators."
                n_attenuator = attenuators[i]

            is_reference = False
            self.tolerance = 0.02

            # Matching slits with the previous run signals a reference run
            if i > 0 and previous_slits is not None \
                and abs(previous_slits[0] - s1h) < self.tolerance \
                and abs(previous_slits[1] - s1w) < self.tolerance \
                and abs(previous_slits[2] - s2h) < self.tolerance \
                and abs(previous_slits[3] - s2w) < self.tolerance:
                is_reference = True
                
                # This signals an attenuation number change if we happen to need the info
                if have_attenuator_info is False:
                    n_attenuator += 1
                    attenuators[i] = n_attenuator

            previous_slits = [s1h, s1w, s2h, s2w]

            # If the number of attenuators is zero, skip.
            if n_attenuator == 0:
                if references.has_key(0):
                    raise RuntimeError, "More than one run with zero attenuator was supplied."
                references[0] = {'index': i,
                                 'run': run,
                                 'ref_ws': workspace_name,
                                 'ratio_ws': None}
                previous_ws = workspace_name
                continue

            if is_reference is True:
                references[n_attenuator] = {'index': i,
                                            'run': run,
                                            'ref_ws': workspace_name}
                # Compute ratio of the run with fewer attenuators and the
                # reference with the same number of attenuators as that run.
                Divide(LHSWorkspace = previous_ws,
                       RHSWorkspace = references[n_attenuator-1]['ref_ws'],
                       OutputWorkspace = "ScalingRatio_%s" % n_attenuator)
                # Multiply the result by the ratio for that run, and store
                if references[n_attenuator-1]['ratio_ws'] is not None:
                    Multiply(LHSWorkspace = references[n_attenuator-1]['ratio_ws'],
                             RHSWorkspace = "ScalingRatio_%s" % n_attenuator,
                             OutputWorkspace = "ScalingRatio_%s" % n_attenuator)
                references[n_attenuator]['ratio_ws'] = "ScalingRatio_%s" % n_attenuator
            # If this is not a reference run, compute F
            else:
                # Divide by the reference for this number of attenuators
                # and multiply by the reference ratio
                if not references.has_key(n_attenuator):
                    raise RuntimeError, "No reference for %s attenuators: check run ordering." % n_attenuator
                f_ws = "F_%s_%s" % (run, n_attenuator)
                Divide(LHSWorkspace=workspace_name,
                       RHSWorkspace=references[n_attenuator]['ref_ws'],
                       OutputWorkspace=f_ws)
                Multiply(LHSWorkspace=references[n_attenuator]['ratio_ws'],
                         RHSWorkspace=f_ws,
                         OutputWorkspace=f_ws)
                # Store the final result for this setting
                ReplaceSpecialValues(InputWorkspace=f_ws, OutputWorkspace=f_ws,
                                     NaNValue=0.0, NaNError=1000.0,
                                     InfinityValue=0.0, InfinityError=1000.0)

                # Remove prompt pulse bin, replace the y value by the 
                # average and give it a very large error.
                x_values = mtd[f_ws].readX(0)
                y_values = mtd[f_ws].dataY(0)
                average = numpy.average(y_values)
                e_values = mtd[f_ws].dataE(0)
                for i in range(len(y_values)):
                    # Go up to 4 frames - that should cover more than enough TOF
                    for n in range(1, 4):
                        peak_x = 1.0e6 / 60.0 * n
                        if peak_x > x_values[i] and peak_x < x_values[i+1]:
                            y_values[i] = average
                            e_values[i] = average*100.0

                Fit(InputWorkspace=f_ws,
                    Function="name=UserFunction, Formula=a+b*x, a=1, b=0",
                    Output='fit_result')
                a = mtd['fit_result_Parameters'].cell(0,1)
                b = mtd['fit_result_Parameters'].cell(1,1)
                error_a = mtd['fit_result_Parameters'].cell(0,2)
                error_b = mtd['fit_result_Parameters'].cell(1,2)
                scaling_factors.append({'IncidentMedium': medium,
                                        'LambdaRequested': wl,
                                        'S1H':s1h, 'S1W':s1w,
                                        'S2iH':s2h, 'S2iW':s2w,
                                        'a':a, 'error_a': error_a, 
                                        'b':b, 'error_b': error_b})

            previous_ws = workspace_name
        # Save the output in a configuration file
        self.save_scaling_factor_file(scaling_factors)

    def save_scaling_factor_file(self, scaling_factors):
        """
            Save the output. First see if the scaling factor file exists.
            If it does, we need to update it.
            @param scaling_factors: list of scaling factor dictionaries
        """
        scaling_file = self.getPropertyValue("ScalingFactorFile")
        scaling_file_content = []
        if os.path.isfile(scaling_file):
            fd = open(scaling_file, 'r')
            content = fd.read()
            fd.close()
            for line in content.split('\n'):
                if line.startswith('#') or len(line.strip()) == 0:
                    continue
                toks = line.split()
                entry = {}
                for token in toks:
                    pair = token.split('=')
                    entry[pair[0].strip()] = pair[1].strip()
                # If we are about to update an entry, don't include it in the new file
                add_this_entry = True
                for new_entry in scaling_factors:
                    is_matching = entry["IncidentMedium"] == new_entry["IncidentMedium"]
                    for slit in ["LambdaRequested", "S1H", "S1W", "S2iH", "S2iW"]:
                        is_matching = is_matching \
                            and abs(float(entry[slit])-float(new_entry[slit])) < self.tolerance
                    if is_matching:
                        add_this_entry = False
                if add_this_entry:
                    scaling_file_content.append(entry)
        scaling_file_content.extend(scaling_factors)

        fd = open(scaling_file, 'w')
        fd.write("#y=a+bx\n#\n")
        fd.write("#lambdaRequested[Angstroms] S1H[mm] (S2/Si)H[mm] S1W[mm] (S2/Si)W[mm] a b error_a error_b\n#\n")
        for item in scaling_file_content:
            fd.write("IncidentMedium=%s " % item["IncidentMedium"])
            fd.write("LambdaRequested=%s " % item["LambdaRequested"])
            fd.write("S1H=%s " % item["S1H"])
            fd.write("S2iH=%s " % item["S2iH"])
            fd.write("S1W=%s " % item["S1W"])
            fd.write("S2iW=%s " % item["S2iW"])
            fd.write("a=%s " % item["a"])
            fd.write("b=%s " % item["b"])
            fd.write("error_a=%s " % item["error_a"])
            fd.write("error_b=%s\n" % item["error_b"])
        fd.close()

    def process_data(self, workspace, peak_range, background_range):
        """
            Common processing for both sample data and normalization.
            @param workspace: name of the workspace to work with
            @param peak_range: range of pixels defining the peak
            @param background_range: range of pixels defining the background
        """
        # Rebin TOF axis
        tof_range = self.getProperty("TOFRange").value
        tof_step = self.getProperty("TOFSteps").value
        workspace = Rebin(InputWorkspace=workspace, Params=[tof_range[0], tof_step, tof_range[1]], 
                          PreserveEvents=False, OutputWorkspace=str(workspace))

        # Integrate over low resolution range
        low_res_range = self.getProperty("LowResolutionPixelRange").value
        x_min = int(low_res_range[0])
        x_max = int(low_res_range[1])

        # Subtract background
        workspace = LRSubtractAverageBackground(InputWorkspace=workspace,
                                                PeakRange=peak_range,
                                                BackgroundRange=background_range,
                                                LowResolutionRange=[x_min, x_max],
                                                OutputWorkspace=str(workspace))

        # Normalize by current proton charge
        # Note that the background subtraction will use an error weighted mean
        # and use 1 as the error on counts of zero. We normalize by the integrated
        # current _after_ the background subtraction so that the 1 doesn't have
        # to be changed to a 1/Charge.
        workspace = NormaliseByCurrent(InputWorkspace=workspace, 
                                       OutputWorkspace=str(workspace))

        # Crop to only the selected peak region
        workspace = CropWorkspace(InputWorkspace=workspace,
                                  StartWorkspaceIndex=int(peak_range[0]),
                                  EndWorkspaceIndex=int(peak_range[1]),
                                  OutputWorkspace=str(workspace))
        workspace = SumSpectra(InputWorkspace=workspace,
                               OutputWorkspace=str(workspace))

        return str(workspace)

AlgorithmFactory.subscribe(LRScalingFactors)