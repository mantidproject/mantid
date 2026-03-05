# -*- mode: python -*-
# -*- coding: utf-8 -*-
# All configuration values have a default; values that are commented out
# serve to show the default.

import sys
from datetime import datetime

# Workaround module destruction order issues. If Qt is imported after
# mantid then any active Qt widgets are deleted before the mantid
# atexit handlers kick in. Some widgets, e.g. WorkspaceSelector,
# subscribe to mantid notifications and deleting the widget references leaves
# dangling references in the notification centre that cause a segfault
import qtpy.QtCore  # noqa: F401

import os

import mantid
from mantid.kernel import ConfigService

# If extensions (or modules to document with autodoc) are in another directory,
# add these directories to sys.path here. If the directory is relative to the
# documentation root, use os.path.abspath to make it absolute, like shown here.
sys.path.insert(0, os.path.abspath(os.path.join("..", "sphinxext")))

# -- General configuration ------------------------------------------------


def setup(app):
    """Called automatically by Sphinx when starting the build process"""
    app.add_css_file("custom.css")


# General information about the project.
project = "MantidProject"
copyright = f"{datetime.now().year}, Mantid"

# The full version, including alpha/beta/rc tags.
release = mantid.__version__
# The short X.Y version.
version = ".".join(release.split(".")[:2])

# The root toctree document.
root_doc = "index"


# Add any Sphinx extension module names here, as strings. They can be
# extensions coming with Sphinx (named 'sphinx.ext.*') or your custom
# ones.
extensions = [
    "mantid_sphinx_theme",
    # We use imgmath (LaTeX-rendered images) instead of mathjax (JavaScript)
    # to support QTextBrowser which doesn't support JavaScript
    "sphinx.ext.autodoc",
    "sphinx.ext.intersphinx",
    "sphinx.ext.doctest",
    # our custom directives
    "mantiddoc.directives.algorithm",
    "mantiddoc.directives.attributes",
    "mantiddoc.directives.categories",
    "mantiddoc.directives.amalgamate",
    "mantiddoc.directives.diagram",
    "mantiddoc.directives.interface",
    "mantiddoc.directives.plot_directive",
    "mantiddoc.directives.properties",
    "mantiddoc.directives.relatedalgorithms",
    "mantiddoc.directives.sourcelink",
    "mantiddoc.directives.summary",
    "mantiddoc.autodoc",
    "mantiddoc.doctest",
    # myst_parser enables markdown support
    "myst_parser",
]
# Math extension: imgmath (LaTeX images, for local QTextBrowser)
# Local builds default to imgmath via CMake. Online/published docs explicitly use mathjax.
mathext = os.environ.get("MATH_EXT", "sphinx.ext.imgmath")
extensions.append(mathext)

# MathJax configuration to:
# - define Angstrom symbol macro
# - enable polyfill for hasOwn (needed for some older browsers)
mathjax3_config = {"tex": {"macros": {"AA": r"\unicode{x212B}"}}, "startup": {"polyfillHasOwn": True}}

# Add any paths that contain templates here, relative to this directory.
templates_path = ["_templates"]

# The suffix of source filenames.
source_suffix = {
    ".rst": "restructuredtext",
    ".md": "markdown",
}

# MyST parser configuration
myst_enable_extensions = [
    "colon_fence",
    "deflist",
    "dollarmath",
    "fieldlist",
    "html_admonition",
    "html_image",
    "replacements",
    "smartquotes",
    "strikethrough",
    "substitution",
    "tasklist",
]

# Allow heading anchors
myst_heading_anchors = 3

# The name of the Pygments (syntax highlighting) style to use.
pygments_style = "sphinx"

# -- Options for doctest --------------------------------------------------

# Store certain config options so they can be restored to initial
# settings after each test.
mantid_init_config_keys = ("datasearch.directories", "defaultsave.directory", "default.facility", "default.instrument")

# With the default facility changed from ISIS to nothing (EMPTY),
# the following setting is put in place to avoid failure of tests
ConfigService.Instance().setString("default.facility", "ISIS")

