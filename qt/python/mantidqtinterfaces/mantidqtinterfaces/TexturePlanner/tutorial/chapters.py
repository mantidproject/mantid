# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
"""What the Texture Planner tutorial shows, in the order it shows it.

Each step is written against the sandbox (see ``sandbox.py``), which is why the actions may reach
for the presenter and the model as well as the widgets: the tour is demonstrating a workflow, and
some of that workflow is what the interface does in response, not only what the user clicks.

Two conventions worth knowing before adding a step:

* **Targets are looked up, not held.** ``lambda sandbox: sandbox.view.btnXML`` rather than a
  captured widget, because chapters are replayed against a freshly built interface whenever the
  user jumps to one.
* **Actions drive the interface, they do not fake it.** Where a step can go through the widget it
  is pointing at, it does - so what the user watches is the interface really working. The
  exceptions are called out where they occur, and each is a modal dialog that would otherwise stop
  the tour dead.
"""

from mantid.simpleapi import SetSampleMaterial

from mantidqt.widgets.tutorial.interaction import click, select_combo, select_tab, set_check_state, set_spin_box, set_text
from mantidqt.widgets.tutorial.step import TutorialChapter, TutorialStep

# the tour's sample material: deliberately not the interface's default, so the "Current material"
# label visibly changes when the step runs
DEMO_MATERIAL = "Cu"

# how long a file finder is given to finish its background search before the tour gives up on it.
# Generous on purpose - the finder walks every configured data search directory, which on a real
# instrument mount is a network round trip per candidate.
FINDER_TIMEOUT_S = 60.0


# ------------------------------------------------------------------------------------------------
# helpers used by the steps
# ------------------------------------------------------------------------------------------------


def _set_finder(finder, path):
    """Put a path into a Mantid file finder the way typing into it does.

    ``setFileTextWithSearch`` starts a search on a background thread, so the file is not available
    on the next line - the step that follows waits for ``_finder_ready``.
    """
    finder.setFileTextWithSearch(path)


def _finder_ready(finder):
    return not finder.isSearching()


def _apply_material(sandbox):
    """Set the sample material without the modal dialog the button opens.

    ``btnSetMaterial`` opens the standard ``SetSampleMaterial`` algorithm dialog, which is modal:
    it would block the tour's timers and the tutorial would stop at that step until the user dealt
    with a dialog they had not asked for. So the step points at the button and explains it, and the
    material is applied along the same path the dialog uses - the algorithm against the raw mesh
    workspace, then ``on_material_set`` to share it with the rest - so the result the user sees is
    exactly what clicking through the dialog would have produced.
    """
    SetSampleMaterial(InputWorkspace=sandbox.model.workspaces.WS_MESH_RAW, ChemicalFormula=DEMO_MATERIAL)
    sandbox.presenter.on_material_set()


# ------------------------------------------------------------------------------------------------
# chapter 1 - sample setup
# ------------------------------------------------------------------------------------------------


