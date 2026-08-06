# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
"""Shared setup for the Engineering Diffraction interface system tests.

These tests replace ``dev-docs/source/Testing/EngineeringDiffraction/EngineeringDiffractionTestGuide.rst``.
They build the real ``EngineeringDiffractionGui`` and click it with ``QTest``; the model, the
presenters, the algorithms and the files written to disk are all real. The only things mocked are
the ones that would block waiting for a user: the generated algorithm dialogs, the error popups,
and (in the GSAS-II tests) the external GSAS-II process.

This module holds the setup, the interaction helpers that are specific to this interface, and the
fixture that fabricates IMAT run data. It is not itself a test - the base class is abstract, which
is also what keeps the system test collector from picking it up out of the modules that import it.
"""

import os
import re
import sys
from abc import ABCMeta

import numpy as np

_PARENT_DIR = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
if _PARENT_DIR not in sys.path:
    sys.path.insert(0, _PARENT_DIR)

from automated_ui_test_base import AutomatedUITestBase  # noqa: E402
from qt_interaction_helpers import (  # noqa: E402
    click,
    process_events,
    select_combo,
    set_checkbox,
    set_finder_text,
    wait_for_file_finder,
)

TAB_PREFIX = "mantidqtinterfaces.Engineering.gui.engineering_diffraction.tabs"
CALIBRATION_PRESENTER = f"{TAB_PREFIX}.calibration.presenter"
FOCUS_PRESENTER = f"{TAB_PREFIX}.focus.presenter"
CORRECTION_PRESENTER = f"{TAB_PREFIX}.correction.presenter"
DATA_PRESENTER = f"{TAB_PREFIX}.common.data_handling.data_presenter"

# modules that import the blocking error popup by name, so each needs patching individually
ERROR_MESSAGE_MODULES = (CALIBRATION_PRESENTER, FOCUS_PRESENTER, DATA_PRESENTER)

# tab titles, as added in EngineeringDiffractionPresenter.setup_*
TAB_RUN_PROCESSING = "Run Processing"
TAB_CORRECTION = "Absorption Correction"
TAB_FITTING = "Fitting"
TAB_TEXTURE = "Texture"
TAB_GSAS2 = "GSAS II"

# ENGINX runs used by the guide, all present in Testing/Data/DocTest
ENGINX_CERIA = "305738"
ENGINX_VANADIUM = "307521"
ENGINX_FOCUS_RUN = "305761"
ENGINX_FOCUS_RUNS = "305793-305795"

# fabricated runs (see create_synthetic_ceria_and_vanadium). Run numbers well outside the range of
# anything shipped, so a fabricated file can never shadow a real one in the search path.
IMAT_CERIA_RUN = 99001
IMAT_VANADIUM_RUN = 99002
ENGINX_SYNTHETIC_CERIA_RUN = 99101
ENGINX_SYNTHETIC_VANADIUM_RUN = 99102


