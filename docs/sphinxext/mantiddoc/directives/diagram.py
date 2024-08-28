# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantiddoc.directives.base import BaseDirective  # pylint: disable=unused-import
from sphinx.locale import _  # noqa: F401
import os
from string import Template
import subprocess
from pathlib import Path

STYLE = dict()

STYLE["global_style"] = """
fontname = Helvetica
labelloc = t
ordering = out
node[fontname="Helvetica", style = filled]
edge[fontname="Helvetica"]
"""

STYLE["param_style"] = "node[fillcolor = khaki, shape = oval]"
STYLE["decision_style"] = "node[fillcolor = limegreen, shape = diamond]"
STYLE["algorithm_style"] = 'node[style = "rounded,filled", fillcolor = lightskyblue, shape = rectangle]'
STYLE["process_style"] = "node[fillcolor = lightseagreen, shape = rectangle]"
STYLE["value_style"] = 'node[fontname = "Times-Roman", fillcolor = grey, shape = parallelogram]'


class DiagramDirective(BaseDirective):
    """
    Adds a diagram from a dot source file

    It requires DIAGRAMS_DIR and DOT_EXECUTABLE environment variables to be set to the
    directory where a diagram should be generated and where the dot program may be found,
    respectively. If they are not set then a RuntimeError occurs
    """

    required_arguments, optional_arguments = 1, 0

    @property
    def diagrams_dir(self):
        """Return the directory generated diagrams should be stored in or
        None if they should not be created
        """
        diagrams_dir = os.environ.get("DIAGRAMS_DIR", None)
        if diagrams_dir is None or diagrams_dir == "":
            return None
        else:
            return Path(diagrams_dir)

    def run(self):
        """
        The main entry point that docutils calls.
        It calls self.execute to do the main work.
        Derived classes should override execute() and insert
        whatever rst they require with self.add_rst()
        """
        nodes = self.execute()
        if self.rst_lines is not None:
            self.commit_rst()
        return nodes

    def execute(self):
        env = self.state.document.settings.env
        diagrams_dir = self.diagrams_dir
        if diagrams_dir is None:
            self.add_rst(".. figure:: /images/ImageNotFound.png\n\n" "    diagram generation was disabled")
            return []

        try:
            dot_executable = os.environ["DOT_EXECUTABLE"]
        except KeyError:
            self.add_rst(".. figure:: /images/ImageNotFound.png\n\n" "    graphviz not found - diagram could not be rendered.")
            return []

        # Make sure we have an output directory
        if not os.path.exists(diagrams_dir):
            os.makedirs(diagrams_dir)
        diagram_name = self.arguments[0]
        if diagram_name[-4:] != ".dot":
            raise RuntimeError("Diagrams need to be referred to by their filename, including '.dot' extension.")

        in_path = Path(env.srcdir, "diagrams", diagram_name)
        out_path = Path(diagrams_dir, diagram_name[:-4] + ".svg")

        # Generate the diagram
        try:
            in_src = open(in_path, "r").read()
        except Exception:
            raise RuntimeError("Cannot find dot-file: '" + diagram_name + "' in '" + Path(env.srcdir, "diagrams"))

        out_src = Template(in_src).substitute(STYLE)
        out_src = out_src.encode()
        gviz = subprocess.Popen([dot_executable, "-Tsvg", "-o", out_path], stdin=subprocess.PIPE)
        gviz.communicate(input=out_src)
        gviz.wait()

        # relative path to image, in unix style
        rel_path = out_path.relative_to(env.srcdir)

        self.add_rst(".. image:: /" + rel_path + "\n\n")

        return []


# ------------------------------------------------------------------------------------------------------------


def setup(app):
    """
    Setup the directives when the extension is activated

    Args:
      app: The main Sphinx application object
    """
    app.add_directive("diagram", DiagramDirective)