SAMPLE_SETUP = TutorialChapter(
    name="Sample setup",
    description="Load a sample shape, give it a material, and place it in the beam.",
    steps=[
        TutorialStep(
            title="Welcome to the Texture Planner",
            text=(
                "This is a working copy of the interface, loaded with a demo sample. Everything the "
                "tutorial does happens here — your own session is untouched.<br><br>"
                "Each step explains a control first. Press <b>Show me</b> to watch the tutorial use "
                "it, then <b>Next</b> to move on — nothing happens on its own. <b>Back</b> re-reads a "
                "step, and the tabs above jump to a chapter."
            ),
        ),
        TutorialStep(
            title="Two setup tabs",
            text=(
                "Planning an experiment has two halves. <b>Sample Setup</b> describes what you are "
                "measuring; <b>Experimental Setup</b> describes the instrument and how you will move "
                "the sample. We start with the sample."
            ),
            target=lambda s: s.view.tabSetup,
            action=lambda s: select_tab(s.view.tabSetup, "Sample Setup"),
        ),
        TutorialStep(
            title="Load a sample shape",
            text=(
                "The sample's shape comes from a file: an <b>STL mesh</b> exported from CAD, or a "
                "<b>CSG description</b> in Mantid's XML shape format. The tutorial is loading a 2 cm "
                "cube defined in XML."
            ),
            target=lambda s: s.view.finder_xml,
            action=lambda s: _set_finder(s.view.finder_xml, s.data.cube_xml_path),
            await_=lambda s: _finder_ready(s.view.finder_xml),
            await_timeout_s=FINDER_TIMEOUT_S,
            await_text="Looking for the sample file…",
        ),
        TutorialStep(
            title="Load it",
            text=(
                "<b>Load CSG</b> stays disabled until the file finder has found a valid file — the same "
                "is true of the STL button above. Watch the <b>Lab View</b> on the right when it is "
                "pressed."
            ),
            target=lambda s: s.view.btnXML,
            action=lambda s: click(s.view.btnXML),
            settle_ms=600,
        ),
        TutorialStep(
            title="Give the sample a material",
            text=(
                "Absorption depends on what the sample is made of. <b>Set Material</b> opens Mantid's "
                "standard <i>SetSampleMaterial</i> dialog, where you give a chemical formula and a "
                "density. The tutorial is setting copper for you rather than opening the dialog."
            ),
            target=lambda s: s.view.btnSetMaterial,
            action=_apply_material,
            settle_ms=400,
        ),
        TutorialStep(
            title="The material is shown here",
            text=(
                "Whatever is currently set on the sample is displayed beside the button, so you can "
                "tell at a glance whether the transmission estimates later on mean anything."
            ),
            target=lambda s: s.view.lblCurrentMaterialValue,
        ),
        TutorialStep(
            title="How the sample sits in its own frame",
            text=(
                "A shape file rarely arrives in the orientation you mounted it in. These angles rotate "
                "the sample once, before any goniometer motion — they describe how the sample sits in "
                "its holder."
            ),
            target=lambda s: s.view.initOrientation,
            action=lambda s: set_spin_box(s.view.spnInitX, 30.0),
            settle_ms=500,
        ),
        TutorialStep(
            title="And where it sits in the beam",
            text=(
                "The position offsets move the sample relative to the instrument's origin. Together "
                "with the gauge volume set up in the next chapter, this is what decides which part of "
                "the sample is actually being measured."
            ),
            target=lambda s: s.view.initPosition,
            action=lambda s: set_spin_box(s.view.spnInitPZ, 0.005),
            settle_ms=500,
        ),
        TutorialStep(
            title="Name your sample directions",
            text=(
                "Texture results are quoted in the sample's own frame — rolling, normal and transverse "
                "for a rolled plate, for instance. Naming those directions here means the pole figure "
                "is labelled in terms you recognise rather than in instrument coordinates."
            ),
            target=lambda s: s.view.grpDirectionWidgets,
            action=lambda s: set_check_state(s.view.grpDirectionWidgets, True),
            settle_ms=400,
        ),
        TutorialStep(
            title="Apply the directions",
            text=(
                "Edit the vectors and press <b>Update Directions</b> to re-project everything into the "
                "new frame. The tutorial has renamed the first direction to show where it appears."
            ),
            target=lambda s: s.view.updateDirs,
            action=lambda s: (set_text(s.view.lineedit_RD, "Rolling"), click(s.view.updateDirs)),
            settle_ms=600,
        ),
    ],
)


# ------------------------------------------------------------------------------------------------
# chapter 2 - experimental setup
# ------------------------------------------------------------------------------------------------


# how many orientations the tour builds. Enough to make a pole figure and a table worth looking at,
# few enough that the per-orientation absorption calculation in the next chapter stays quick.
DEMO_ORIENTATION_COUNT = 3

DEMO_GROUP = "Texture20"
DEMO_GAUGE_VOLUME = "4mmCube"


def _add_orientations(sandbox):
    for _ in range(DEMO_ORIENTATION_COUNT):
        click(sandbox.view.addOrientation)


