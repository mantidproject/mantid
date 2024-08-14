# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from typing import Union
import matplotlib.pyplot as plt
from matplotlib.patches import Circle
from matplotlib.collections import PatchCollection
import matplotlib.offsetbox as pltbox
import numpy as np

from mantid.api import WorkspaceFactory
from mantid.dataobjects import TableWorkspace, Workspace2D
from mantid.plots.datafunctions import get_axes_labels
from mantid.plots.resampling_image.samplingimage import imshow_sampling
from mantid.plots.utility import colormap_as_plot_color
from mantid.simpleapi import CalculateDIFC, LoadDiffCal, mtd, LoadEmptyInstrument, SolidAngle


# Diamond peak positions in d-space which may differ from actual sample
DIAMOND = np.asarray(
    (
        2.06,
        1.2615,
        1.0758,
        0.892,
        0.8186,
        0.7283,
        0.6867,
        0.6307,
        0.5642,
        0.5441,
        0.515,
        0.4996,
        0.4768,
        0.4645,
        0.4205,
        0.3916,
        0.3499,
        0.3257,
        0.3117,
    )
)


def _get_xrange(wksp, xmarkers, tolerance, xmin, xmax):
    # start with the range determined by the markers and tolerance
    x_mins = [xmarkers.min() * (1.0 - 3 * tolerance)]
    x_maxs = [xmarkers.max() * (1.0 + 3 * tolerance)]
    # add range based on user supplied parameters
    if not np.isnan(xmin):
        x_mins.append(xmin)
    if not np.isnan(xmax):
        x_maxs.append(xmax)
    # add data range if possible
    try:
        temp = wksp.getTofMin()  # only a method on EventWorkspace
        if not np.isnan(temp):
            x_mins.append(temp)
        temp = wksp.getTofMax()
        if not np.isnan(temp):
            x_maxs.append(temp)
    except:
        pass  # don't use the data range
    xmin = np.max(x_mins)
    xmax = np.min(x_maxs)

    return xmin, xmax


def _get_difc_ws(wksp, instr_ws=None):
    if wksp is None:
        return None
    # Check if given a workspace
    ws_str = str(wksp)
    difc_ws = None
    if not mtd.doesExist(ws_str):
        # Check if it was a file instead
        if ws_str.endswith(tuple([".h5", ".hd5", ".hdf", ".cal"])):
            try:
                LoadDiffCal(Filename=ws_str, WorkspaceName="__cal_{}".format(ws_str))
                difc_ws = CalculateDIFC(
                    InputWorkspace="__cal_{}_group".format(ws_str),
                    CalibrationWorkspace="__cal_{}_cal".format(ws_str),
                    OutputWorkspace="__difc_{}".format(ws_str),
                )
            except:
                raise RuntimeError("Could not load calibration file {}".format(ws_str))
        else:
            raise RuntimeError("Could not find workspace {} in ADS and it was not a file".format(ws_str))
    else:
        # If workspace exists, check if it is a SpecialWorkspace2D (result from CalculateDIFC)
        if mtd[ws_str].id() == "SpecialWorkspace2D":
            difc_ws = mtd[ws_str]
        elif mtd[ws_str].id() == "TableWorkspace":
            if not mtd.doesExist(str(instr_ws)):
                raise RuntimeError("Expected instrument workspace instr_ws to use with calibration tables")
            # Check if the workspace looks like a calibration workspace
            col_names = mtd[ws_str].getColumnNames()
            # Only need the first two columns for the CalculateDIFC algorithm to work
            if len(col_names) >= 2 and col_names[0] == "detid" and col_names[1] == "difc":
                # Calculate DIFC on this workspace
                difc_ws = CalculateDIFC(
                    InputWorkspace=mtd[str(instr_ws)], CalibrationWorkspace=mtd[ws_str], OutputWorkspace="__difc_{}".format(ws_str)
                )
        else:
            raise TypeError("Wrong workspace type. Expects SpecialWorkspace2D, TableWorkspace, or a filename")
    return difc_ws