class EngDiffGuiTestBase(AutomatedUITestBase, metaclass=ABCMeta):
    """Builds the Engineering Diffraction interface and exposes its tabs.

    Subclasses implement ``_run_checks``. Anything they need before the interface exists (extra
    data directories, pre-seeded settings) goes in an override of ``setUp`` *before*
    ``super().setUp()``, via the ``pre_gui_setup`` hook.
    """

    def __init__(self):
        super(EngDiffGuiTestBase, self).__init__()
        self.gui = None
        self.save_dir = None

    # ------------------------------------------------------------------ hooks

    def pre_gui_setup(self):
        """Runs after settings isolation but before the interface is constructed, and before
        ``seeded_settings`` is read.

        Override to stage data files the interface will need - including any that a seeded setting
        has to point at, such as a fabricated calibration or an external program's install tree.
        """

    def seeded_settings(self):
        """Settings to write before the interface is built, as a plain dict.

        Defaults to pointing the save location at this test's temporary directory. A test that
        wants to drive the settings dialog itself (guide Test 1, steps 4-7) can return ``{}`` and
        set it through the real dialog instead.
        """
        return {"save_location": self.save_dir}

    # ------------------------------------------------------------------ lifecycle

    def setUp(self):
        super(EngDiffGuiTestBase, self).setUp()
        self.save_dir = os.path.join(self.tmp_root, "output")
        os.makedirs(self.save_dir, exist_ok=True)

        # stage files first: seeded settings routinely point at something pre_gui_setup created
        self.pre_gui_setup()
        for name, value in self.seeded_settings().items():
            self.set_engineering_setting(name, value)

        # error popups are modal and would hang an unattended run; the focus tab additionally asks
        # for confirmation before focusing many runs, and that answer decides whether it proceeds
        self.patch_error_messages(ERROR_MESSAGE_MODULES)
        self.patch_confirmation_box(FOCUS_PRESENTER, answer=True)

        self._build_gui()

    def rebuild_gui(self):
        """Close the interface and open a fresh one, leaving the settings store and the ADS alone.

        This is how the guide's "restart the interface" steps are reproduced: what they are really
        checking is that state which should survive a restart (the last calibration, the last
        vanadium run, the RB number) was written to settings and is read back on construction.
        """
        self.gui.close()
        process_events(2)
        self._build_gui()

    def _build_gui(self):
        # imported here rather than at module scope so a build without the interface skips cleanly
        from mantidqtinterfaces.Engineering.gui.engineering_diffraction.engineering_diffraction import EngineeringDiffractionGui

        self.gui = EngineeringDiffractionGui()
        self.gui.show()
        process_events(2)

        self.presenter = self.gui.presenter
        self.calibration_presenter = self.presenter.calibration_presenter
        self.focus_presenter = self.presenter.focus_presenter
        self.correction_presenter = self.presenter.correction_presenter
        self.fitting_presenter = self.presenter.fitting_presenter
        self.texture_presenter = self.presenter.texture_presenter
        self.gsas2_presenter = self.presenter.gsas2_presenter
        self.settings_presenter = self.presenter.settings_presenter

        # the Run Processing tab hosts both the calibration and focus presenters, so they share it
        self.run_processing_view = self.calibration_presenter.view
        self.cropping_view = self.calibration_presenter.cropping_widget.view
        self.correction_view = self.correction_presenter.view
        self.fitting_view = self.fitting_presenter.view
        self.texture_view = self.texture_presenter.view
        self.gsas2_view = self.gsas2_presenter.view
        self.settings_view = self.settings_presenter.view

    def tearDown(self):
        gui = getattr(self, "gui", None)
        if gui is not None:
            # Clear the ADS while the interface is still alive. The fitting, texture and GSAS-II
            # tabs all hold ADS observers that repopulate their tables on a clear, so clearing after
            # the window has gone drives those handlers into deleted C++ widgets.
            self._clear_ads()
            process_events(2)
            # the window is WA_DeleteOnClose, so the reference must not be touched after this
            gui.close()
            process_events(2)
            self.gui = None
        super(EngDiffGuiTestBase, self).tearDown()

    # ------------------------------------------------------------------ settings

    @staticmethod
    def set_engineering_setting(name, value):
        from mantidqtinterfaces.Engineering.gui.engineering_diffraction.settings.settings_helper import set_setting
        from mantidqtinterfaces.Engineering.gui.engineering_diffraction.tabs.common import output_settings

        set_setting(output_settings.INTERFACES_SETTINGS_GROUP, output_settings.ENGINEERING_PREFIX, name, value)

    @staticmethod
    def get_engineering_setting(name, return_type=str):
        from mantidqtinterfaces.Engineering.gui.engineering_diffraction.settings.settings_helper import get_setting
        from mantidqtinterfaces.Engineering.gui.engineering_diffraction.tabs.common import output_settings

        return get_setting(output_settings.INTERFACES_SETTINGS_GROUP, output_settings.ENGINEERING_PREFIX, name, return_type=return_type)

    def open_settings(self):
        """Open the settings dialog the way a user does, via the cog in the bottom left.

        The dialog is not modal-blocking (the presenter calls ``show()``, not ``exec_()``), so it
        can be driven directly once open.
        """
        click(self.gui.btn_settings)
        return self.settings_view

    def apply_settings(self, **fields):
        """Change settings through the real dialog and press OK.

        ``fields`` names map onto the dialog's own setters, e.g.
        ``apply_settings(save_location=..., peak_function="Gaussian")``. Going through the dialog
        rather than writing QSettings directly is the point: it is the path the guide describes,
        and it exercises the presenter's validation and its save-directory notification.
        """
        view = self.open_settings()
        for name, value in fields.items():
            setter = getattr(view, f"set_{name}")
            setter(value)
        # the save-location and full-calibration fields are file finders, so let their background
        # search settle before OK reads the resolved path back out
        for finder in (view.finder_save, view.finder_fullCalib):
            finder.findFiles(True)
            wait_for_file_finder(finder, msg="settings dialog file finder")
        click(view.btn_ok)
        process_events(2)

    # ------------------------------------------------------------------ interface state

    def show_tab(self, title):
        """Make a tab current. Widgets only report ``isVisible()`` on the current tab, so any
        visibility check has to select the tab first."""
        tabs = self.gui.tabs
        for index in range(tabs.count()):
            if tabs.tabText(index) == title:
                tabs.setCurrentIndex(index)
                process_events()
                return tabs.widget(index)
        raise AssertionError(f"no tab titled '{title}'; found {[tabs.tabText(i) for i in range(tabs.count())]}")

    def statusbar_text(self):
        """The 'info bar' at the bottom of the interface, which reports the loaded calibration."""
        return self.gui.status_label.text()

    def savedir_text(self):
        return self.gui.savedir_label.text()

    def set_instrument(self, instrument):
        select_combo(self.gui.comboBox_instrument, instrument)

    def set_rb_number(self, rb_number):
        self.gui.lineEdit_RBNumber.setText(rb_number)
        process_events()

    # ------------------------------------------------------------------ run processing

    def set_region_of_interest(self, description, custom_spectra=None, custom_grouping_file=None):
        """Tick 'Set Calibration Region of Interest' and choose an option by its combo text.

        ``description`` is the label the user sees, e.g. "1 (North)", "Crop to Spectra",
        "Texture (20 spec)". Passing ``None`` unticks the checkbox, which is the guide's
        "no region of interest" case and makes the calibration fall back to both banks.
        """
        self.show_tab(TAB_RUN_PROCESSING)
        if description is None:
            set_checkbox(self.run_processing_view.check_roiCalib, False)
            return
        set_checkbox(self.run_processing_view.check_roiCalib, True)
        select_combo(self.cropping_view.combo_bank, description)
        if custom_spectra is not None:
            self.cropping_view.edit_crop.setText(custom_spectra)
            process_events()
        if custom_grouping_file is not None:
            set_finder_text(self.cropping_view.finder_custom, custom_grouping_file)
        process_events()

    def calibrate(self, ceria=ENGINX_CERIA, vanadium=ENGINX_VANADIUM, plot_output=False):
        """Run a new calibration through the interface and wait for it to finish."""
        self.show_tab(TAB_RUN_PROCESSING)
        view = self.run_processing_view
        view.radio_newCalib.setChecked(True)
        process_events()
        set_finder_text(view.finder_vanadium, vanadium)
        set_finder_text(view.finder_sample, ceria)
        set_checkbox(view.check_plotOutput, plot_output)

        click(view.button_calibrate)
        self.wait_for_async_task(self.calibration_presenter.worker, what="calibration")
        return self.calibration_presenter.current_calibration

    def load_calibration(self, prm_path):
        """Load an existing calibration from a .prm file, as the guide's 'Load Existing
        Calibration' radio button does. This path is synchronous - no worker is started."""
        self.show_tab(TAB_RUN_PROCESSING)
        view = self.run_processing_view
        view.radio_loadCalib.setChecked(True)
        process_events()
        set_finder_text(view.finder_path, prm_path)
        click(view.button_calibrate)  # the button is relabelled "Load" in this mode
        process_events(2)
        return self.calibration_presenter.current_calibration

    def focus(self, runs=ENGINX_FOCUS_RUN, plot_output=False):
        """Focus one or more sample runs and wait for the worker."""
        self.show_tab(TAB_RUN_PROCESSING)
        view = self.run_processing_view
        set_finder_text(view.finder_focus, runs)
        set_checkbox(view.check_focusPlotOutput, plot_output)

        click(view.button_focus)
        self.wait_for_async_task(self.focus_presenter.worker, what="focus")

    # ------------------------------------------------------------------ output locations

    def calibration_dir(self, rb_number=None):
        return self._output_dir("Calibration", rb_number)

    def focus_dir(self, rb_number=None):
        return self._output_dir("Focus", rb_number)

    def _output_dir(self, name, rb_number):
        if rb_number:
            return os.path.join(self.save_dir, "User", rb_number, name)
        return os.path.join(self.save_dir, name)

    def focused_workspace_names(self):
        """Focused output workspaces currently in the ADS, in spectrum order."""
        from mantid.api import AnalysisDataService as ADS
        from Engineering.EnggUtils import FOCUSED_OUTPUT_WORKSPACE_NAME

        # the spectrum/group number is a trailing suffix, so a plain sort puts _10 before _2
        def spectrum_order(name):
            match = re.search(r"(\d+)$", name)
            return (int(match.group(1)) if match else -1, name)

        names = [name for name in ADS.getObjectNames() if FOCUSED_OUTPUT_WORKSPACE_NAME in name]
        return sorted(names, key=spectrum_order)