EXPERIMENTAL_SETUP = TutorialChapter(
    name="Experimental setup",
    description="Choose the instrument, the gauge volume, and the goniometer moves to measure.",
    steps=[
        TutorialStep(
            title="Now the instrument",
            text=(
                "The second tab describes the measurement rather than the sample: which instrument, "
                "which detector grouping, what part of the sample the beam sees, and how you will "
                "rotate it."
            ),
            target=lambda s: s.view.tabSetup,
            action=lambda s: select_tab(s.view.tabSetup, "Experimental Setup"),
            settle_ms=300,
        ),
        TutorialStep(
            title="Pick an instrument",
            text=(
                "Choosing an instrument loads its detector geometry. Picking <b>Custom</b> instead lets "
                "you name any instrument definition Mantid can find, and supply your own grouping file."
            ),
            target=lambda s: s.view.cmbInstr,
        ),
        TutorialStep(
            title="…and a detector grouping",
            text=(
                "Texture measurements group detectors into banks that each look at the sample from a "
                "different direction — every group becomes one point per orientation on the pole figure. "
                f"The tutorial is selecting <b>{DEMO_GROUP}</b>."
            ),
            target=lambda s: s.view.cmbGroup,
            action=lambda s: select_combo(s.view.cmbGroup, DEMO_GROUP),
            settle_ms=300,
        ),
        TutorialStep(
            title="Apply the selection",
            text=(
                "Nothing is rebuilt until <b>Update Instrument</b> is pressed — and it stays disabled "
                "until the instrument and grouping together make sense. That keeps the interface out of "
                "half-applied states."
            ),
            target=lambda s: s.view.btnUpdateInstr,
            action=lambda s: click(s.view.btnUpdateInstr),
            settle_ms=800,
        ),
        TutorialStep(
            title="The gauge volume",
            text=(
                "The gauge volume is the region where the incident and scattered beams overlap — the "
                "part of the sample you are actually measuring. It decides the scattering centre, and "
                "with it the path lengths through the sample."
            ),
            target=lambda s: s.view.grpGaugeVol,
            action=lambda s: set_check_state(s.view.grpGaugeVol, True),
            settle_ms=400,
        ),
        TutorialStep(
            title="Choose a preset or your own shape",
            text=(
                f"<b>{DEMO_GAUGE_VOLUME}</b> is a preset; <b>Custom Shape</b> takes an XML shape file of "
                "your own, and <b>No Gauge Volume</b> treats the whole sample as illuminated."
            ),
            target=lambda s: s.view.combo_shapeMethod,
            action=lambda s: select_combo(s.view.combo_shapeMethod, DEMO_GAUGE_VOLUME),
            settle_ms=300,
        ),
        TutorialStep(
            title="Set it",
            text="<b>Set Gauge Volume</b> applies it and moves the scattering centre to match.",
            target=lambda s: s.view.setGV,
            action=lambda s: click(s.view.setGV),
            settle_ms=800,
        ),
        TutorialStep(
            title="Describe your goniometer",
            text=(
                "This is how the sample can be moved. Set the number of axes, then give each one a "
                "rotation vector and a sense — the tutorial is setting up two axes."
            ),
            target=lambda s: s.view.grpGoniometer,
            action=lambda s: set_spin_box(s.view.spnNumAxes, 2),
            settle_ms=500,
        ),
        TutorialStep(
            title="An axis is a vector and a sense",
            text=(
                "The vector is the axis of rotation in the lab frame; the sense says which way a positive "
                "angle turns. Axes beyond the number you asked for are greyed out."
            ),
            target=lambda s: s.view.axis0,
            action=lambda s: (set_text(s.view.edtVec0, "0,1,0"), select_combo(s.view.cmbSense0, "Counterclockwise")),
            settle_ms=500,
        ),
        TutorialStep(
            title="The step size",
            text=(
                "The step size sets how far each angle spin box moves per click, so you can walk through "
                "a scan in even increments rather than typing every angle."
            ),
            target=lambda s: s.view.spnStepSize,
            action=lambda s: set_spin_box(s.view.spnStepSize, 30.0),
            settle_ms=400,
        ),
        TutorialStep(
            title="Dial in an orientation",
            text=(
                "Set the angles for each axis and the lab view updates as you go, so you can see where "
                "the sample ends up before committing to it."
            ),
            target=lambda s: s.view.spnAngle0,
            action=lambda s: set_spin_box(s.view.spnAngle0, 30.0),
            settle_ms=700,
        ),
        TutorialStep(
            title="Add it to the list",
            text=(
                "<b>Add Orientation</b> records the current angles as one measurement position. The "
                f"tutorial is adding {DEMO_ORIENTATION_COUNT} of them so there is something to look at."
            ),
            target=lambda s: s.view.addOrientation,
            action=_add_orientations,
            settle_ms=1000,
        ),
        TutorialStep(
            title="Move between them",
            text=(
                "The index selector steps through the orientations you have added. Whichever one is "
                "selected is the one drawn in the lab view and highlighted on the pole figure."
            ),
            target=lambda s: s.view.spnIndex,
            action=lambda s: set_spin_box(s.view.spnIndex, 1),
            settle_ms=700,
        ),
    ],
)


# ------------------------------------------------------------------------------------------------
# chapter 3 - reading the results
# ------------------------------------------------------------------------------------------------


