# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#pylind: disable=too-many-instance-attributes
#pylint: disable=too-many-branches
#pylint: disable=too-many-locals
"""
Provide functions to perform various diagnostic tests.

The output of each function is a workspace containing a single bin where:
    0 - denotes a masked spectra and
    1 - denotes an unmasked spectra.

This workspace can be summed with other masked workspaces to accumulate
masking and also passed to MaskDetectors to match masking there.
"""
from __future__ import (absolute_import, division, print_function)
from mantid.simpleapi import *
from mantid.kernel.funcinspect import lhs_info
import os
import Direct.RunDescriptor as RunDescriptor
from Direct.PropertyManager import PropertyManager
from six import iteritems
# Reference to reducer used if necessary for working with run descriptors (in diagnostics)
__Reducer__ = None


def diagnose(white_int,**kwargs):
    """
        Run diagnostics on the provided workspaces.

        Required inputs:

          white_int  - A workspace, run number or filepath of a white beam run. If a run/file is given it
                       simply loaded and integrated. A workspace is assumed to be prepared in it's integrated form

        Optional inputs:
          instrument_name - The name of the instrument (required for hard_masking)
          start_index    - The index to start the diag
          end_index    - The index to finish the diag
          background_int - A workspace, run number or filepath of a sample run that has been integrated over the background region.
                          If a run/file is given it simply loaded and integrated across the whole range
          sample_counts - A workspace containing the total integrated counts from a sample run
          second_white - If provided an additional set of tests is performed on this.
          hard_mask_file  - A file specifying those spectra that should be masked without testing
          tiny        - Minimum threshold for acceptance
          huge       - Maximum threshold for acceptance
          van_out_lo  - Lower bound defining outliers as fraction of median value
          van_out_hi  - Upper bound defining outliers as fraction of median value
          van_lo      - Fraction of median to consider counting low for the white beam diag
          van_hi      - Fraction of median to consider counting high for the white beam diag
          van_sig     - Error criterion as a multiple of error bar i.e. to fail the test, the magnitude of the
                        difference with respect to the median value must also exceed this number of error bars
          samp_zero    - If true then zeros in background are masked also
          samp_lo      - Fraction of median to consider counting low for the background  diag
          samp_hi      - Fraction of median to consider counting high for the background diag
          samp_sig     - Error criterion as a multiple of error bar i.e. to fail the test, the magnitude of the\n"
                      "difference with respect to the median value must also exceed this number of error bars
          variation  - The number of medians the ratio of the first/second white beam can deviate from
                       the average by
          bleed_test - If true then the CreatePSDBleedMask algorithm is run
          bleed_maxrate - If the bleed test is on then this is the maximum framerate allowed in a tube
          bleed_pixels - If the bleed test is on then this is the number of pixels ignored within the
                         bleed test diagnostic
          print_diag_results - If True then the results are printed to the screen
    """
    if white_int is None and str(white_int) != '':
        raise RuntimeError("No white beam integral specified. This is the minimum required to run diagnostics")

    # Grab the arguments
    parser = ArgumentParser(kwargs)
    start_index = parser.start_index
    end_index = parser.end_index

    # Map the test number to the results
    # Each element is the mask workspace name then the number of failures
    test_results = {}

    # Hard mask
    hardmask_file = kwargs.get('hard_mask_file', None)

    # process subsequent calls to this routine, when white mask is already defined
    white= kwargs.get('white_mask',None) # and white beam is not changed
    #white mask assumed to be global so no sectors in there
    if white is not None and isinstance(white,RunDescriptor.RunDescriptor):
        hardmask_file = None
        white_mask,num_failed = white.get_masking(2)
        add_masking(white_int, white_mask)
        van_mask  = None
    else: # prepare workspace to keep white mask
        white_mask = None
        van_mask = CloneWorkspace(white_int)

    if hardmask_file is not None:
        if parser.mapmask_ref_ws is None:
            ref_ws = white_int
        else:
            ref_ws = parser.mapmask_ref_ws

        hm_ws = LoadMask(Instrument=kwargs.get('instr_name',''),InputFile=parser.hard_mask_file,
                         OutputWorkspace='hard_mask_ws',RefWorkspace = ref_ws)
        MaskDetectors(Workspace=white_int, MaskedWorkspace=hm_ws)
        MaskDetectors(Workspace=van_mask, MaskedWorkspace=hm_ws)
        # Find out how many detectors we hard masked
        _dummy_ws,masked_list = ExtractMask(InputWorkspace=hm_ws)
        DeleteWorkspace('_dummy_ws')
        test_results['Hard mask:'] = [os.path.basename(parser.hard_mask_file),len(masked_list)]
        DeleteWorkspace('hard_mask_ws')

    if not parser.use_hard_mask_only :
        # White beam Test
        if white_mask:
            test_results['First detector vanadium test:'] = ['white_mask cache global', num_failed]
        else:
            __white_masks, num_failed = do_white_test(white_int, parser.tiny, parser.huge,
                                                      parser.van_out_lo, parser.van_out_hi,
                                                      parser.van_lo, parser.van_hi,
                                                      parser.van_sig, start_index, end_index)
            test_results['First detector vanadium test:'] = [str(__white_masks), num_failed]
            add_masking(white_int, __white_masks, start_index, end_index)
            if van_mask:
                add_masking(van_mask, __white_masks, start_index, end_index)
            DeleteWorkspace(__white_masks)

        # Second white beam test
        if 'second_white' in kwargs: #NOT IMPLEMENTED
            raise NotImplementedError("Second detector vanadium test")
            __second_white_masks, num_failed = do_second_white_test(white_int, parser.second_white, parser.tiny, parser.huge,
                                                                    parser.van_out_lo, parser.van_out_hi,
                                                                    parser.van_lo, parser.van_hi, parser.variation,
                                                                    parser.van_sig, start_index, end_index)
            test_results['Second detector vanadium test:'] = [str(__second_white_masks), num_failed]
            add_masking(white_int, __second_white_masks, start_index, end_index)
            #TODO
            #add_masking(van_mask, __second_white_masks, start_index, end_index)

        #
        # Zero total count check for sample counts
        #
        zero_count_failures = 0
        if kwargs.get('sample_counts',None) is not None and kwargs.get('samp_zero',False):
            add_masking(parser.sample_counts, white_int)
            maskZero, zero_count_failures = FindDetectorsOutsideLimits(InputWorkspace=parser.sample_counts,
                                                                       StartWorkspaceIndex=start_index, EndWorkspaceIndex=end_index,
                                                                       LowThreshold=1e-10, HighThreshold=1e100)
            test_results['Zero total count sample check:'] = [str(maskZero),zero_count_failures]
            add_masking(white_int, maskZero, start_index, end_index)
            DeleteWorkspace(maskZero)
        #
        # Background check
        #
        if hasattr(parser, 'background_int'):
            add_masking(parser.background_int, white_int)
            __bkgd_mask, failures = do_background_test(parser.background_int, parser.samp_lo,
                                                       parser.samp_hi, parser.samp_sig, parser.samp_zero, start_index, end_index)
            test_results['Background test:'] = [str(__bkgd_mask), zero_count_failures + failures]
            add_masking(white_int, __bkgd_mask, start_index, end_index)
            DeleteWorkspace(__bkgd_mask)

        #
        # Bleed test
        # (bleed test in multirep mode calculated per TOF region)
        if hasattr(parser, 'bleed_test') and parser.bleed_test and not PropertyManager.incident_energy.multirep_mode():
            if not hasattr(parser, 'sample_run'):
                raise RuntimeError("Bleed test requested but the sample_run keyword has not been provided")
            __bleed_masks, failures = do_bleed_test(parser.sample_run, parser.bleed_maxrate, parser.bleed_pixels)
            test_results['PSD Bleed test:'] = [str(__bleed_masks), failures]
            add_masking(white_int, __bleed_masks)
            DeleteWorkspace(__bleed_masks)
    # endif not hard_mask_only
    start_index_name = "from: start"
    end_index_name=" to: end"
    default = True
    if hasattr(parser, 'print_diag_results') and parser.print_diag_results:
        default=True
    if 'start_index' in kwargs:
        default = False
        start_index_name = "from: "+str(kwargs['start_index'])
    if 'end_index' in kwargs :
        default = False
        end_index_name = " to: "+str(kwargs['end_index'])

    testName=start_index_name+end_index_name
    if not default :
        testName = " For bank: "+start_index_name+end_index_name

    if hasattr(parser, 'print_diag_results') and parser.print_diag_results:
        print_test_summary(test_results,testName)
    return van_mask