def __get_regions(x: np.ndarray):
    """
    Find the beginning and end of all sequences in x
    Ex: [1, 2, 3, 5, 6, 8, 9] will return [(1, 3), (5, 6), (8, 9)]
    :param x: Array of detector IDs (in ascending order)
    :return: List of tuples with start,stop indices to mark detector regions
    """
    regions = []
    # Compute a +/- 1 shift in indices and compare them to find gaps in the sequence
    lower = np.asarray((x + 1)[:-1], dtype=int)
    upper = np.asarray((x - 1)[1:], dtype=int)
    gaps = lower <= upper
    lower, upper = lower[gaps], upper[gaps]
    i = 0
    # Use the start and stop of each gap to find sequential regions instead
    for start, stop in zip(lower, upper):
        if i == 0:
            # Starting edge case - the start of x until the start of the first gap
            regions.append((int(np.min(x)), int(start - 1)))
        else:
            # Mark the region between the end of the previous gap and the start of the next gap
            regions.append((int(upper[i - 1] + 1), int(start - 1)))
        i = i + 1
    # Add ending region if there was at least one region found:
    if i > 0:
        # Ending edge case: the end of the last gap until the end of the input
        regions.append((int(upper[i - 1] + 1), int(np.max(x))))
    return regions


def __get_bad_counts(y, mean=None, band=0.01):
    # Counts pixels in y that are outside of +/- mean*band
    if not mean:
        mean = np.mean(y)
    top = mean + mean * band
    bot = mean - mean * band
    return len(y[(y > top) | (y < bot)])


def get_peakwindows(peakpositions: np.ndarray):
    """
    Calculates the peak windows for the given peak positions used for FitPeaks
    :param peakpositions: List of peak positions in d-space
    :return: List of peak windows (left and right) for each peak position
    """
    peakwindows = []
    deltas = 0.5 * (peakpositions[1:] - peakpositions[:-1])
    # first left and right
    peakwindows.append(peakpositions[0] - deltas[0])
    peakwindows.append(peakpositions[0] + deltas[0])
    # ones in the middle
    for i in range(1, len(peakpositions) - 1):
        peakwindows.append(peakpositions[i] - deltas[i - 1])
        peakwindows.append(peakpositions[i] + deltas[i])
    # last one
    peakwindows.append(peakpositions[-1] - deltas[-1])
    peakwindows.append(peakpositions[-1] + deltas[-1])
    return peakwindows


def plot2d(
    workspace, tolerance: float = 0.001, peakpositions: np.ndarray = DIAMOND, xmin: float = np.nan, xmax: float = np.nan, horiz_markers=[]
):
    TOLERANCE_COLOR = "w"  # color to mark the area within tolerance
    MARKER_COLOR = "b"  # color for peak markers and horizontal (between bank) markers

    # convert workspace to pointer to workspace
    wksp = mtd[str(workspace)]

    # determine x-range
    xmin, xmax = _get_xrange(wksp, peakpositions, tolerance, xmin, xmax)

    # create a new figure
    fig, ax = plt.subplots()
    # show the image - this uses the same function used by "colorfill" plot
    # and slice viewer. The data is subsampled according to how may screen
    # pixels are available
    im = imshow_sampling(ax, wksp, aspect="auto", origin="lower")
    # add the colorbar
    fig.colorbar(im, ax=ax)
    # set the x-range to show
    ax.set_xlim(xmin, xmax)

    # annotate expected peak positions and tolerances
    for peak in peakpositions:
        if peak > xmin and peak < xmax:
            shade = ((1 - tolerance) * peak, (1 + tolerance) * peak)
            ax.axvspan(shade[0], shade[1], color=TOLERANCE_COLOR, alpha=0.2)
            ax.axvline(x=peak, color=MARKER_COLOR, alpha=0.4)

    # annotate areas of the detector
    for position in horiz_markers:
        ax.axhline(position, color=MARKER_COLOR, alpha=0.4)

    # add axes labels
    labels = get_axes_labels(wksp)
    ax.set_xlabel(labels[1])
    ax.set_ylabel(labels[2])

    # show the figure
    fig.show()

    # return the figure so others can customize it
    return fig, fig.axes