# ---------------------------------------------------------------------- IMAT data fixture

# Subsample the instrument. Enough detectors that every shipped grouping still has a healthy number
# in each group - the finest is ENGIN-X's Texture30, at 30 groups - but small enough to keep the
# file and the focusing quick. Taken as an even stride so the sample is spread across every bank.
_TARGET_SPECTRA = 960

# d range the expected ceria peaks are drawn from (they span 0.781 - 3.141 A)
_D_MIN, _D_MAX = 0.5, 3.6

# peak area, scaled with d so the long wavelength peaks are not swamped once the vanadium division
# has flattened the spectrum. Well above PDCalibration's MinimumPeakHeight at every d.
_PEAK_INTENSITY_PER_D = 4.0e4

# peak width as a fraction of the peak's TOF position. Wide enough that the closest expected pair
# (0.9019 / 0.9147 A) stays many sigma apart, narrow enough to sit inside PDCalibration's windows.
_REL_SIGMA = 0.002


def create_imat_ceria_and_vanadium(out_dir):
    """Fabricate an IMAT ceria and vanadium run. See ``create_synthetic_ceria_and_vanadium``."""
    return create_synthetic_ceria_and_vanadium("IMAT", out_dir, IMAT_CERIA_RUN, IMAT_VANADIUM_RUN)