#-------------------------------------------------------------------------------


def add_masking(input_ws, mask_ws, start_index=None, end_index=None):
    """
    Mask the Detectors on the input workspace that are masked
    on the mask_ws.
    """
    MaskDetectors(Workspace=input_ws, MaskedWorkspace=mask_ws,
                  StartWorkspaceIndex=start_index, EndWorkspaceIndex=end_index)

#-------------------------------------------------------------------------------


def do_white_test(white_int, tiny, large, out_lo, out_hi, median_lo, median_hi, sigma,
                  start_index=None, end_index=None):
    """
    Run the diagnostic tests on the integrated white beam run

    Required inputs:

      white_int - An integrated workspace
      tiny      - Minimum threshold for acceptance
      large     - Maximum threshold for acceptance
      out_lo    - Lower bound defining outliers as fraction of median value (default = 0.01)
      out_hi    - Upper bound defining outliers as fraction of median value (default = 100.)
      median_lo - Fraction of median to consider counting low
      median_hi - Fraction of median to consider counting high
      sigma     - Error criterion as a multiple of error bar

    """
    logger.notice('Running first white beam test')

    # Make sure we are a MatrixWorkspace
    white_int = ConvertToMatrixWorkspace(InputWorkspace=white_int,OutputWorkspace=white_int)
    # The output workspace will have the failed detectors masked
    white_masks,num_failed = FindDetectorsOutsideLimits(white_int, StartWorkspaceIndex=start_index,
                                                        EndWorkspaceIndex=end_index,
                                                        HighThreshold=large, LowThreshold=tiny)

    MaskDetectors(Workspace=white_int, MaskedWorkspace=white_masks,
                  StartWorkspaceIndex=start_index, EndWorkspaceIndex=end_index)
    DeleteWorkspace(Workspace=white_masks)

    white_masks,failed_median = MedianDetectorTest(white_int, StartWorkspaceIndex=start_index,
                                                   EndWorkspaceIndex=end_index, SignificanceTest=sigma,
                                                   LowThreshold=median_lo, HighThreshold=median_hi,
                                                   LowOutlier=out_lo, HighOutlier=out_hi, ExcludeZeroesFromMedian=False)
    MaskDetectors(Workspace=white_int, MaskedWorkspace=white_masks,
                  StartWorkspaceIndex=start_index, EndWorkspaceIndex=end_index)
    num_failed += failed_median
    return white_masks, num_failed