RESULTS = TutorialChapter(
    name="Reading the results",
    description="The lab view, the pole figure, transmission, and the orientation table.",
    steps=[
        TutorialStep(
            title="The lab view",
            text=(
                "The sample as the instrument sees it, in the currently selected orientation, together "
                "with the beam and the detector directions. This is the sanity check that your sample "
                "really is where you think it is."
            ),
            target=lambda s: s.view.grpSampleFigure,
        ),
        TutorialStep(
            title="The pole figure",
            text=(
                "Every detector group, for every orientation you added, projected into the sample's own "
                "frame. Gaps here are directions your planned measurement never samples — which is the "
                "whole reason for planning before beam time."
            ),
            target=lambda s: s.view.grpPoleFigure,
        ),
        TutorialStep(
            title="Transmission",
            text=(
                "Ticking this estimates how much of the beam survives the path through the sample for "
                "each point, by Monte Carlo simulation, and colours the pole figure by it. It is the "
                "slowest thing the interface does — the tutorial has turned the statistics right down."
            ),
            target=lambda s: s.view.chkTransmission,
        ),
        TutorialStep(
            title="Now with absorption",
            text=(
                "Dark points are directions where the beam has a long path through the sample, so they "
                "will need longer counting times — or a different sample orientation."
            ),
            target=lambda s: s.view.grpPoleFigure,
            action=lambda s: set_check_state(s.view.chkTransmission, True),
            settle_ms=1200,
        ),
        TutorialStep(
            title="The orientation table",
            text=(
                "One row per orientation, with the goniometer angles that produce it. <b>Include</b> "
                "controls whether an orientation is exported; <b>Select</b> is for the buttons below."
            ),
            target=lambda s: s.view.grpDynamicTable,
        ),
        TutorialStep(
            title="Selecting rows",
            text=(
                "<b>Select All</b> and <b>Deselect All</b> work on the selection column, and "
                "<b>Delete Selected</b> removes those rows from the plan — useful once the pole figure "
                "shows an orientation is not earning its beam time."
            ),
            target=lambda s: s.view.selectAll,
            action=lambda s: (click(s.view.selectAll), click(s.view.deselectAll)),
            settle_ms=700,
        ),
    ],
)


# ------------------------------------------------------------------------------------------------
# chapter 4 - exporting, and where the rest lives
# ------------------------------------------------------------------------------------------------


DEMO_EXPORT_FORMAT = "Sscanss2 Angles"


EXPORT = TutorialChapter(
    name="Exporting",
    description="Write the plan out, and where the remaining options live.",
    steps=[
        TutorialStep(
            title="Where to write it",
            text=(
                "The output section takes a directory and a file name. The tutorial is pointing it at a "
                "temporary folder, so the file it writes in a moment is a real export that disappears "
                "with this window."
            ),
            target=lambda s: s.view.finder_save_dir,
            action=lambda s: _set_finder(s.view.finder_save_dir, s.data.save_directory),
            await_=lambda s: _finder_ready(s.view.finder_save_dir),
            await_timeout_s=FINDER_TIMEOUT_S,
            await_text="Checking the output directory…",
            settle_ms=400,
        ),
        TutorialStep(
            title="And what to call it",
            text="Export stays disabled until both a directory and a file name are given.",
            target=lambda s: s.view.saveFileLine,
            action=lambda s: set_text(s.view.saveFileLine, s.data.save_filename),
            settle_ms=400,
        ),
        TutorialStep(
            title="Pick a format",
            text=(
                "<b>Sscanss2 Angles</b> feeds the plan straight into SScanSS-2. There are also Euler and "
                "matrix orientation files, a <b>Reference Workspace</b> that the Engineering Diffraction "
                "interface can load, and — once transmission has been estimated — a set of counting-time "
                "weightings."
            ),
            target=lambda s: s.view.cmbExportFormat,
            action=lambda s: select_combo(s.view.cmbExportFormat, DEMO_EXPORT_FORMAT),
            settle_ms=400,
        ),
        TutorialStep(
            title="Write it out",
            text="Only the orientations ticked as <b>Include</b> in the table are written.",
            target=lambda s: s.view.btnExport,
            action=lambda s: click(s.view.btnExport),
            settle_ms=800,
        ),
        TutorialStep(
            title="Everything else is in Settings",
            text=(
                "The cog holds the options the tour has skipped: how STL files are scaled and rotated on "
                "load, which axes Euler angles are written about, the Monte Carlo statistics, and the "
                "wavelength the attenuation is quoted at. They are remembered between sessions."
            ),
            target=lambda s: s.view.btn_settings,
        ),
        TutorialStep(
            title="That is the whole workflow",
            text=(
                "Describe the sample, describe the instrument and how you will move it, read the pole "
                "figure, export the plan.<br><br>"
                "The full documentation is under <b>Help → Mantid Help</b>. Close this window whenever "
                "you like — nothing here has touched your own session."
            ),
        ),
    ],
)


CHAPTERS = (SAMPLE_SETUP, EXPERIMENTAL_SETUP, RESULTS, EXPORT)
