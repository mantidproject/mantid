"""
Provide functions to perform various diagnostic tests.

The output of each function is a workspace containing a single bin where:
    0 - denotes a masked spectra and
    1 - denotes an unmasked spectra.

This workspace can be summed with other masked workspaces to accumulate
masking and also passed to MaskDetectors to match masking there.
"""
from mantidsimple import *
import CommonFunctions as common

def diagnose(white_run, sample_run=None, other_white=None, remove_zero=None, 
             tiny=None, large=None, median_lo=None, median_hi=None, signif=None, 
             bkgd_threshold=None, bkgd_range=None, variation=None,
             bleed_test=False, bleed_maxrate=None, bleed_pixels=None,
             hard_mask=None, print_results=False, inst_name=None):
    """
    Run diagnostics on the provided run and white beam files.

    There are 4 possible tests, depending on the input given:
      White beam diagnosis
      Background tests
      Second white beam
      Bleed test
    
    Required inputs:
    
      white_run  - The run number or filepath of the white beam run
    
    Optional inputs:
      sample_run - The run number or filepath of the sample run for the background test (default = None)
      other_white   - If provided an addional set of tests is performed on this file. (default = None)
      remove_zero - If true then zeroes in the data will count as failed (default = False)
      tiny          - Minimum threshold for acceptance (default = 1e-10)
      large         - Maximum threshold for acceptance (default = 1e10)
      median_lo     - Fraction of median to consider counting low (default = 0.1)
      median_hi     - Fraction of median to consider counting high (default = 3.0)
      signif           - Counts within this number of multiples of the 
                      standard dev will be kept (default = 3.3)
      bkgd_threshold - High threshold for background removal in multiples of median (default = 5.0)
      bkgd_range - The background range as a list of 2 numbers: [min,max]. 
                   If not present then they are taken from the parameter file. (default = None)
      variation  - The number of medians the ratio of the first/second white beam can deviate from
                   the average by (default=1.1)
      bleed_test - If true then the CreatePSDBleedMask algorithm is run
      bleed_maxrate - If the bleed test is on then this is the maximum framerate allowed in a tube
      bleed_pixels - If the bleed test is on then this is the number of pixels ignored within the
                     bleed test diagnostic
      hard_mask  - A file specifying those spectra that should be masked without testing
      print_results - If True then the results are printed to std out
      inst_name  - The name of the instrument to perform the diagnosis.
                   If it is not provided then the default instrument is used (default = None)
    """
    # Set the default instrument so that Mantid can search for the runs correctly, 
    # but switch it back at the end
    def_inst = mtd.settings["default.instrument"]
    if inst_name == None:
        inst_name = def_inst
    elif inst_name != def_inst:
        mtd.settings["default.instrument"] = inst_name
    else: pass

    # Load the hard mask file if necessary
    hard_mask_spectra = ''
    if hard_mask is not None:
        hard_mask_spectra = common.load_mask(hard_mask)

    # Map the test number to the results
    # Each element is the mask workspace name then the number of failures
    test_results = [ [None, None], [None, None], [None, None], [None, None]]

    ##
    ## Accumulate the masking on this workspace
    ##
    # What shall we call the output
    lhs_names = lhs_info('names')
    if len(lhs_names) > 0:
        diag_total_mask = lhs_names[0]
    else:
        diag_total_mask = 'diag_total_mask'

    ##
    ## White beam Test
    ##
    white_counts = None
    if white_run is not None and str(white_run) != '':
        # Load and integrate
        data_ws = common.load_run(white_run)
        # Make sure we have a matrix workspace otherwise Integration() returns all zeros.
        ConvertToMatrixWorkspace(data_ws, data_ws)
        white_counts = Integration(data_ws, "__counts_white-beam").workspace()
        MaskDetectors(white_counts, SpectraList=hard_mask_spectra)
        # Run first white beam tests
        __white_masks, num_failed = \
                      _do_white_test(white_counts, tiny, large, median_lo, median_hi, signif)
        test_results[0] = [str(__white_masks), num_failed]
        diag_total_mask = CloneWorkspace(__white_masks, diag_total_mask).workspace()
    else:
        raise RuntimeError('Invalid input for white run "%s"' % str(white_run))

    ##
    ## Second white beam Test
    ##
    second_white_counts = None
    if other_white is not None and str(other_white) != '':
        # Load and integrate
        data_ws = common.load_run(other_white)
        second_white_counts = Integration(data_ws, "__counts_white-beam2").workspace()
        MaskDetectors(white_counts, SpectraList=hard_mask_spectra)
        # Run tests
        __second_white_masks, num_failed = \
                             _do_second_white_test(white_counts, second_white_counts,
                                                   tiny, large, median_lo, median_hi,
                                                   signif,variation)
        test_results[1] = [str(__second_white_masks), num_failed]
        # Accumulate
        diag_total_mask += __second_white_masks

    ##
    ## Tests on the sample(s)
    ##
    if sample_run is not None and sample_run != '':
        if type(sample_run) != list:
            sample_run = [sample_run]
        __bkgd_masks = None
        __bleed_masks = None
        bkgd_failures = 0
        bleed_failures = 0
        for sample in sample_run:
            ## Background test
            __bkgd_masks_i, failures = _do_background_test(sample, white_counts, second_white_counts,
                                                           bkgd_range, bkgd_threshold, remove_zero, signif,
                                                           hard_mask_spectra)
            bkgd_failures += failures
            __bkgd_masks = _accumulate(__bkgd_masks,__bkgd_masks_i)
                
            ## PSD Bleed Test
            if type(bleed_test) == bool and bleed_test == True:
                __bleed_masks_i, failures = _do_bleed_test(sample, bleed_maxrate, bleed_pixels)
                bleed_failures += failures
                __bleed_masks = _accumulate(__bleed_masks, __bleed_masks_i)

        ## End of sample loop
        test_results[2] = [str(__bkgd_masks), bkgd_failures]
        diag_total_mask += __bkgd_masks
        if __bleed_masks is not None:
            test_results[3] = [str(__bleed_masks), bleed_failures]
            diag_total_mask += __bleed_masks

    ## End of background test

    # Remove temporary workspaces
    mtd.deleteWorkspace(str(white_counts))
    if second_white_counts is not None:
        mtd.deleteWorkspace(str(second_white_counts))
    
    # Revert our default instrument changes if necessary
    if inst_name != def_inst:
        mtd.settings["default.instrument"] = def_inst

    if print_results:
        print_test_summary(test_results)
    
    # This will be a MaskWorkspace which contains the accumulation of all of the masks
    return diag_total_mask