def create_enginx_ceria_and_vanadium(out_dir, extra_sample_runs=()):
    """Fabricate an ENGIN-X ceria and vanadium run. See ``create_synthetic_ceria_and_vanadium``."""
    return create_synthetic_ceria_and_vanadium(
        "ENGINX", out_dir, ENGINX_SYNTHETIC_CERIA_RUN, ENGINX_SYNTHETIC_VANADIUM_RUN, extra_sample_runs
    )


def create_synthetic_ceria_and_vanadium(instrument, out_dir, ceria_run, vanadium_run, extra_sample_runs=()):
    """Fabricate a ceria and vanadium run that survive the real calibrate/focus path.

    Used for both instruments. For IMAT there is no choice - the repository has no ceria/vanadium
    pair at all. For ENGIN-X it is a deliberate trade: the real runs are large and slow to load and
    focus, and what the Run Processing tests are actually checking is the interface, the output file
    layout and the reported state, none of which depend on the counts being real. The numerical
    correctness of the calibration chain is already covered by ``EnginXScriptTest``.

    The files are written with the run-number naming the file finder expects, so the interface loads
    them exactly as it would a real run rather than the test reaching past the view to inject a
    workspace.

    Counts are written directly in TOF, with each detector's peaks placed at ``difc * d`` using its
    own entry in the full instrument calibration, so they stay aligned through
    ``DiffractionFocussing`` and give ``PDCalibration`` a problem whose answer is difa = tzero = 0.
    The peaks are Gaussian, so the calibration must be run with the peak function set to
    ``Gaussian`` - see ``_fill_ceria_peaks``.

    :param instrument: "ENGINX" or "IMAT"
    :param out_dir: directory to write the two nexus files into
    :param ceria_run: run number to give the fabricated ceria run
    :param vanadium_run: run number to give the fabricated vanadium run
    :param extra_sample_runs: further run numbers to write with the same peaks as the ceria run, for
        tests that need more than one sample to focus or fit
    :return: (ceria_path, vanadium_path)
    """
    from mantid.simpleapi import CreateSimulationWorkspace, ExtractSpectra, Load
    from Engineering.EnggUtils import CALIB_DIR
    from Engineering.common.instrument_config import get_instr_config

    config = get_instr_config(instrument)
    tof_min, _tof_step, tof_max = config.calibration_tof_binning
    # name it as the interface would, so CalibrationModel.load_full_instrument_calibration reuses
    # this very workspace instead of reloading and risking a mismatch with the fabricated data
    full_calib = Load(
        Filename=os.path.join(CALIB_DIR, config.full_instr_calib),
        OutputWorkspace=f"full_inst_calib_{instrument}",
    )

    # a coarse template keeps this tiny; Rebin then subdivides it, so the fine grid is only ever
    # materialised for the subsampled spectra. Two bins is the minimum the algorithm accepts.
    template_name = f"__{instrument.lower()}_template"
    template = CreateSimulationWorkspace(
        Instrument=instrument,
        BinParams=f"{tof_min},{0.5 * (tof_max - tof_min)},{tof_max}",
        UnitX="TOF",
        OutputWorkspace=template_name,
    )
    stride = max(1, template.getNumberHistograms() // _TARGET_SPECTRA)
    indices = list(range(0, template.getNumberHistograms(), stride))
    template = ExtractSpectra(InputWorkspace=template, WorkspaceIndexList=indices, OutputWorkspace=template_name)

    ceria_path = _write_synthetic_run(instrument, template, full_calib, ceria_run, out_dir, ceria=True)
    vanadium_path = _write_synthetic_run(instrument, template, full_calib, vanadium_run, out_dir, ceria=False)
    for run in extra_sample_runs:
        _write_synthetic_run(instrument, template, full_calib, run, out_dir, ceria=True)
    return ceria_path, vanadium_path


def _write_synthetic_run(instrument, template, full_calib, run_number, out_dir, ceria):
    from mantid.simpleapi import AddSampleLog, Rebin, SaveNexusProcessed
    from Engineering.EnggUtils import default_ceria_expected_peaks
    from Engineering.common.instrument_config import get_instr_config

    name = f"__{instrument.lower()}_{'ceria' if ceria else 'vanadium'}"
    tof_min, tof_step, tof_max = get_instr_config(instrument).calibration_tof_binning
    ws = Rebin(InputWorkspace=template, Params=f"{tof_min},{tof_step},{tof_max}", OutputWorkspace=name)

    edges = ws.readX(0)
    tof = 0.5 * (edges[1:] + edges[:-1])
    tof_centre = 0.5 * (tof_min + tof_max)

    if not ceria:
        # featureless and strictly positive, so the vanadium background estimate returns
        # essentially the input and the subsequent division never divides by zero
        y = 800.0 + 400.0 * np.exp(-(((tof - tof_centre) / (0.5 * (tof_max - tof_min))) ** 2))
        e = np.sqrt(y)
        for index in range(ws.getNumberHistograms()):
            ws.setY(index, y)
            ws.setE(index, e)
    else:
        _fill_ceria_peaks(ws, tof, full_calib, default_ceria_expected_peaks(final=True))

    AddSampleLog(Workspace=ws, LogName="run_number", LogType="String", LogText=str(run_number))
    # without a proton charge the interface's loader silently rejects the run (NormaliseByCurrent)
    AddSampleLog(Workspace=ws, LogName="gd_prtn_chrg", LogType="Number", NumberType="Double", LogText="1.0")

    # both instruments zero pad their run numbers to 8 digits, which is what
    # path_handling.get_run_number_from_path strips back off
    path = os.path.join(out_dir, f"{instrument}{run_number:08d}.nxs")
    SaveNexusProcessed(InputWorkspace=ws, Filename=path)
    return path


def _fill_ceria_peaks(ws, tof, full_calib, expected_d_values):
    """Write one Gaussian per expected ceria peak into every spectrum.

    Gaussians rather than the instrument's own ``IkedaCarpenterPV``, and the calibration is run with
    the peak function set to ``Gaussian`` to match (see ``EngDiffGuiImatCalibrateAndFocusTest``).
    Generating from ``IkedaCarpenterPV`` is more faithful, but it is slow on both sides - the
    function has to be evaluated once per peak per spectrum rather than as one vectorised
    expression, and fitting it with ``RespectFixedPeakParameters`` is far more expensive than a
    Gaussian - which made this the longest running class in the suite by a wide margin.

    What matters here is that the *generated* and *fitted* shapes agree, which is what gives
    PDCalibration a well conditioned problem with a known answer of difa = tzero = 0. IMAT's real
    default peak function is still asserted, at the configuration level, in
    ``EngDiffGuiImatSettingsTest``.
    """
    # detector id -> difc, so each detector's peaks land where its own calibration puts them
    difc_by_detid = {int(row["detid"]): float(row["difc"]) for row in full_calib}
    if not difc_by_detid:
        # otherwise every spectrum would come out as background only and the fixture would produce
        # peakless data that fails much later, in the calibration, for no obvious reason
        raise AssertionError("the full calibration table has no difc entries, so no peaks can be generated")

    # a smooth, strictly positive background; the peaks sit far above it
    background = 100.0 + 20.0 * np.exp(-(((tof - 40000.0) / 25000.0) ** 2))
    d_values = [d for d in expected_d_values if _D_MIN <= d <= _D_MAX]

    for index in range(ws.getNumberHistograms()):
        difc = difc_by_detid.get(ws.getDetector(index).getID())
        if difc is None:
            # no calibration entry for this detector, so leave it as background only
            ws.setY(index, background)
            ws.setE(index, np.sqrt(background))
            continue
        y = background.copy()
        for d in d_values:
            centre = difc * d
            if not tof[0] < centre < tof[-1]:
                continue
            sigma = _REL_SIGMA * centre
            y += (_PEAK_INTENSITY_PER_D * d / (sigma * np.sqrt(2.0 * np.pi))) * np.exp(-0.5 * ((tof - centre) / sigma) ** 2)
        ws.setY(index, y)
        ws.setE(index, np.sqrt(y))