def difc_plot2d(calib_new, calib_old=None, instr_ws=None, mask=None, vrange=(0, 1)):
    """
    Plots the percent change in DIFC between calib_new and calib_old
    :param calib_new: New calibration, can be filename, SpecialWorkspace2D, or calibration table (TableWorkspace)
    :param calib_old: Optional old calibration, can be same as types as calib_new; if not specified, default instr used
    :param instr_ws: Workspace used for instrument definition, only needed if calib_new and/or calib_old are tables
    :param mask: MaskWorkspace used to hide detector pixels from plot
    :param vrange: Tuple of (min,max) used for the plot color bar
    :return: figure and figure axes
    """

    ws_new = _get_difc_ws(calib_new, instr_ws)
    if ws_new is None:
        raise TypeError("Expected to receive a workspace or filename, got None.")

    ws_old = _get_difc_ws(calib_old, instr_ws)
    if ws_old is None:
        # If no second workspace is given, then load default instrument to compare against
        instr_name = ws_new.getInstrument().getName()
        ws_old = CalculateDIFC(InputWorkspace=ws_new, OutputWorkspace="__difc_{}".format(instr_name))

    delta = ws_new - ws_old

    use_mask = False
    if mask is not None and mtd.doesExist(str(mask)):
        use_mask = True

    # Plotting below taken from addie/calibration/CalibrationDiagnostics.py
    theta_array = []
    phi_array = []
    value_array = []
    masked_theta_array = []
    masked_phi_array = []
    info = delta.spectrumInfo()
    for det_id in range(info.size()):
        pos = info.position(det_id)
        phi = np.arctan2(pos[0], pos[1])
        theta = np.arccos(pos[2] / pos.norm())
        if use_mask and mtd[str(mask)].dataY(det_id):
            masked_theta_array.append(theta)
            masked_phi_array.append(phi)
        else:
            theta_array.append(theta)
            phi_array.append(phi)
            percent = 100.0 * np.sum(np.abs(delta.dataY(det_id))) / np.sum(ws_old.dataY(det_id))
            value_array.append(percent)

    # Use the largest solid angle for circle radius
    maximum_solid_angle = 0.0
    if instr_ws is None:
        LoadEmptyInstrument(InstrumentName=instr_name, OutputWorkspace=f"{instr_name}_instr")
        instr_ws = f"{instr_name}_instr"
    det_tmp = SolidAngle(InputWorkspace=instr_ws, OutputWorkspace="detector_tmp")
    for wksp_index in range(info.size()):
        maximum_solid_angle = max(maximum_solid_angle, det_tmp.readY(wksp_index)[0])

    # Convert to degrees for plotting
    theta_array = np.rad2deg(theta_array)
    phi_array = np.rad2deg(phi_array)
    maximum_solid_angle = np.rad2deg(maximum_solid_angle)
    if use_mask:
        masked_phi_array = np.rad2deg(masked_phi_array)
        masked_theta_array = np.rad2deg(masked_theta_array)

    # Radius also includes a fudge factor to improve plotting.
    # May need to add finer adjustments on a per-instrument basis.
    # Small circles seem to alias less than rectangles.
    radius = maximum_solid_angle * 8.0
    patches = []
    for x1, y1 in zip(theta_array, phi_array):
        circle = Circle((x1, y1), radius)
        patches.append(circle)

    masked_patches = []
    for x1, y1 in zip(masked_theta_array, masked_phi_array):
        circle = Circle((x1, y1), radius)
        masked_patches.append(circle)

    # Matplotlib requires this to be a Numpy array.
    colors = np.array(value_array)
    p = PatchCollection(patches)
    p.set_array(colors)
    p.set_clim(vrange[0], vrange[1])
    p.set_edgecolor("face")

    fig, ax = plt.subplots()
    ax.add_collection(p)
    mp = PatchCollection(masked_patches)
    mp.set_facecolor("gray")
    mp.set_edgecolor("face")
    ax.add_collection(mp)
    cb = fig.colorbar(p, ax=ax)
    cb.set_label("delta DIFC (%)")
    ax.set_xlabel(r"in-plane polar (deg)")
    ax.set_ylabel(r"azimuthal (deg)")

    # Find bounds based on detector pos
    xmin = np.min(theta_array)
    xmax = np.max(theta_array)
    ymin = np.min(phi_array)
    ymax = np.max(phi_array)
    ax.set_xlim(np.floor(xmin), np.ceil(xmax))
    ax.set_ylim(np.floor(ymin), np.ceil(ymax))

    fig.show()

    return fig, ax