#-------------------------------------------------------------------------------

def _do_white_test(white_counts, tiny, large, median_lo, median_hi, signif):
    """
    Run the diagnostic tests on the integrated white beam run

    Required inputs:
    
      white_counts  - A workspace containing the integrated counts from a
                      white beam vanadium run
      tiny          - Minimum threshold for acceptance
      large         - Maximum threshold for acceptance
      median_lo     - Fraction of median to consider counting low
      median_hi     - Fraction of median to consider counting high
      signif        - Counts within this number of multiples of the 
                      standard dev will be kept (default = 3.3)
    """
    mtd.sendLogMessage('Running first white beam test')

    # What shall we call the output
    lhs_names = lhs_info('names')
    if len(lhs_names) > 0:
        ws_name = lhs_names[0]
    else:
        ws_name = '__do_white_test'

    # Make sure we are a MatrixWorkspace
    ConvertToMatrixWorkspace(white_counts, white_counts)
    
    # The output workspace will have the failed detectors masked
    range_check = FindDetectorsOutsideLimits(white_counts, ws_name, HighThreshold=large, LowThreshold=tiny)
    median_test = MedianDetectorTest(white_counts, ws_name, SignificanceTest=signif,
                                     LowThreshold=median_lo, HighThreshold=median_hi)

    num_failed = range_check['NumberOfFailures'].value + median_test['NumberOfFailures'].value

    maskWS = median_test.workspace()
    MaskDetectors(white_counts, MaskedWorkspace=maskWS)
    return maskWS, num_failed

#-------------------------------------------------------------------------------