#-------------------------------------------------------------------------------


def do_second_white_test(white_counts, comp_white_counts, tiny, large, out_lo, out_hi,
                         median_lo, median_hi, sigma, variation,
                         start_index=None, end_index=None):
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
    logger.notice('Running second white beam test')

    # What shall we call the output
    lhs_names = lhs_info('names')
    if len(lhs_names) > 0:
        ws_name = lhs_names[0]
    else:
        ws_name = '__do_second_white_test'

    # Make sure we are a MatrixWorkspace
    white_counts = ConvertToMatrixWorkspace(InputWorkspace=white_counts,OutputWorkspace=white_counts)
    comp_white_counts = ConvertToMatrixWorkspace(InputWorkspace=comp_white_counts,OutputWorkspace=comp_white_counts)

    # Do the white beam test
    __second_white_tests, failed = do_white_test(comp_white_counts, tiny, large, median_lo, median_hi,
                                                 sigma, start_index, end_index)
    # and now compare it with the first
    effic_var, num_failed = DetectorEfficiencyVariation(WhiteBeamBase=white_counts, WhiteBeamCompare=comp_white_counts,
                                                        OutputWorkspace=ws_name,
                                                        Variation=variation, StartWorkspaceIndex=start_index,
                                                        EndWorkspaceIndex=end_index)

    DeleteWorkspace(Workspace=str(__second_white_tests))
    # Mask those that failed
    maskWS = effic_var
    MaskDetectors(Workspace=white_counts, MaskedWorkspace=maskWS)
    MaskDetectors(Workspace=comp_white_counts, MaskedWorkspace=maskWS)

    return maskWS, num_failed

