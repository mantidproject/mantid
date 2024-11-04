"""
mantiddoc.directive.plot_directive
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

A thin wrapper around matplotlib.sphinxext.plot_directive
to be able to control execution via an environment variable

:copyright: Copyright 2020
    ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory
"""

import os
import matplotlib.sphinxext.plot_directive as mpl_plot_directive_module

if hasattr(mpl_plot_directive_module, "PlotDirective"):

    class PlotDirectivePatch(mpl_plot_directive_module.PlotDirective):
        """Overridden directive to allow disabling plots via environment
        variable
        """

        def run(self):
            if plot_directive_disabled():
                return insert_placeholder_caption(self.state_machine)
            else:
                return super().run()

    def setup(app):
        metadata = mpl_plot_directive_module.setup(app)
        kwargs = dict()
        # avoid warning overwriting directive
        kwargs["override"] = True
        app.add_directive("plot", PlotDirectivePatch, **kwargs)
        return metadata

else:
    # Save original definition
    plot_directive_orig = mpl_plot_directive_module.plot_directive

    def plot_directive_patch(name, arguments, options, content, lineno, content_offset, block_text, state, state_machine):
        """
        Drop-in replacement for plot_directive allowing plots to be
        enabled by defining ENABLE_PLOTDIRECTIVE environment variable.
        If this is undefined or empty then plots are not generated and
        a placeholder-text is inserted.
        """
        if plot_directive_disabled():
            return insert_placeholder_caption(state_machine)
        else:
            return plot_directive_orig(name, arguments, options, content, lineno, content_offset, block_text, state, state_machine)

    # Monkey-patch in our definition
    mpl_plot_directive_module.plot_directive = plot_directive_patch

    def setup(app):
        return mpl_plot_directive_module.setup(app)


# Helper functions


def plot_directive_disabled() -> bool:
    """Return True if the plot directive should be skipped."""
    enable_plotdirective = os.environ.get("ENABLE_PLOTDIRECTIVE", None)
    return enable_plotdirective is None or enable_plotdirective == ""


def insert_placeholder_caption(state_machine):
    """Assuming the plot directive is disabled
    then insert placeholder text warning of this.
    :param state_machine: The current statemachine object
    """
    document = state_machine.document
    source = document.settings.env.docname
    # yapf: disable
    placeholder = [
        ".. figure:: /images/ImageNotFound.png",
        "   :class: screenshot",
        "   :width: 200px",
        "\n",
        "   Enable :plots: using DOCS_PLOTDIRECTIVE in CMake", ""
    ]
    # yapf: enable

    state_machine.insert_input(placeholder, source=source)
    return []
