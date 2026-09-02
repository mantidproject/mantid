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

from mantidqt.widgets.tutorial.interaction import (
    click,
    process_events,
    select_combo,
    select_tab,
    set_check_state,
    set_spin_box,
    set_text,
)
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


def _permute_directions(sandbox):
    """Cycle the three texture directions, so a different one lands in the middle row.

    The pole figure is projected about the second direction (see ``get_alpha_beta_from_cart``,
    whose polar angle is measured from +y, and the second column of the model's ``ax_transform``
    is what ends up there). Cycling is the clearest demonstration of that: nothing about the
    measurement changes, only which axis it is viewed down.
    """
    view = sandbox.view
    rd, nd, td = view.get_rd_dir(), view.get_nd_dir(), view.get_td_dir()
    view.set_rd_dir(tuple(nd.split(",")))
    view.set_nd_dir(tuple(td.split(",")))
    view.set_td_dir(tuple(rd.split(",")))
    click(view.updateDirs)


def _show_pinned_directions(sandbox):
    """Tilt the sample and pin the texture directions to it.

    Both, together, because the check box on its own has nothing to show: it applies the initial
    orientation to the sample directions as well as to the shape, so with the sample square to the
    lab frame the pinned and unpinned directions are identical. Tilting it first is what makes the
    difference visible on the pole figure.
    """
    set_spin_box(sandbox.view.spnInitX, 30.0)
    set_check_state(sandbox.view.chkTransformDirs, True)


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
                "This is a working copy of the interface, which we will step through as a demo. "
                "Everything the tutorial does happens in this self-contained window, "
                "the session which opened this tutorial is untouched.<br><br>"
                "Each step explains a control first. Press <b>Show me</b> to watch the tutorial use "
                "it, then <b>Next</b> to move on. <b>Back</b> re-reads a "
                "step, and the tabs at the top of the window allow you to jump to a chapter.<br><br>"
                "As the whole window is live, you can interact with the widgets being shown "
                "(and are encouraged to do so) or indeed you can interact with any "
                "of the interface just note that the more you change the less clear the rest of the tutorial may be!"
            ),
        ),
        TutorialStep(
            title="Two setup tabs",
            text=(
                "The setup controls are split into two halves. <b>Sample Setup</b> describes what you are "
                "measuring; <b>Experimental Setup</b> describes the instrument and how you will move "
                "the sample. We start with the sample. <br><br>"
                "(<b>Show me</b> will simply select the <b>Sample Setup</b> tab in case you have changed to "
                "<b>Experimental Setup</b>)"
            ),
            target=lambda s: s.view.tabSetup,
            action=lambda s: select_tab(s.view.tabSetup, "Sample Setup"),
        ),
        TutorialStep(
            title="Load a sample shape",
            text=(
                "The sample's shape comes from a file: an <b>STL mesh</b> exported from CAD/photo-scanning"
                ", or a <b>CSG description</b> in Mantid's XML shape format. The tutorial is loading a "
                "30 × 10 × 20 mm cuboid defined in XML — a deliberately lopsided block, so every "
                "rotation of it looks different in the lab view."
            ),
            target=lambda s: s.view.finder_xml,
            action=lambda s: _set_finder(s.view.finder_xml, s.data.cuboid_xml_path),
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
                "density. The tutorial is setting an example copper material for you rather than "
                "interacting with the dialog."
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
                "A shape file is not necessarily defined in the same orientation that it is mounted in the beam. "
                "These angles rotate the sample once, before any goniometer motion; "
                "they describe how the sample sits in the beam before positioning."
            ),
            target=lambda s: s.view.initOrientation,
            action=lambda s: set_spin_box(s.view.spnInitX, 90.0),
            settle_ms=500,
        ),
        TutorialStep(
            title="Where the sample sits in the beam",
            text=(
                "The position offsets move the sample relative to the instrument's origin. Together "
                "with the gauge volume set up in the next chapter, this is what decides which part of "
                "the sample is actually being measured.<br><br>"
                "Note that without a gauge volume the <i>whole</i> sample is still assumed to be illuminated "
                "by the beam, hence the location of the scattering vector and sample directions remaining at "
                "the sample centre."
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
        ),
        TutorialStep(
            title="The middle row is the pole figure axis",
            text=(
                "The order matters. The pole figure is always projected about the <b>second</b> of the "
                "three directions — <b>ND</b> here — with the first and third becoming the horizontal "
                "and vertical of the plot.<br><br>"
                "So which direction you put in the middle row decides which pole you are looking down."
            ),
            target=lambda s: s.view.groupBox_textureVectors,
        ),
        TutorialStep(
            title="Pinning the directions to the shape",
            text=(
                "<b>Apply Orientation to Directions</b> is for the common case where the sample "
                "directions and the shape file are both easy to write down in the <i>sample</i> frame "
                "and awkward in the lab frame.<br><br>"
                "Say you know the rolling direction runs along the length of the block. Both the shape "
                "and the directions are obvious in the sample's own frame — but once the block is "
                "mounted at some angle in the beam, neither is obvious in the lab frame."
            ),
            target=lambda s: s.view.chkTransformDirs,
        ),
        TutorialStep(
            title="One rotation aligns both",
            text=(
                "Tick it and an <b>initial orientation</b> (now set to 30 degrees) is applied to the sample "
                "directions as well as to the shape. <br><br>"
                "Try toggling the box yourself to see the lab-frame vs sample-frame definition.<br><br>"
                "Watch the pole figure: the measurements have not changed, but the frame they are "
                "quoted in rotates with the sample."
            ),
            target=lambda s: s.view.chkTransformDirs,
            avoid=lambda s: (s.view.groupBox_textureVectors, s.view.grpPoleFigure),
            action=_show_pinned_directions,
            settle_ms=800,
        ),
        TutorialStep(
            title="Seeing them in the lab frame",
            text=(
                "<b>View in Lab Frame</b> shows what those directions have become in instrument "
                "coordinates. The fields go read-only, because these values are derived — the sample "
                "frame is still where you edit them.<br><br>"
                "Again, try toggling this yourself"
            ),
            target=lambda s: s.view.chkLabDirs,
            # the change this makes shows up in the vector fields, not at the check box
            avoid=lambda s: s.view.groupBox_textureVectors,
            action=lambda s: set_check_state(s.view.chkLabDirs, True),
            settle_ms=600,
        ),
        TutorialStep(
            title="Back to the sample frame",
            text=("Untick it to edit the directions again. The tutorial is switching back so the next step can change them."),
            target=lambda s: s.view.chkLabDirs,
            avoid=lambda s: s.view.groupBox_textureVectors,
            action=lambda s: set_check_state(s.view.chkLabDirs, False),
            settle_ms=600,
        ),
        TutorialStep(
            title="Permute them and the projection changes",
            text=(
                "The tutorial is cycling the three vectors — the direction that was RD moves into the "
                "middle row, so it becomes the pole. Press <b>Show me</b> and watch the pole figure "
                "re-project: the same measurements, viewed down a different axis."
            ),
            target=lambda s: s.view.updateDirs,
            action=_permute_directions,
            settle_ms=800,
        ),
    ],
)


