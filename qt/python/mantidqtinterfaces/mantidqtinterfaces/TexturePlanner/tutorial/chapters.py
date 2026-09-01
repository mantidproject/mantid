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

from mantidqt.widgets.tutorial.interaction import click, select_tab, set_check_state, set_spin_box, set_text
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
                "Use <b>Pause</b> to stop and look around, <b>Back</b> to re-read a step, or "
                "<b>Chapters…</b> to skip to a part you care about."
            ),
            dwell_ms=6000,
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
            dwell_ms=4500,
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
            dwell_ms=4500,
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
            dwell_ms=4500,
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
            dwell_ms=5000,
        ),
        TutorialStep(
            title="The material is shown here",
            text=(
                "Whatever is currently set on the sample is displayed beside the button, so you can "
                "tell at a glance whether the transmission estimates later on mean anything."
            ),
            target=lambda s: s.view.lblCurrentMaterialValue,
            dwell_ms=4000,
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
            dwell_ms=5000,
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
            dwell_ms=5000,
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
            dwell_ms=5500,
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
            dwell_ms=5000,
        ),
    ],
)


CHAPTERS = (SAMPLE_SETUP,)
