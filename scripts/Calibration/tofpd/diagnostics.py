from mantid.plots.resampling_image.samplingimage import imshow_sampling
from mantid.plots.datafunctions import get_axes_labels
from mantid.simpleapi import CalculateDIFC, LoadDiffCal, mtd
import matplotlib.pyplot as plt
from matplotlib.patches import Circle
from matplotlib.collections import PatchCollection
import numpy as np


# Diamond peak positions in d-space which may differ from actual sample
DIAMOND = np.asarray((2.06,1.2615,1.0758,0.892,0.8186,0.7283,0.6867,0.6307,0.5642,0.5441,0.515,0.4996,0.4768,
                      0.4645,0.4205,0.3916,0.3499,0.3257,0.3117))


def _get_xrange(wksp, xmarkers, tolerance, xmin, xmax):
    # start with the range determined by the markers and tolerance
    x_mins = [xmarkers.min() * (1. - 3 * tolerance)]
    x_maxs = [xmarkers.max() * (1. + 3 * tolerance)]
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
                difc_ws = CalculateDIFC(InputWorkspace="__cal_{}_group".format(ws_str),
                                        CalibrationWorkspace="__cal_{}_cal".format(ws_str),
                                        OutputWorkspace="__difc_{}".format(ws_str))
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
                difc_ws = CalculateDIFC(InputWorkspace=mtd[str(instr_ws)], CalibrationWorkspace=mtd[ws_str],
                                        OutputWorkspace="__difc_{}".format(ws_str))
        else:
            raise TypeError("Wrong workspace type. Expects SpecialWorkspace2D, TableWorkspace, or a filename")
    return difc_ws


def plot2d(workspace, tolerance: float=0.001, peakpositions: np.ndarray=DIAMOND,
           xmin: float=np.nan, xmax: float=np.nan, horiz_markers=[]):
    TOLERANCE_COLOR = 'w'  # color to mark the area within tolerance
    MARKER_COLOR = 'b'  # color for peak markers and horizontal (between bank) markers

    # convert workspace to pointer to workspace
    wksp = mtd[str(workspace)]

    # determine x-range
    xmin, xmax = _get_xrange(wksp, peakpositions, tolerance, xmin, xmax)

    # create a new figure
    fig, ax = plt.subplots()
    # show the image - this uses the same function used by "colorfill" plot
    # and slice viewer. The data is subsampled according to how may screen
    # pixels are available
    im = imshow_sampling(ax, wksp, aspect='auto', origin='lower')
    # add the colorbar
    fig.colorbar(im, ax=ax)
    # set the x-range to show
    ax.set_xlim(xmin, xmax)

    # annotate expected peak positions and tolerances
    for peak in peakpositions:
        if peak > 0.4 and peak < 1.5:
            shade = ((1-tolerance) * peak, (1+tolerance) * peak)
            ax.axvspan(shade[0], shade[1], color=TOLERANCE_COLOR, alpha=.2)
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


def difc_plot2d(calib_new, calib_old=None, instr_ws=None, mask=None, vrange=(0,1)):
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
    sample_position = info.samplePosition()
    maximum_solid_angle = 0.0
    for det_id in range(info.size()):
        maximum_solid_angle = max(maximum_solid_angle, delta.getDetector(det_id).solidAngle(sample_position))

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
    p.set_edgecolor('face')

    fig, ax = plt.subplots()
    ax.add_collection(p)
    mp = PatchCollection(masked_patches)
    mp.set_facecolor('gray')
    mp.set_edgecolor('face')
    ax.add_collection(mp)
    cb = fig.colorbar(p, ax=ax)
    cb.set_label('delta DIFC (%)')
    ax.set_xlabel(r'in-plane polar (deg)')
    ax.set_ylabel(r'azimuthal (deg)')

    # Find bounds based on detector pos
    xmin = np.min(theta_array)
    xmax = np.max(theta_array)
    ymin = np.min(phi_array)
    ymax = np.max(phi_array)
    ax.set_xlim(np.floor(xmin), np.ceil(xmax))
    ax.set_ylim(np.floor(ymin), np.ceil(ymax))

    fig.show()

    return fig, ax