# ------------------------------------------------------------------------------------------------
# chapter 2 - experimental setup
# ------------------------------------------------------------------------------------------------


# The planner opens on ENGINX, so the tour moves to another instrument: watching the detector
# geometry and the pole figure change is what shows the control doing something.
DEMO_INSTRUMENT = "IMAT"
DEMO_GROUP = "Module4"
DEMO_GAUGE_VOLUME = "4mmCube"

# The angle each added orientation is recorded at. Every one is different, so stepping through the
# index selector afterwards visibly rotates the sample and moves its points on the pole figure -
# with identical orientations there would be nothing to see. Kept short because the absorption
# calculation in the next chapter runs once per orientation.
# The first matches the angle the "dial in an orientation" step sets, so the orientation the user
# just watched being dialled is the one that gets recorded rather than being silently reset.
DEMO_ANGLES = (30.0, 60.0, 90.0)


def _add_orientations(sandbox):
    """Record one orientation per angle in ``DEMO_ANGLES``.

    Adding an orientation selects the new one and stores whatever the angle fields currently hold,
    so the angle is set *before* each add. The first angle applies to the orientation the planner
    already starts with, which is why there is one fewer add than there are angles.
    """
    view = sandbox.view
    for index, angle in enumerate(DEMO_ANGLES):
        if index > 0:
            click(view.addOrientation)
        set_spin_box(view.spnAngle0, angle)


