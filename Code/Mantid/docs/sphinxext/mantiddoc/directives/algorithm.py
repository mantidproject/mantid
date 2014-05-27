from base import BaseDirective
import os

REDIRECT_TEMPLATE = "redirect.html"

class AlgorithmDirective(BaseDirective):

    """
    Adds a referenceable link for a given algorithm, a title,
    and a screenshot of the algorithm to an rst file.
    """

    required_arguments, optional_arguments = 0, 0

    def run(self):
        """
        Called by Sphinx when the ..algorithm:: directive is encountered
        """
        algorithm_name, version = self._algorithm_name_and_version()
        self._track_algorithm(algorithm_name, version)

        # Seperate methods for each unique piece of functionality.
        reference = self._make_reference_link(algorithm_name)
        title = self._make_header(algorithm_name, True)
        toc = self._make_local_toc()
        imgpath = self._create_screenshot(algorithm_name, version)
        screenshot = self._make_screenshot_link(algorithm_name, imgpath)

        return self._insert_rest(reference + title + screenshot + toc)

    def _track_algorithm(self, name, version):
        """
        Keep a track of the highest versions of algorithms encountered
        
        Arguments:
          name (str): Name of the algorithm
          version (int): Integer version number
        """
        env = self.state.document.settings.env
        if not hasattr(env, "algorithm"):
            env.algorithms = {}
        #endif
        algorithms = env.algorithms
        if name in algorithms:
            prev_version = algorithms[name][1]
            if version > prev_version:
                algorithms[name][1] = version
        else:
            algorithms[name] = (name, version)

    def _make_reference_link(self, algorithm_name):
        """
        Outputs a reference to the top of the algorithm's rst
        of the form .. _AlgorithmName:

        Args:
          algorithm_name (str): The name of the algorithm to reference.

        Returns:
          str: A ReST formatted reference.
        """
        return ".. _algorithm|%s:\n" % algorithm_name

    def _make_local_toc(self):
        return ".. contents:: Table of Contents\n    :local:\n"

    def _create_screenshot(self, algorithm_name, version):
        """
        Creates a screenshot for the named algorithm in an "images/screenshots"
        subdirectory of the currently processed document

        The file will be named "algorithmname-vX_dlg.png", e.g. Rebin-v1_dlg.png

        Args:
          algorithm_name (str): The name of the algorithm.
          version (int): The version of the algorithm

        Returns:
          str: The full path to the created image
        """
        from mantiddoc.tools.screenshot import algorithm_screenshot

        env = self.state.document.settings.env
        screenshots_dir = self._screenshot_directory(env)
        if not os.path.exists(screenshots_dir):
            os.makedirs(screenshots_dir)

        try:
            imgpath = algorithm_screenshot(algorithm_name, screenshots_dir, version=version)
        except Exception, exc:
            env.warn(env.docname, "Unable to generate screenshot for '%s' - %s" % (algorithm_name, str(exc)))
            imgpath = os.path.join(screenshots_dir, "failed_dialog.png")

        return imgpath

    def _make_screenshot_link(self, algorithm_name, img_path):
        """
        Outputs an image link with a custom :class: style. The filename is
        extracted from the path given and then a link to /images/screenshots/filename.png
        is created. Sphinx handles copying the files over to the build directory
        and reformatting the links

        Args:
          algorithm_name (str): The name of the algorithm that the screenshot represents
          img_path (str): The full path as on the filesystem to the image

        Returns:
          str: A ReST formatted reference.
        """
        format_str = ".. figure:: %s\n"\
                     "    :class: screenshot\n\n"\
                     "    %s\n\n"
        
        filename = os.path.split(img_path)[1]
        path = "/images/screenshots/" + filename
        caption = "A screenshot of the **" + algorithm_name + "** dialog."

        return format_str % (path, caption)

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