mantid_config_reset = ["_cfg['{0}'] = '{1}'".format(k, ConfigService.Instance()[k]) for k in mantid_init_config_keys]
mantid_config_reset = "\n".join(mantid_config_reset)

# Run this before each test is executed
doctest_global_setup = """
from mantid.simpleapi import *
from mantid.kernel import ConfigService as _cfg
{0}

# doctest.py examines the global scope for future imports
# and uses the sames ones in the executing scope. We don't want the
# Python-3 style behaviour in the docs yet.
try:
    del print_function
    del absolute_import
    del division
    del unicode_literals
except NameError:
    pass
""".format(mantid_config_reset)

# Run this after each test group has executed
doctest_global_cleanup = """
import time
from mantid.api import FrameworkManager
from mantid.kernel import MemoryStats

FrameworkManager.Instance().clear()
if MemoryStats().getFreeRatio() < 0.75:
    # sleep for short period to allow memory to be freed
    time.sleep(2)
"""

# -- Options for pngmath --------------------------------------------------

# Load the preview package into latex
pngmath_latex_preamble = r"\usepackage[active]{preview}"

# Ensures that the vertical alignment of equations is correct.
# See http://sphinx-doc.org/ext/math.html#confval-pngmath_use_preview
pngmath_use_preview = True

# -- HTML output ----------------------------------------------------
# Options for HTML output. For a full list and more details see the documentation:
# http://www.sphinx-doc.org/en/master/usage/configuration.html#options-for-html-output

# Use our custom Mantid Sphinx theme (extends https://pydata-sphinx-theme.readthedocs.io)
html_theme = "mantid_sphinx_theme"

# -- Options for Epub output ---------------------------------------------------
# This flag determines if a toc entry is inserted again at the beginning of its nested toc listing.
# This allows easier navigation to the top of a chapter, but can be confusing because it mixes entries of different depth in one list.
# The default value is True.
epub_tocdup = True

# This setting control the scope of the epub table of contents
epub_tocscope = "includehidden"

# The author of the document. This is put in the Dublin Core metadata. The default value is 'unknown'.
epub_author = "The Mantid Project"

# The publisher of the document. This is put in the Dublin Core metadata. You may use any sensible string, e.g. the project homepage.
epub_publisher = "The Mantid Project"

# An identifier for the document. This is put in the Dublin Core metadata.
# For published documents this is the ISBN number, but you can also use an alternative scheme, e.g. the project homepage.
# The default value is 'unknown'.
epub_identifier = "www.mantidproject.org"

# The publication scheme for the epub_identifier. This is put in the Dublin Core metadata.
# For published books the scheme is 'ISBN'. If you use the project homepage, 'URL' seems reasonable.
# The default value is 'unknown'.
epub_scheme = "URL"

# A unique identifier for the document. This is put in the Dublin Core metadata. You may use a random string.
# The default value is 'unknown'.
epub_uid = "Mantid Reference: " + version

# -- Link to other projects ----------------------------------------------------

intersphinx_mapping = {
    "h5py": ("https://docs.h5py.org/en/stable/", None),
    "matplotlib": ("https://matplotlib.org/stable/", None),
    "numpy": ("https://numpy.org/doc/stable/", None),
    "python": ("https://docs.python.org/3/", None),
    "SciPy": ("https://docs.scipy.org/doc/scipy/", None),
    "pandas": ("https://pandas.pydata.org/pandas-docs/stable", None),
    "pystog": ("https://pystog.readthedocs.io/en/latest/", None),
    "mantid-dev": ("https://developer.mantidproject.org/", None),
}

# Suppress build warnings of the type:
# "WARNING: document isn't included in any toctree"
# for individual release notes files.
exclude_patterns = [
    "images/README.md",
    "release/templates/*.rst",
    "release/**/Bugfixes/*.rst",
    "release/**/New_features/*.rst",
    "release/**/Used/*.rst",
    "release/**/Removed/*.rst",
    "release/**/Deprecated/*.rst",
]