def _step_through_orientations(sandbox):
    """Walk the index selector back through every orientation, ending on the first.

    Ends on the first rather than the last so the lab view is left showing an orientation the user
    has just watched it move to, rather than the one it was already displaying.
    """
    for index in reversed(range(len(DEMO_ANGLES))):
        set_spin_box(sandbox.view.spnIndex, index + 1)
        process_events(3)


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
                "Choosing an instrument loads its detector geometry. The planner opens on ENGINX; the "
                f"tutorial is switching to <b>{DEMO_INSTRUMENT}</b>, which has a different detector "
                "layout.<br><br>"
                "Picking <b>Custom</b> instead lets you name any instrument definition Mantid can find, "
                "and supply your own grouping file."
            ),
            target=lambda s: s.view.cmbInstr,
            action=lambda s: select_combo(s.view.cmbInstr, DEMO_INSTRUMENT),
            settle_ms=400,
        ),
        TutorialStep(
            title="Pick a detector grouping",
            text=(
                "Texture measurements group detectors into banks that each look at the sample from a "
                "different direction — every group becomes one point per orientation on the pole figure. "
                f"The groups on offer follow the instrument; the tutorial is selecting <b>{DEMO_GROUP}</b>."
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
                f"tutorial is adding {len(DEMO_ANGLES)}, each at a different angle — "
                f"{', '.join(f'{angle:g}°' for angle in DEMO_ANGLES)} — so the plan covers more of the "
                "pole figure."
            ),
            target=lambda s: s.view.addOrientation,
            action=_add_orientations,
            settle_ms=1000,
        ),
        TutorialStep(
            title="Move between them",
            text=(
                "The index selector steps through the orientations you have added. Whichever one is "
                "selected is the one drawn in the lab view and highlighted on the pole figure — watch "
                "the sample turn as the tutorial steps back through them."
            ),
            target=lambda s: s.view.spnIndex,
            action=_step_through_orientations,
            settle_ms=1000,
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
            title="The goniometer axes and rings",
            text=(
                "Each goniometer is drawn as an <b>arrow</b> along its rotation axis and a <b>ring</b> "
                "showing the circle it sweeps. The rings nest: an axis is drawn inside the ones it is "
                "mounted on, so the stack reads outermost-first.<br><br>"
                "The coloured part of a ring is the angle it has been rotated through; the grey "
                "remainder is the travel left. The axis you changed most recently is drawn "
                "<b>solid</b>, the others dashed."
            ),
            target=lambda s: s.view.grpSampleFigure,
        ),
        TutorialStep(
            title="The pole figure",
            text=(
                "Every detector group, for every orientation you added, projected into the sample's own "
                "frame. The gaps here show you directions around your sample the proposed measurement "
                "don't cover (remembering that the individual points just represent the average position of "
                "all the detectors in the group and the actual depends on the entire group rather than "
                "just this centre)"
            ),
            target=lambda s: s.view.grpPoleFigure,
        ),
        TutorialStep(
            title="The goniometers appear here too",
            text=(
                "Each goniometer axis is projected onto the pole figure as well — as a <b>point</b>, or "
                "as a <b>line</b> across the figure when the axis lies in its plane.<br><br>"
                "The fill matches the lab view: the axis you last changed is drawn <b>filled</b> (or "
                "solid, for a line), the rest hollow and dashed. It is the quickest way to see which "
                "axis you are currently moving and where it points."
            ),
            target=lambda s: s.view.grpPoleFigure,
        ),
        TutorialStep(
            title="Which orientation is which",
            text=(
                "The pole figure distinguishes the orientation you have selected from the rest:<br>"
                "• <b>Filled</b> points — the current orientation, included in the plan.<br>"
                "• <b>Hollow</b> points — every other included orientation.<br>"
                "• <b>Faint grey</b> points — the current orientation when it is <i>not</i> included.<br>"
                "• Orientations that are neither current nor included are not drawn at all.<br><br>"
                "So stepping the index selector walks the filled points around the figure - try doing this now!"
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
                "will need longer counting times — or a different sample orientation.<br><br>"
                "With every point coloured by transmission there is no fill left to mark the current "
                "orientation, so it is <b>ringed</b> with oversized open circles instead. Only included "
                "orientations are shown at all. The ring can be turned off under "
                "<i>Attenuation Settings</i> in the settings menu."
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
            text=("<b>Select All</b> ticks every row's selection box. Watch the <b>Select</b> column on the right of the table."),
            target=lambda s: s.view.selectAll,
            action=lambda s: click(s.view.selectAll),
            settle_ms=700,
        ),
        TutorialStep(
            title="…and clearing them",
            text=(
                "<b>Deselect All</b> clears the column again. <b>Delete Selected</b> removes the ticked "
                "rows from the plan altogether — useful once the pole figure shows an orientation is "
                "not earning its beam time."
            ),
            target=lambda s: s.view.deselectAll,
            action=lambda s: click(s.view.deselectAll),
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
                "The cog holds the options the tour has skipped: how STL files are scaled and rotated "
                "on load, which axes Euler angles are written about, the Monte Carlo statistics behind "
                "the transmission estimate, and the wavelength the attenuation is quoted at — including "
                "the ring around the current orientation you saw a moment ago. They are remembered "
                "between sessions."
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
