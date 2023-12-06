# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantiddoc.directives.base import AlgorithmBaseDirective  # pylint: disable=unused-import
import re

REDIRECT_TEMPLATE = "redirect.html"

DEPRECATE_USE_ALG_RE = re.compile(r"Use\s(([A-Z][a-zA-Z0-9]+)\s(version ([0-9])+)?)\s*instead.")

# --------------------------------------------------------------------------


class AlgorithmDirective(AlgorithmBaseDirective):
    """
    Inserts details of an algorithm by querying Mantid

    Adds:
     - A referenceable link for use with Sphinx ":ref:`` tags". If this is
       the highest version of the algorithm being processed then a both
       a versioned link is created and a non-versioned link
     - A title
     - Table of contents

    If the algorithms is deprecated then a warning is inserted.
    """

    required_arguments, optional_arguments = 0, 0

    def execute(self):
        """
        Called by Sphinx when the ..algorithm:: directive is encountered
        """
        self._insert_pagetitle()
        self._insert_toc()
        self._insert_deprecation_warning()

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
        self.add_rst("\n.. index:: %s-v%d\n\n" % (alg_name, version))

    def _insert_toc(self):
        """
        Outputs a title for the page
        """
        self.add_rst(".. contents:: Table of Contents\n    :local:\n")

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
        if match is not None and len(match.groups()) == 4:
            link_text = match.group(1)
            alg_name = match.group(2)
            version = match.group(4)
            link = "algm-%s%s"
            if version is not None:
                link = link % (alg_name, "-v" + str(version))
            else:
                link = link % (alg_name, "")
            replacement = "Use :ref:`%s <%s>` instead." % (link_text, link)
            msg = DEPRECATE_USE_ALG_RE.sub(replacement, msg)
        # endif

        self.add_rst(".. warning:: %s" % msg)


# ------------------------------------------------------------------------------------------------------------


def html_collect_pages(dummy_app):
    """
    Write out unversioned algorithm pages that redirect to the highest version of the algorithm
    """
    from mantid.api import AlgorithmFactory

    template = REDIRECT_TEMPLATE
    all_algs = AlgorithmFactory.getRegisteredAlgorithms(True)

    for name, versions in all_algs.items():
        redirect_pagename = "algorithms/%s" % name
        versions.sort()
        highest_version = versions[-1]
        target = "%s-v%d.html" % (name, highest_version)
        context = {"name": name, "target": target}
        yield (redirect_pagename, context, template)


# ------------------------------------------------------------------------------------------------------------


def setup(app):
    """
    Setup the directives when the extension is activated

    Args:
      app: The main Sphinx application object
    """
    from mantid.api import FrameworkManager

    app.add_directive("algorithm", AlgorithmDirective)
    # connect event html collection to handler
    app.connect("html-collect-pages", html_collect_pages)

    # start framework manager to load plugins once
    FrameworkManager.Instance()