def __calculate_strain(obs: dict, exp: np.ndarray):
    """Computes the fraction of observed d-spacing values to expected (diamond) peak positions"""
    return __calculate_dspacing(obs) / exp


def __calculate_difference(obs: dict, exp: np.ndarray):
    """Calculate the difference between d-spacing values and expected d-spacing values"""
    return np.abs(__calculate_dspacing(obs) - exp) / exp


def __calculate_dspacing(obs: dict):
    """Extract the dspacing values from a row in output dspace table workspace from PDCalibration"""
    # Trim the table row since this is from the output of PDCalibration, so remove detid, chisq and normchisq columns
    return np.asarray(list(obs.values())[1:-2])


def __create_outputws(donor: Union[str, Workspace2D], numSpec, numPeaks):
    """The resulting workspace needs to be added to the ADS"""
    # convert the d-space table to a Workspace2d
    if donor:
        donor = mtd[str(donor)]
    else:
        donor = "Workspace2D"
    output = WorkspaceFactory.create(donor, NVectors=numSpec, XLength=numPeaks, YLength=numPeaks)
    output.getAxis(0).setUnit("dSpacing")
    return output


def collect_peaks(wksp: Union[str, TableWorkspace], outputname: str, donor: Union[str, Workspace2D] = None, infotype: str = "strain"):
    """
    Calculate different types of information for each peak position in wksp and create a new Workspace2D
    where each column is a peak position and each row is the info for each workspace index in donor
    :param wksp: Input TableWorkspace (i.e, dspacing table from PDCalibration)
    :param outputname: Name of the Workspace2D to create with the information
    :param donor: Name of the donor Workspace2D, used to create the shape of the output
    :param infotype: 'strain', 'difference', or 'dpsacing' to specify which kind of data to output
    :return: Created Workspace2D with fractional difference, relative difference, or dspacing values
    """
    if infotype not in ["strain", "difference", "dspacing"]:
        raise ValueError('Do not know how to calculate "{}"'.format(infotype))

    wksp = mtd[str(wksp)]

    numSpec = int(wksp.rowCount())
    peak_names = [item for item in wksp.getColumnNames() if item not in ["detid", "chisq", "normchisq"]]
    peaks = np.asarray([float(item[1:]) for item in peak_names])
    numPeaks = len(peaks)

    # convert the d-space table to a Workspace2d
    output = __create_outputws(donor, numSpec, numPeaks)
    for i in range(numSpec):  # TODO get the detID correct
        output.setX(i, peaks)
        if infotype == "strain":
            output.setY(i, __calculate_strain(wksp.row(i), peaks))
        elif infotype == "difference":
            output.setY(i, __calculate_difference(wksp.row(i), peaks))
        elif infotype == "dspacing":
            output.setY(i, __calculate_dspacing(wksp.row(i)))
        else:
            raise ValueError(f"Do not know how to calculate {infotype}")

    # add the workspace to the AnalysisDataService
    mtd.addOrReplace(outputname, output)
    return mtd[outputname]


def collect_fit_result(
    wksp: Union[str, TableWorkspace],
    outputname: str,
    peaks,
    donor: Union[str, Workspace2D] = None,
    infotype: str = "centre",
    chisq_max: float = 1.0e4,
):
    """
    Extracts different information about fit results from a TableWorkspace and places into a Workspace2D
    This assumes that the input is sorted by wsindex then peakindex
    :param wksp: Input TableWorkspace from PDCalibration or FitPeaks (should have infotype as a column)
    :param outputname: Name of output Workspace2D created
    :param peaks: Array of peak positions
    :param donor: Optional Workspace2D to use to determine output size
    :param infotype: Type of fit information to extract ("centre", "width", "height", or "intensity")
    :param chisq_max: Max chisq value that should be included in output, data gets set to nan if above this
    :return: Created Workspace2D with infotype extracted from wksp
    """
    KNOWN_COLUMNS = ["centre", "width", "height", "intensity"]
    if infotype not in KNOWN_COLUMNS:
        raise ValueError(f'Do not know how to extract "{infotype}"')

    wksp = mtd[str(wksp)]
    for name in KNOWN_COLUMNS + ["wsindex", "peakindex", "chi2"]:
        if name not in wksp.getColumnNames():
            raise RuntimeError('did not find column "{}" in workspace "{}"'.format(name, str(wksp)))

    wsindex = np.asarray(wksp.column("wsindex"))
    wsindex_unique = np.unique(wsindex)
    chi2 = np.asarray(wksp.column("chi2"))
    observable = np.asarray(wksp.column(infotype))
    # set values to nan where the chisq is too high
    observable[chi2 > chisq_max] = np.nan

    # convert the numpy arrays to a Workspace2d
    numPeaks = len(peaks)
    if donor:
        numSpec = mtd[str(donor)].getNumberHistograms()
    else:
        numSpec = len(wsindex_unique)
    output = __create_outputws(donor, numSpec, numPeaks)
    for i in range(len(wsindex_unique)):
        start = int(np.searchsorted(wsindex, wsindex_unique[i]))
        i = int(i)  # to be compliant with mantid API
        output.setX(i, peaks)
        output.setY(i, observable[start : start + numPeaks])
    mtd.addOrReplace(outputname, output)
    return mtd[outputname]