def _do_second_white_test(white_counts, comp_white_counts, tiny, large, median_lo,
                          median_hi, signif, variation):
    """
    Run additional tests comparing given another white beam count workspace, comparing
    to the first

    Required inputs:
    
      white_counts  - A workspace containing the integrated counts from a
                      white beam vanadium run
      comp_white_counts  - A workspace containing the integrated counts from a
                      white beam vanadium run
      tiny          - Minimum threshold for acceptance
      large         - Maximum threshold for acceptance
      median_lo     - Fraction of median to consider counting low
      median_hi     - Fraction of median to consider counting high
      signif          - Counts within this number of multiples of the 
                      standard dev will be kept
      variation     - Defines a range within which the ratio of the two counts is
                      allowed to fall in terms of the number of medians
    """ 
    mtd.sendLogMessage('Running second white beam test')   

    # What shall we call the output
    lhs_names = lhs_info('names')
    if len(lhs_names) > 0:
        ws_name = lhs_names[0]
    else:
        ws_name = '__do_second_white_test'
    
    # Make sure we are a MatrixWorkspace
    ConvertToMatrixWorkspace(white_counts, white_counts)
    ConvertToMatrixWorkspace(comp_white_counts, comp_white_counts)
    
    # Do the white beam test
    __second_white_tests, failed = _do_white_test(comp_white_counts, tiny, large, median_lo, median_hi, signif)
    # and now compare it with the first
    effic_var = DetectorEfficiencyVariation(white_counts, comp_white_counts, ws_name, Variation=variation)
    # Total number of failures
    num_failed = effic_var['NumberOfFailures'].value + failed

    mtd.deleteWorkspace(str(__second_white_tests))
    # Mask those that failed
    maskWS = effic_var.workspace()
    MaskDetectors(white_counts, MaskedWorkspace=maskWS)
    MaskDetectors(comp_white_counts, MaskedWorkspace=maskWS)
  
    return maskWS, num_failed

#------------------------------------------------------------------------------

def _do_background_test(sample_run, white_counts, comp_white_counts, bkgd_range,
                        bkgd_threshold, remove_zero, signif, hard_mask_spectra):
    """
    Run the background tests on the integrated sample run normalised by an
    integrated white beam run

    Required inputs:
    
      sample_run          - The run number or filepath of the sample run
      white_counts        - A workspace containing the integrated counts from a
                            white beam vanadium run
      comp_white_counts - A second workspace containing the integrated counts from a
                            different white beam vanadium run
      bkgd_range          - The background range as a list of 2 numbers: [min,max]. 
                            If not present then they are taken from the parameter file.
      bkgd_threshold      - High threshold for background removal in multiples of median
      remove_zero         - If true then zeroes in the data will count as failed (default = False)
      signif              - Counts within this number of multiples of the 
                            standard dev will be kept (default = 3.3)
      prev_ma sks         - If present then this masking is applied to the sample run before the
                            test is applied. It is expected as a MaskWorkspace
    """
    mtd.sendLogMessage('Running background count test')
    # Load and integrate the sample using the defined background range. If none is given use the
    # instrument defaults
    data_ws = common.load_runs(sample_run)
    instrument = data_ws.getInstrument()
    if bkgd_range is None:
        min_value = float(instrument.getNumberParameter('bkgd-range-min')[0])
        max_value = float(instrument.getNumberParameter('bkgd-range-max')[0])
    elif len(bkgd_range) != 2:
        raise ValueError("The provided background range is incorrect. A list of 2 numbers is required.")
    else:
        min_value = bkgd_range[0]
        if min_value is None:
            float(instrument.getNumberParameter('bkgd-range-min')[0])
        max_value = bkgd_range[1]
        if max_value is None:
            float(instrument.getNumberParameter('bkgd-range-max')[0])

    # Get the total counts
    sample_counts = Integration(data_ws, '__counts_mono-sample', RangeLower=min_value, \
                                RangeUpper=max_value).workspace()

    # Apply hard mask spectra and previous masking first
    MaskDetectors(sample_counts, SpectraList=hard_mask_spectra, MaskedWorkspace=white_counts)
    
    # If we have another white beam then compute the harmonic mean of the counts
    # The harmonic mean: 1/av = (1/Iwbv1 + 1/Iwbv2)/2
    # We'll resuse the comp_white_counts workspace as we don't need it anymore
    white_count_mean = white_counts
    if comp_white_counts is not None:
        MaskDetectors(sample_counts, MaskedWorkspace=comp_white_counts)
        white_count_mean = (comp_white_counts * white_counts)/(comp_white_counts + white_counts)

    # Make sure we have matrix workspaces before this division
    ConvertToMatrixWorkspace(white_count_mean, white_count_mean)
    ConvertToMatrixWorkspace(sample_counts, sample_counts)
        
    # Normalise the sample run
    sample_counts = Divide(sample_counts, white_count_mean, sample_counts).workspace()
    if comp_white_counts is not None:
        mtd.deleteWorkspace(str(white_count_mean))

    # If we need to remove zeroes as well then set the the low threshold to a tiny positive number
    if remove_zero:
        low_threshold = 1e-40
    else:
        low_threshold = -1.0

    # What shall we call the output
    lhs_names = lhs_info('names')
    if len(lhs_names) > 0:
        ws_name = lhs_names[0]
    else:
        ws_name = '__do_background_test'

    median_test = MedianDetectorTest(sample_counts, ws_name, SignificanceTest=signif,\
                                     LowThreshold=low_threshold, HighThreshold=bkgd_threshold)
    # Remove temporary
    mtd.deleteWorkspace(str(sample_counts))

    num_failed = median_test['NumberOfFailures'].value
    return median_test.workspace(), num_failed

