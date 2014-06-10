from base import BaseDirective
from docutils import nodes
from sphinx.locale import _
from sphinx.util.compat import make_admonition
import os
import re

REDIRECT_TEMPLATE = "redirect.html"

DEPRECATE_USE_ALG_RE = re.compile(r'Use\s([A-Z][a-zA-Z0-9]+)\sinstead')

#--------------------------------------------------------------------------
class AlgorithmDirective(BaseDirective):

    """
    Inserts details of an algorithm by querying Mantid

    Adds:
     - A referenceable link for use with Sphinx ":ref:`` tags". If this is
       the highest version of the algorithm being processed then a both
       a versioned link is created and a non-versioned link
     - A title
     - A screenshot of the algorithm
     - Table of contents

    If the algorithms is deprecated then a warning is inserted.

    It requires a SCREENSHOTS_DIR environment variable to be set to the
    directory where a screenshot should be generated. If it is not set then
    a RuntimeError occurs
    """

    required_arguments, optional_arguments = 0, 0

    def run(self):
        """
        Called by Sphinx when the ..algorithm:: directive is encountered
        """
        self._insert_pagetitle()
        imgpath = self._create_screenshot()
        self._insert_screenshot_link(imgpath)
        self._insert_toc()
        self._insert_deprecation_warning()

        self.commit_rst()
        return []

    def _insert_pagetitle(self):
        """
        Outputs a reference to the top of the algorithm's rst
        of the form ".. _algm-AlgorithmName-vVersion:", so that
        the page can be referenced using 
        :ref:`algm-AlgorithmName-version`. If this is the highest 
        version then it outputs a reference ".. _algm-AlgorithmName: instead
        
        It then outputs a title for the page
        """
        from mantid.api import AlgorithmFactory

        alg_name = self.algorithm_name()
        version = self.algorithm_version()

        # page reference must come directly before the title if it wants
        # to be referenced without defining the link text. Here we put the
        # specific version one first so that it always must be referenced
        # using the full link text ":ref`LinkText <algm-AlgorithmName-vX>`:"
        self.add_rst(".. _algm-%s-v%d:\n\n" % (alg_name, version))
        if AlgorithmFactory.highestVersion(alg_name) == version:
            self.add_rst(".. _algm-%s:\n\n" % alg_name)

        # title
        title = "%s v%d" % (alg_name, version)
        self.add_rst(self.make_header(title, True))

    def _insert_toc(self):
        """
        Outputs a title for the page
        """
        self.add_rst(".. contents:: Table of Contents\n    :local:\n")

    def _create_screenshot(self):
        """
        Creates a screenshot for the named algorithm in the "images/screenshots"
        subdirectory.

        The file will be named "algorithmname-vX_dlg.png", e.g. Rebin-v1_dlg.png

        Returns:
          str: The full path to the created image
        """
        notfoundimage = "/images/ImageNotFound.png"
        try:
            screenshots_dir = self._screenshot_directory()
        except RuntimeError:
            return notfoundimage

        # Generate image
        from mantiddoc.tools.screenshot import algorithm_screenshot
        if not os.path.exists(screenshots_dir):
            os.makedirs(screenshots_dir)

        try:
            imgpath = algorithm_screenshot(self.algorithm_name(), screenshots_dir, version=self.algorithm_version())
        except Exception, exc:
            env = self.state.document.settings.env
            env.warn(env.docname, "Unable to generate screenshot for '%s' - %s" % (algorithm_name, str(exc)))
            imgpath = notfoundimage

        return imgpath

    def _insert_screenshot_link(self, img_path):
        """
        Outputs an image link with a custom :class: style. The filename is
        extracted from the path given and then a relative link to the
        directory specified by the SCREENSHOTS_DIR environment variable from
        the root source directory is formed.

        Args:
          img_path (str): The full path as on the filesystem to the image
        """
        env = self.state.document.settings.env
        format_str = ".. figure:: %s\n"\
                     "    :class: screenshot\n"\
                     "    :align: right\n"\
                     "    :width: 400px\n\n"\
                     "    %s\n\n"
        
        # Sphinx assumes that an absolute path is actually relative to the directory containing the
        # conf.py file and a relative path is relative to the directory where the current rst file
        # is located.

        filename = os.path.split(img_path)[1]
        cfgdir = env.srcdir

        try:
            screenshots_dir = self._screenshot_directory()
            rel_path = os.path.relpath(screenshots_dir, cfgdir)
            # This is a href link so is expected to be in unix style
            rel_path = rel_path.replace("\\","/")
            # stick a "/" as the first character so Sphinx computes relative location from source directory
            path = "/" + rel_path + "/" + filename
        except RuntimeError:
            # Use path as it is
            path = img_path

        caption = "A screenshot of the **" + self.algorithm_name() + "** dialog."
        self.add_rst(format_str % (path, caption))

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
            raise RuntimeError("The '.. algorithm::' directive requires a SCREENSHOTS_DIR environment variable to be set.")

    def _insert_deprecation_warning(self):
        """
        If the algorithm version is deprecated then construct a warning message
        """
        from mantid.api import DeprecatedAlgorithmChecker

        checker = DeprecatedAlgorithmChecker(self.algorithm_name(), self.algorithm_version())
        msg = checker.isDeprecated()
        if len(msg) == 0:
            return

        # Check for message to use another algorithm an insert a link
        match = DEPRECATE_USE_ALG_RE.search(msg)
        if match is not None and len(match.groups()) == 1:
            name = match.group(0)
            msg = DEPRECATE_USE_ALG_RE.sub(r"Use :ref:`algm-\1` instead.", msg)

        self.add_rst(".. warning:: %s" % msg)


#------------------------------------------------------------------------------------------------------------

def html_collect_pages(app):
    """
    Write out unversioned algorithm pages that redirect to the highest version of the algorithm
    """
    from mantid.api import AlgorithmFactory

    template = REDIRECT_TEMPLATE
    all_algs = AlgorithmFactory.getRegisteredAlgorithms(True)

    for name, versions in all_algs.iteritems():
        redirect_pagename = "algorithms/%s" % name
        versions.sort()
        highest_version = versions[-1]
        target = "%s-v%d.html" % (name, highest_version)
        context = {"name" : name, "target" : target}
        yield (redirect_pagename, context, template)

#------------------------------------------------------------------------------------------------------------

def setup(app):
    """
    Setup the directives when the extension is activated

    Args:
      app: The main Sphinx application object
    """
    app.add_directive('algorithm', AlgorithmDirective)

    # connect event html collection to handler
    app.connect("html-collect-pages", html_collect_pages)
