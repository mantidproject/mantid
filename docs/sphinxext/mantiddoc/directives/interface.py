# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantiddoc.directives.base import BaseDirective  # pylint: disable=unused-import
from pathlib import Path


class InterfaceDirective(BaseDirective):
    """
    Adds a screenshot of the custom interface

    It requires a SCREENSHOTS_DIR environment variable to be set to the
    directory where a screenshot should be generated. If it is not set then
    a RuntimeError occurs
    """

    required_arguments, optional_arguments = 1, 0
    option_spec = {"widget": str, "align": str, "width": int}

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
        """
        Called by Sphinx when the ..interface:: directive is encountered
        """
        try:
            picture = self._create_screenshot(widget_name=self.options.get("widget", None))
        except RuntimeError:
            picture = None
        self._insert_screenshot_link(picture, align=self.options.get("align", None), width=self.options.get("width", None))
        return []

    def interface_name(self):
        return self.arguments[0]

    def _create_screenshot(self, widget_name=None):
        """
        Creates a screenshot for the named interface in the "images/screenshots"
        subdirectory.

        The file will be named "interfacename_interface.png", e.g. "ISIS_Reflectometry_interface.png"

        Returns:
          screenshot: A mantiddoc.tools.Screenshot object
        """
        screenshots_dir = self.screenshots_dir
        if screenshots_dir is None:
            return None

        # Generate image
        from mantiddoc.tools.screenshot import custominterface_screenshot

        return custominterface_screenshot(self.interface_name(), screenshots_dir, widget_name=widget_name)

    def _insert_screenshot_link(self, picture, align=None, width=None):
        """
        Outputs an image link with a custom :class: style. The filename is
        extracted from the path given and then a relative link to the
        directory specified by the SCREENSHOTS_DIR environment variable from
        the root source directory is formed.

        Args:
          picture (Screenshot): A Screenshot object
          align: The alignment to use, None for block, "left" or "right" for flowing
          width: The width to use (in pixels, defaults to width of screenshot)
        """
        env = self.state.document.settings.env

        # Sphinx assumes that an absolute path is actually relative to the directory containing the
        # conf.py file and a relative path is relative to the directory where the current rst file
        # is located.
        if picture:
            picture_imgpath = Path(picture.imgpath)
            screenshots_dir = picture_imgpath.parent
            filename = picture_imgpath.name

            if width is None:
                # No width provided, use screenshot width
                width = picture.width

            # relative path to image
            rel_path = screenshots_dir.relative_to(env.srcdir)
            # stick a "/" as the first character so Sphinx computes relative location from source directory
            path = Path("/").joinpath(rel_path).joinpath(filename)
            caption = ""
        else:
            # use stock not found image
            path = Path("/images/ImageNotFound.png")
            width = 200
            caption = "Enable screenshots using DOCS_SCREENSHOTS in CMake"

        if align is not None:
            self.add_rst_list(
                [f".. figure:: {path}", "   :class: screenshot", f"   :width: {width}px", f"   :align: {align}\n", "", f"   {caption}", ""]
            )
        else:
            self.add_rst_list([f".. figure:: {path}", "   :class: screenshot", f"   :width: {width}px\n", "", f"   {caption}", ""])


# ------------------------------------------------------------------------------------------------------------


def setup(app):
    """
    Setup the directives when the extension is activated

    Args:
      app: The main Sphinx application object
    """
    app.add_directive("interface", InterfaceDirective)
