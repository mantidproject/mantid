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
     - A referenceable link for use with Sphinx ":ref:`` tags", if this is
       the highest version of the algorithm being processed
     - A title
     - A screenshot of the algorithm
     - Table of contents

    If the algorithms is deprecated then a warning is inserted.
    """

    required_arguments, optional_arguments = 0, 0

    def run(self):
        """
        Called by Sphinx when the ..algorithm:: directive is encountered
        """
        self._track_algorithm()

        self._insert_reference_link()
        self._insert_pagetitle()
        imgpath = self._create_screenshot()
        self._insert_screenshot_link(imgpath)
        self._insert_toc()
        self._insert_deprecation_warning()

        self.commit_rst()
        return []

    def _track_algorithm(self):
        """
        Keep a track of the highest versions of algorithms encountered.
        The algorithm name and version are retrieved from the document name.
        See BaseDirective::set_algorithm_and_version()
        """
        env = self.state.document.settings.env
        if not hasattr(env, "algorithm"):
            env.algorithms = {}
        #endif

        name, version = self.algorithm_name(), self.algorithm_version()
        algorithms = env.algorithms
        if name in algorithms:
            prev_version = algorithms[name][1]
            if version > prev_version:
                algorithms[name][1] = version
        else:
            algorithms[name] = (name, version)

    def _insert_reference_link(self):
        """
        Outputs a reference to the top of the algorithm's rst
        of the form .. _AlgorithmName:
        """
        self.add_rst(".. _algorithm|%s:\n" % self.algorithm_name())

    def _insert_pagetitle(self):
        """
        Outputs a title for the page
        """
        self.add_rst(self.make_header(self.algorithm_name(), True))

    def _insert_toc(self):
        """
        Outputs a title for the page
        """
        self.add_rst(".. contents:: Table of Contents\n    :local:\n")

    def _create_screenshot(self):
        """
        Creates a screenshot for the named algorithm in an "images/screenshots"
        subdirectory of the currently processed document

        The file will be named "algorithmname-vX_dlg.png", e.g. Rebin-v1_dlg.png

        Returns:
          str: The full path to the created image
        """
        from mantiddoc.tools.screenshot import algorithm_screenshot

        env = self.state.document.settings.env
        screenshots_dir = self._screenshot_directory(env)
        if not os.path.exists(screenshots_dir):
            os.makedirs(screenshots_dir)

        try:
            imgpath = algorithm_screenshot(self.algorithm_name(), screenshots_dir, version=self.algorithm_version())
        except Exception, exc:
            env.warn(env.docname, "Unable to generate screenshot for '%s' - %s" % (algorithm_name, str(exc)))
            imgpath = os.path.join(screenshots_dir, "failed_dialog.png")

        return imgpath

    def _insert_screenshot_link(self, img_path):
        """
        Outputs an image link with a custom :class: style. The filename is
        extracted from the path given and then a link to /images/screenshots/filename.png
        is created. Sphinx handles copying the files over to the build directory
        and reformatting the links

        Args:
          img_path (str): The full path as on the filesystem to the image
        """
        format_str = ".. figure:: %s\n"\
                     "    :class: screenshot\n\n"\
                     "    %s\n\n"
        
        filename = os.path.split(img_path)[1]
        path = "/images/screenshots/" + filename
        caption = "A screenshot of the **" + self.algorithm_name() + "** dialog."

        self.add_rst(format_str % (path, caption))

    def _screenshot_directory(self, env):
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
        cfg_dir = env.app.srcdir
        return os.path.join(cfg_dir, "images", "screenshots")

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
            msg = DEPRECATE_USE_ALG_RE.sub(r"Use :ref:`algorithm|\1` instead.", msg)

        self.add_rst(".. warning:: %s" % msg)

############################################################################################################

def html_collect_pages(app):
    """
    Write out unversioned algorithm pages that redirect to the highest version of the algorithm
    """
    env = app.builder.env
    if not hasattr(env, "algorithms"):
        return # nothing to do

    template = REDIRECT_TEMPLATE

    algorithms = env.algorithms
    for name, highest_version in algorithms.itervalues():
        redirect_pagename = "algorithms/%s" % name
        target = "%s-v%d.html" % (name, highest_version)
        context = {"name" : name, "target" : target}
        yield (redirect_pagename, context, template)

############################################################################################################

def setup(app):
    """
    Setup the directives when the extension is activated

    Args:
      app: The main Sphinx application object
    """
    app.add_directive('algorithm', AlgorithmDirective)

    # connect event html collection to handler
    app.connect("html-collect-pages", html_collect_pages)

