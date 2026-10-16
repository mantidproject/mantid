# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
"""The throwaway Texture Planner the tutorial drives.

A complete second instance - its own model, view and presenter - so the tour can load a sample,
set a material, add orientations and export a file without any of it reaching the planner the user
has open. Two things already in the interface make that safe rather than merely hopeful:

* ``WorkspaceManager`` gives every instance's workspaces a name ending in a unique token, so two
  planners never collide in the ADS, and
* closing the window runs ``TexturePlannerPresenter.on_close``, which removes all of them.

So the sandbox needs no special isolation of its own - it needs to be *closed*, which is what
``teardown`` does.
"""

from mantidqt.widgets.tutorial.sandbox_base import InterfaceSandbox
from mantidqtinterfaces.TexturePlanner.model import TexturePlannerModel
from mantidqtinterfaces.TexturePlanner.presenter import TexturePlannerPresenter
from mantidqtinterfaces.TexturePlanner.tutorial.demo_data import DemoData
from mantidqtinterfaces.TexturePlanner.view import TexturePlannerView

# The tour ticks "show transmission", which runs MonteCarloAbsorption for every orientation
# *synchronously*, on the GUI thread. At the interface's default 50 events per point that is long
# enough to look like the tutorial has hung. The tour is demonstrating where the control is and
# what it produces, not the accuracy of the simulation, so the sandbox turns the statistics right
# down - and only ever on its own model.
DEMO_MC_EVENTS_PER_POINT = 3


class TutorialSandbox(InterfaceSandbox):
    """A Texture Planner built for the tutorial to drive, and the demo files it uses.

    Closing is what removes the workspaces: ``closeEvent`` calls the presenter's ``on_close``,
    which calls ``WorkspaceManager.cleanup``. Doing it by hand in ``teardown`` as well would be a
    second place to keep in step with what a planner owns.
    """

    def __init__(self, parent=None):
        self.data = DemoData()

        model = TexturePlannerModel()
        # register_usage=False: this window is Mantid opening the interface to demonstrate it, not
        # the user opening it to work, and counting it would inflate every real launch
        view = TexturePlannerView(parent=parent, register_usage=False)
        # offer_tutorial=False: this planner is the tutorial. Left True it would offer a tutorial
        # from inside one, and on a first-ever open would launch a second tour recursively
        super().__init__(model, view, TexturePlannerPresenter(model, view, offer_tutorial=False))

        # deliberately not given window flags or a title: the tutorial shell takes this view as a
        # child and is itself the window, so anything set here would never be seen
        self.model.absorption.mc_kwargs["EventsPerPoint"] = DEMO_MC_EVENTS_PER_POINT

    def _cleanup(self):
        self.data.cleanup()


def make_sandbox_factory(parent=None):
    """A no-argument factory for ``run_tutorial``, which rebuilds the sandbox whenever the user
    jumps to a chapter."""

    def build():
        return TutorialSandbox(parent=parent)

    return build
