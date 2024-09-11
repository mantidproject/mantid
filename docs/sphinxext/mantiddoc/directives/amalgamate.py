# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantiddoc.directives.base import BaseDirective

import os


class CompilationDirective(BaseDirective):
    """
    Obtains the summary for a given algorithm based on its name.
    """

    required_arguments, optional_arguments = 1, 1

    def run(self):
        """
        The main entry point that docutils calls.
        It calls self.execute() to do the main work.
        Derived classes should override execute() and insert
        whatever rst they require with self.add_rst()
        """
        nodes = self.execute()
        if self.rst_lines is not None:
            self.commit_rst()
        return nodes

    def execute(self):
        """
        Called by Sphinx when the ..compiler:: directive is encountered.
        """
        script_dir = self.getPath()
        if os.path.exists(script_dir):
            for file in os.listdir(script_dir):
                if file.endswith(".rst"):
                    with open(script_dir + "/" + file) as f:
                        contents = f.read()
                        self.add_rst(contents)

        return []

    def getPath(self):
        # the location of documentation
        source_dir = self.state.document.settings.env.srcdir
        # the location of the release notes for this version
        release_dir = self.source().rsplit("/", 1)[0]
        # argument provided to amalgamate directive
        args = self.arguments[0]
        if args[0] != "/":
            args = "/" + args
        path_to_notes = release_dir + args
        return os.path.abspath(os.path.join(source_dir, path_to_notes))


def setup(app):
    """
    Setup the directives when the extension is activated

    Args:
      app: The main Sphinx application object
    """
    app.add_directive("amalgamate", CompilationDirective)