def extract_peak_info(wksp: Union[str, Workspace2D], outputname: str, peak_position: float):
    """
    Extract information about a single peak from a Workspace2D. The input workspace is expected to have
    common x-axis of observed d-spacing. The y-values and errors are extracted.

    The output workspace will be a single spectra with the x-axis being the detector-id. The y-values
    and errors are extracted from the input workspace.
    """
    # confirm that the input is a workspace pointer
    wksp = mtd[str(wksp)]
    numSpec = wksp.getNumberHistograms()

    # get the index into the x/y arrays of the peak position
    peak_index = wksp.readX(0).searchsorted(peak_position)

    # create a workspace to put the result into
    single = WorkspaceFactory.create("Workspace2D", NVectors=1, XLength=wksp.getNumberHistograms(), YLength=wksp.getNumberHistograms())
    single.setTitle("d-spacing={}\\A".format(wksp.readX(0)[peak_index]))

    # get a handle to map the detector positions
    detids = wksp.detectorInfo().detectorIDs()
    have_detids = bool(len(detids) > 0)

    # fill in the data values
    x = single.dataX(0)
    y = single.dataY(0)
    e = single.dataE(0)
    start_detid = np.searchsorted(detids, 0)
    for wksp_index in range(numSpec):
        if have_detids:
            x[wksp_index] = detids[start_detid + wksp_index]
        else:
            x[wksp_index] = wksp_index
        y[wksp_index] = wksp.readY(wksp_index)[peak_index]
        e[wksp_index] = wksp.readE(wksp_index)[peak_index]

    # add the workspace to the AnalysisDataService
    mtd.addOrReplace(outputname, single)
    return mtd[outputname]