#------------------------------------------------------------------------------


def normalise_background(background_int, white_int, second_white_int=None):
    """Normalize the background integrals

       If two white beam files are provided then the background integrals
       are normalized by the harmonic mean of the two:

       hmean = 2.0/((1/v1) + (1/v2)) = 2v1*v2/(v1+v2)

       If only a single white
       beam is provided then the background is normalized by the white beam itself

    """
    if second_white_int is None:
        #quietly divide background integral by white beam integral not reporting about possible 0 in
        #wb integral (they will be removed by diag anyway)
        background_int =  Divide(LHSWorkspace=background_int,RHSWorkspace=white_int,WarnOnZeroDivide='0')
    else:
        hmean = 2.0*white_int*second_white_int/(white_int+second_white_int)
        #background_int /= hmean
        background_int =  Divide(LHSWorkspace=background_int,RHSWorkspace=hmean,WarnOnZeroDivide='0')
        DeleteWorkspace(hmean)

#------------------------------------------------------------------------------


def do_background_test(background_int, median_lo, median_hi, sigma, mask_zero,
                       start_index=None, end_index=None):
    """
    Run the background tests

    Required inputs:
      background_int - An integrated workspace
      median_lo - Fraction of median to consider counting low
      median_hi - Fraction of median to consider counting high
      sigma     - Error criterion as a multiple of error bar
      mask_zero - If True, zero background counts will be considered a fail

    """
    logger.notice('Running background count test')

    mask_bkgd, num_failures = MedianDetectorTest(InputWorkspace=background_int,
                                                 StartWorkspaceIndex=start_index, EndWorkspaceIndex=end_index,
                                                 SignificanceTest=sigma,
                                                 LowThreshold=median_lo, HighThreshold=median_hi,
                                                 LowOutlier=0.0, HighOutlier=1e100, ExcludeZeroesFromMedian=True)
    #TODO: Looks like hack! why it returns negative value
    return mask_bkgd, abs(num_failures)

#-------------------------------------------------------------------------------


