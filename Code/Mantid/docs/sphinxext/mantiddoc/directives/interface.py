from base import BaseDirective
from sphinx.locale import _
import os

class InterfaceDirective(BaseDirective):

    """
    Adds a screenshot of the custom interface

    It requires a SCREENSHOTS_DIR environment variable to be set to the
    directory where a screenshot should be generated. If it is not set then
    a RuntimeError occurs
    """

    required_arguments, optional_arguments = 1, 0
    option_spec = {"widget":str, "align":str, "width":int}

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
        picture = self._create_screenshot(widget_name=self.options.get("widget", None))
        self._insert_screenshot_link(picture, align=self.options.get("align", None),
                                     width=self.options.get("width", None))
        return []

    def interface_name(self):
        return self.arguments[0]

    def _create_screenshot(self, widget_name = None):
        """
        Creates a screenshot for the named interface in the "images/screenshots"
        subdirectory.

        The file will be named "interfacename_interface.png", e.g. "ISIS_Reflectometry_interface.png"

        Returns:
          screenshot: A mantiddoc.tools.Screenshot object
        """
        try:
            screenshots_dir = self._screenshot_directory()
        except RuntimeError:
            return None

        # Generate image
        from mantiddoc.tools.screenshot import custominterface_screenshot
        if not os.path.exists(screenshots_dir):
            os.makedirs(screenshots_dir)

        try:
            picture = custominterface_screenshot(self.interface_name(), screenshots_dir, widget_name = widget_name)
        except RuntimeError, exc:
            env = self.state.document.settings.env
            env.warn(env.docname, "Unable to generate screenshot for '%s' - %s" % (self.interface_name(), str(exc)))
            picture = None

        return picture

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
        format_str = ".. figure:: %s\n"\
                     "   :class: screenshot\n"\
                     "   :width: %dpx"

        if align != None:
            format_str += "\n   :align: " + align

        format_str += "\n\n"

        # Sphinx assumes that an absolute path is actually relative to the directory containing the
        # conf.py file and a relative path is relative to the directory where the current rst file
        # is located.
        if picture:
            screenshots_dir, filename = os.path.split(picture.imgpath)

            if width is None:
                # No width provided, use screenshot width
                width = picture.width

            # relative path to image
            rel_path = os.path.relpath(screenshots_dir, env.srcdir)
            # This is a href link so is expected to be in unix style
            rel_path = rel_path.replace("\\","/")
            # stick a "/" as the first character so Sphinx computes relative location from source directory
            path = "/" + rel_path + "/" + filename
        else:
            # use stock not found image
            path = "/images/ImageNotFound.png"
            width = 200

        self.add_rst(format_str % (path, width))

    def _screenshot_directory(self):
        """
        Returns a full path where the screenshots should be generated. They are
        put in a screenshots subdirectory of the main images directory in the source
        tree. Sphinx then handles copying them to the final location

        Arguments:
          env (BuildEnvironment): Allows access to find the source directory

        Returns:
          str: A string containing a path to where the screenshots should be created. This will
          be a filesystem path
        """
        try:
            return os.environ["SCREENSHOTS_DIR"]
        except:
            raise RuntimeError("The '.. interface::' directive requires a SCREENSHOTS_DIR environment variable to be set.")

#------------------------------------------------------------------------------------------------------------

def setup(app):
    """
    Setup the directives when the extension is activated

    Args:
      app: The main Sphinx application object
    """
    app.add_directive('interface', InterfaceDirective)