# flake8: noqa: C901
def plot_peakd(
    wksp: Union[str, Workspace2D], peak_positions: Union[float, list], plot_regions=True, show_bad_cnt=True, drange=(0, 0), threshold=0.01
):
    """
    Plots peak d spacing value for each peak position in peaks
    :param wksp: Workspace returned from collect_peaks
    :param peak_positions: List of peak positions, or single peak position
    :param plot_regions: Whether to show vertical lines indicating detector regions
    :param show_bad_cnt: Whether to display number of pixels that fall outside of threshold from mean
    :param drange: A tuple of the minimum and maximum detector ID to plot, plots full range if (0,0) or out of bounds
    :param threshold: The fraction of the mean to display horizontal bars at +/- the mean
    :return: plot, plot axes
    """

    ylabel = "rel strain"

    peaks = peak_positions
    if isinstance(peak_positions, float):
        peaks = [peak_positions]
        ylabel += " (peak {})".format(peaks[0])

    if len(peaks) == 0:
        raise ValueError("Expected one or more peak positions")

    if not mtd.doesExist(str(wksp)):
        raise ValueError("Could not find provided workspace in ADS")

    fig, ax = plt.subplots()
    ax.set_xlabel("detector IDs")
    ax.set_ylabel(ylabel)

    # Hold the mean and stddev for each peak to compute total at end
    means = []
    stddevs = []
    lens = []

    ax.set_prop_cycle(color=colormap_as_plot_color(len(peaks), cmap=plt.get_cmap("jet")))

    regions = []
    plotted_peaks = []

    # Use full detector range if not specified
    max_detid = int(np.max(mtd[str(wksp)].detectorInfo().detectorIDs()))
    min_detid = int(np.min([item for item in mtd[str(wksp)].detectorInfo().detectorIDs() if item > 0]))
    if drange == (0, 0) or drange > (min_detid, max_detid):
        drange = (min_detid, max_detid)
        print("Specified drange out of bounds, using {}".format(drange))

    # Plot data for each peak position
    for peak in peaks:
        print("Processing peak position {}".format(peak))
        single = extract_peak_info(wksp, "single", peak)

        # get x and y arrays from single peak ws
        x = single.dataX(0)
        y = single.dataY(0)

        # filter out any nans
        y_val = y[~np.isnan(y)]

        try:
            dstart = single.yIndexOfX(drange[0])
        except ValueError:
            # If detector id not valid, find next closest range
            dstart = single.yIndexOfX(int(x.searchsorted(drange[0])) - 1)
            print("Specified starting detector range was not valid, adjusted to detID={}".format(dstart))
        try:
            dend = single.yIndexOfX(drange[1])
        except ValueError:
            dend = single.yIndexOfX(int(x.searchsorted(drange[1])))
            print("Specified ending detector range was not valid, adjusted to detID={}".format(dend))

        if dstart >= dend:
            raise RuntimeError("Detector start region ({}) must be less than the end region ({})".format(dstart, dend))
        cut_id = (dstart, dend)

        # skip if y was entirely nans or detector slice yields empty region
        if len(y_val) == 0 or len(y_val[cut_id[0] : cut_id[1]]) == 0:
            print("No valid y-data was found for peak {}, so it will be skipped".format(peak))
            continue

        lens.append(len(y_val[cut_id[0] : cut_id[1]]))
        means.append(np.mean(y_val[cut_id[0] : cut_id[1]]))
        stddevs.append(np.std(y_val[cut_id[0] : cut_id[1]]))

        # Draw vertical lines between detector regions
        if not regions:
            regions = __get_regions(x[cut_id[0] : cut_id[1]])
            if plot_regions:
                for region in regions:
                    ax.axvline(x=region[0], lw=0.5, color="black")
                    ax.axvline(x=region[1], lw=0.5, color="black", ls="--")
        plotted_peaks.append(peak)

        ax.plot(x[cut_id[0] : cut_id[1]], y[cut_id[0] : cut_id[1]], marker="x", linestyle="None", label="{:0.6f}".format(peak))
        ax.legend(bbox_to_anchor=(1, 1), loc="upper left")

    # If every peak had nans, raise error
    if len(means) == 0 or len(stddevs) == 0:
        raise RuntimeError("No valid peak data was found for provided peak positions")

    # Calculate total mean and stddev of all peaks
    total_mean = np.sum(np.asarray(means) * np.asarray(lens)) / np.sum(lens)
    sq_sum = np.sum(lens * (np.power(stddevs, 2) + np.power(means, 2)))
    total_stddev = np.sqrt((sq_sum / np.sum(lens)) - total_mean**2)

    # Get bad counts in regions over all peaks using the total mean
    if show_bad_cnt and regions:
        region_cnts = [0] * len(regions)
        for peak in plotted_peaks:
            single = extract_peak_info(wksp, "single", peak)
            y = single.dataY(0)
            for i in range(len(regions)):
                det_start = single.yIndexOfX(regions[i][0])
                det_end = single.yIndexOfX(regions[i][1])
                region_cnts[i] += __get_bad_counts(y[det_start:det_end], mean=total_mean, band=threshold)

        # Display the bad counts for each region, or the overall count
        if plot_regions:
            for i in range(len(regions)):
                mid_region = regions[i][0] + 0.5 * (regions[i][1] - regions[i][0])
                ax.annotate(
                    "{}".format(region_cnts[i]), xy=(mid_region, 0.05), xycoords=("data", "axes fraction"), clip_on=True, ha="center"
                )
        else:
            overall_cnt = int(np.sum(region_cnts))
            ax.annotate("{}".format(overall_cnt), xy=(0.5, 0.05), xycoords=("axes fraction", "axes fraction"), clip_on=True, ha="center")
    # Draw solid line at mean
    ax.axhline(total_mean, color="black", lw=2.5)  # default lw=1.5

    # Upper and lower lines calibration lines (1 percent of mean)
    band = total_mean * threshold
    ax.axhline(total_mean + band, color="black", ls="--")
    ax.axhline(total_mean - band, color="black", ls="--")

    # Add mean and stddev text annotations
    stat_str = "Mean = {:0.6f} Stdev = {:1.5e}".format(total_mean, total_stddev)
    plt_text = pltbox.AnchoredText(stat_str, loc="upper center", frameon=True)
    ax.add_artist(plt_text)

    plt.show()

    return fig, ax