def do_bleed_test(sample_run, max_framerate, ignored_pixels):
    """Runs the CreatePSDBleedMask algorithm

    Input:
    sample_run  -  The run number of the sample
    max_framerate - The maximum allowed framerate in a tube. If None, the instrument defaults are used.
    ignored_pixels - The number of central pixels to ignore. If None, the instrument defaults are used.
    """
    #NOTE: Should be deployed on non-normalized workspace only!
    logger.notice('Running PSD bleed test')
    # Load the sample run
    if __Reducer__: #  Try to use generic loader which would work with files or workspaces alike
        sample_run = __Reducer__.get_run_descriptor(sample_run)
        data_ws    = sample_run.get_workspace() # this will load data if necessary
        ws_name    = data_ws.name()+'_bleed'
    else:
        # may be sample run is already a run descriptor despite __Reducer__ have not been exposed
        data_ws    = sample_run.get_workspace() # this will load data if necessary
        ws_name    = data_ws.name()+'_bleed'

    if max_framerate is None: #get defaults
        max_framerate = float(data_ws.getInstrument().getNumberParameter('max-tube-framerate')[0])
    if ignored_pixels is None: #get defaults
        ignored_pixels = int(data_ws.getInstrument().getNumberParameter('num-ignored-pixels')[0])
    else:
        # Make sure it is an int
        ignored_pixels = int(ignored_pixels)

    # What shall we call the output
    lhs_names = lhs_info('names')
    if len(lhs_names) > 0:
        ws_name = lhs_names[0]
    else:
        ws_name = '__do_bleed__test'
    # Check if all necessary logs present in the workspace,as nxs workspace log names are different
    #  from a raw file workspace logs.
    try:
        nFrames= data_ws.getRun().getLogData('goodfrm').value
    except RuntimeError:
        try:
            nFrames = data_ws.getRun().getLogData('good_frames').lastValue()
            AddSampleLog(Workspace=data_ws, LogName='goodfrm', LogText=str(int(nFrames)), LogType='Number')
        except RuntimeError:
            raise RuntimeError("Bleed test fails as no appropriate 'good_frames' or 'goodfrm' log is loaded with ws: {0}\n"
                               "Disable bleed test by setting diag_bleed_test=False or add 'goodfrm' log to the workspace\n"
                               .format(data_ws.name()))

    bleed_test, num_failed = CreatePSDBleedMask(InputWorkspace=data_ws, OutputWorkspace=ws_name,
                                                MaxTubeFramerate=max_framerate,
                                                NIgnoredCentralPixels=ignored_pixels)
    return bleed_test, num_failed

#-------------------------------------------------------------------------------


def print_test_summary(test_results,test_name=None):
    """Print a summary of the failures per test run.

    Input:
    test_results - A list or tuple containing either the number of failed spectra or None
                   indicating that the test was not run
    IMPORTANT: The output of this function is used as
               input for GUI, so the keys names, = sign and : are control
               symbols of MantidWidgets->DiagResults method.
    """

    if len(test_results) == 0:
        print("No tests have been run!")
        return

    if test_name is None:
        print('======== Diagnostic Test Summary ')
    else:
        print('======== Diagnostic Test Summary {0} '.format(test_name))

    max_test_len = 0
    max_ws_len = 0
    for t_name in test_results:
        if len(t_name)>max_test_len:
            max_test_len = len(t_name)
        t_result = test_results[t_name]
        if len(t_result[0])>max_ws_len :
            max_ws_len = len(t_result[0])
    format_string = " {{0:<{0}}}  {{1:<{1}}}  {{2:>10}}".format(max_test_len,max_ws_len)

    for t_name in test_results:
        t_result = test_results[t_name]
        print(format_string.format(t_name,t_result[0],t_result[1]))
    # Append a new line
    print('================================================================')
    print('')


#-------------------------------------------------------------------------------

def get_failed_spectra_list(diag_workspace):
    """Compile a list of spectra numbers that are marked as
    masked in the given workspace

    Input:

     diag_workspace  -  A workspace containing masking
    """
    if isinstance(diag_workspace, str):
        diag_workspace = mtd[diag_workspace]

    failed_spectra = []
    spectrumInfo = diag_workspace.spectrumInfo()
    for i in range(diag_workspace.getNumberHistograms()):
        if spectrumInfo.hasDetectors(i):
            if spectrumInfo.isMasked(i):
                failed_spectra.append(diag_workspace.getSpectrum(i).getSpectrumNo())

    return failed_spectra

#------------------------------------------------------------------------------


class ArgumentParser(object):

    def __init__(self, keywords):
        self.start_index = None # Make this more general for anything that is missing!
        self.end_index = None
        for key, value in iteritems(keywords):
            setattr(self, key, value)