#-------------------------------------------------------------------------------

def _do_bleed_test(sample_run, max_framerate, ignored_pixels):
    """Runs the CreatePSDBleedMask algorithm

    Input:
    sample_run  -  The run number of the sample
    max_framerate - The maximum allowed framerate in a tube. If None, the instrument defaults are used.
    ignored_pixels - The number of central pixels to ignore. If None, the instrument defaults are used.
    """
    mtd.sendLogMessage('Running PSD bleed test')
    # Load the sample run
    data_ws = common.load_run(sample_run)

    if max_framerate is None:
        max_framerate = float(data_ws.getInstrument().getNumberParameter('max-tube-framerate')[0])
    if ignored_pixels is None:
        ignored_pixels = int(data_ws.getInstrument().getNumberParameter('num-ignored-pixels')[0])
    
    # What shall we call the output
    lhs_names = lhs_info('names')
    if len(lhs_names) > 0:
        ws_name = lhs_names[0]
    else:
        ws_name = '__do_bleed__test'

    bleed_test = CreatePSDBleedMask(data_ws, ws_name, MaxTubeFramerate=max_framerate,\
                                    NIgnoredCentralPixels=ignored_pixels)

    num_failed = bleed_test['NumberOfFailures'].value
    return bleed_test.workspace(), num_failed

#-------------------------------------------------------------------------------

def _accumulate(lhs, rhs):
    """Essentially performs lhs += rhs for workspaces
    but checks if the lhs is None first and just assigns the workspace over
    """
    if lhs is None:
        lhs_names = lhs_info('names')
        if len(lhs_names) == 0: 
            raise RuntimeError('diagnostics._accumulate called without assignment of return value')
        lhs = RenameWorkspace(rhs, lhs_names[0]).workspace()
    else:
        lhs += rhs
        mtd.deleteWorkspace(str(rhs))                

    return lhs
    
#-------------------------------------------------------------------------------

def print_test_summary(test_results):
    """Print a summary of the failures per test run.

    Input:
    test_results - A list or tuple containing either the number of failed spectra or None
                   indicating that the test was not run
    """
    num_diags = 4
    if len(test_results) != num_diags:
        raise ValueError("Invalid input for print_test_summary. A list of %d numbers is expected." % num_diags)

    tests_run=False
    for failures in test_results:
        if failures is not None:
            tests_run = True

    if tests_run == False:
        print "No tests have been run!"
        return

    summary = (
        ['First white beam test:',test_results[0]], \
        ['Second white beam test:',test_results[1]], \
        ['Background test:',test_results[2]], \
        ['PSD Bleed test :',test_results[3]] \
        )

    print '==== Diagnostic Test Summary ===='

    max_name_length = -1
    max_ws_length = -1
    for key in range(num_diags):
        result = summary[key]
        name_length = len(str(result[0]))
        ws_length = len(str(result[1][0]))
        if name_length > max_name_length:
            max_name_length = name_length
        if ws_length > max_ws_length:
            max_ws_length = ws_length

    max_name_length += 2
    max_ws_length += 2
    for result in summary:
        test_name = str(result[0])
        workspace = str(result[1][0])
        nfailed = str(result[1][1])
        line = test_name + ' '*(max_name_length-len(test_name)) + \
               workspace + ' '*(max_ws_length-len(workspace)) + str(nfailed)
        print line
    # Append a new line
    print ''

#-------------------------------------------------------------------------------

def get_failed_spectra_list(diag_workspace):
    """Compile a list of spectra numbers that are marked as
    masked in the given workspace

    Input:

     diag_workspace  -  A workspace containing masking
    """
    if type(diag_workspace) == str:
        diag_workspace = mtd[diag_workspace]

    if hasattr(diag_workspace, "getAxis") == False:
        raise ValueError("Invalid input to get_failed_spectra_list. "
                         "A workspace handle or name is expected")
        
    spectra_axis = diag_workspace.getAxis(1)
    failed_spectra = []
    for i in range(diag_workspace.getNumberHistograms()):
	try:
            det = diag_workspace.getDetector(i)
	except RuntimeError:
            continue
	if det.isMasked():
            failed_spectra.append(spectra_axis.spectraNumber(i))

    return failed_spectra