def plot_corr(tof_ws):
    """
    Plots Pearson correlation coefficient for each detector
    :param tof_ws: Workspace returned from collect_fit_result
    :return: plot, plot axes
    """
    if not mtd.doesExist(str(tof_ws)):
        raise ValueError("Could not find the provided workspace in ADS")

    tof_ws = mtd[str(tof_ws)]

    numHist = tof_ws.getNumberHistograms()

    # Create an array for Pearson corr coef
    r_vals = np.empty((numHist,), dtype=float)
    r_vals.fill(np.nan)

    # Create an array for detector IDs
    detectors = tof_ws.detectorInfo().detectorIDs()
    detID = np.empty((numHist,), dtype=float)
    detID.fill(np.nan)

    for workspaceIndex in range(numHist):
        # Get Pearson correlation coefficient for each detector
        x = tof_ws.dataY(workspaceIndex)
        y = tof_ws.dataX(workspaceIndex)

        mask = np.logical_not(np.isnan(x))
        if np.sum(mask) > 1:
            r, p = np.corrcoef(x[mask], y[mask])
            # Use r[1] because the corr coef is always the off-diagonal element here
            r_vals[workspaceIndex] = r[1]
        else:
            r_vals[workspaceIndex] = np.nan

        # Get detector ID for this spectrum
        detID[workspaceIndex] = detectors[workspaceIndex]

    fig, ax = plt.subplots()
    ax.set_xlabel("det IDs")
    ax.set_ylabel("Pearson correlation coefficient (TOF, d)")

    ax.plot(detID, r_vals, marker="x", linestyle="None")


def plot_peak_info(wksp: Union[str, TableWorkspace], peak_positions: Union[float, list], donor=None):
    """
    Generates a plot using the PeakParameter Workspace returned from FitPeaks to show peak information
    (center, width, height, and intensity) for each bank at different peak positions.
    :param wksp: Peak Parameter TableWorkspace returned from FitPeaks
    :param peak_positions: List of peak positions to plot peak info at
    :param donor: Optional donor Workspace2D to use for collect_fit_result
    :return: plot, plot axes for each information type
    """
    wksp = mtd[str(wksp)]

    if not mtd.doesExist(str(wksp)):
        raise ValueError("Could not find provided workspace in ADS")

    peaks = peak_positions
    if isinstance(peak_positions, float):
        peaks = [peak_positions]

    if len(peaks) == 0:
        raise ValueError("Expected one or more peak positions")

    # Generate workspaces for each kind of peak info
    center = collect_fit_result(wksp, "center", peaks, donor=donor, infotype="centre")
    width = collect_fit_result(wksp, "width", peaks, donor=donor, infotype="width")
    height = collect_fit_result(wksp, "height", peaks, donor=donor, infotype="height")
    intensity = collect_fit_result(wksp, "intensity", peaks, donor=donor, infotype="intensity")

    nbanks = center.getNumberHistograms()
    workspaces = [center, width, height, intensity]
    labels = ["center", "width", "height", "intensity"]

    fig, ax = plt.subplots(len(workspaces), 1, sharex="all")
    for i in range(len(workspaces)):
        ax[i].set_ylabel(labels[i])

    # Show small ticks on x axis at peak positions
    ax[-1].set_xticks(peaks, True)
    ax[-1].set_xlabel(r"peak position ($\AA$)")

    markers = ["x", ".", "^", "s", "d", "h", "p", "v"]
    for i in range(nbanks):
        for j in range(len(workspaces)):
            x = workspaces[j].dataX(i)
            data = workspaces[j].dataY(i)
            # Cycle through marker list if there are too many banks
            marker = i % len(markers)
            ax[j].plot(x, data, marker=markers[marker], ls="None", label="bank{}".format(i))

    ax[0].legend()
    fig.tight_layout()

    plt.show()

    return fig, ax
