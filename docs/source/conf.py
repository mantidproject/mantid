# -*- mode: python -*-
# -*- coding: utf-8 -*-
# All configuration values have a default; values that are commented out
# serve to show the default.

import sys

# Workaround module destruction order issues. If Qt is imported after
# mantid then any active Qt widgets are deleted before the mantid
# atexit handlers kick in. Some widgets, e.g. WorkspaceSelector,
# subscribe to mantid notifications and deleting the widget references leaves
# dangling references in the notification centre that cause a segfault
import qtpy.QtCore  # noqa: F401

import os

import mantid
from mantid.kernel import ConfigService
import sphinx_bootstrap_theme

# If extensions (or modules to document with autodoc) are in another directory,
# add these directories to sys.path here. If the directory is relative to the
# documentation root, use os.path.abspath to make it absolute, like shown here.
sys.path.insert(0, os.path.abspath(os.path.join("..", "sphinxext")))

# -- General configuration ------------------------------------------------


def setup(app):
    """Called automatically by Sphinx when starting the build process"""
    app.add_css_file("custom.css")


# Add any Sphinx extension module names here, as strings. They can be
# extensions coming with Sphinx (named 'sphinx.ext.*') or your custom
# ones.
extensions = [
    # we use pngmath over mathjax so that the offline help isn't reliant on
    # anything external and we don't need to include the large mathjax package
    "sphinx.ext.autodoc",
    "sphinx.ext.intersphinx",
    "sphinx.ext.doctest",
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
]
# Deal with math extension. Can be overridden with MATH_EXT environment variable
mathext = os.environ.get("MATH_EXT", "sphinx.ext.imgmath")
extensions.append(mathext)

# Add any paths that contain templates here, relative to this directory.
templates_path = ["_templates"]

# The suffix of source filenames.
source_suffix = ".rst"

# The root toctree document.
root_doc = "index"

# General information about the project.
project = "MantidProject"
copyright = "2015, Mantid"

# The full version, including alpha/beta/rc tags.
release = mantid.__version__
# The short X.Y version.
version = ".".join(release.split(".")[:2])

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

# Use legacy numpy printing. This fix is made to keep doctests functional.
# TODO: remove this workaround once minimal required numpy is set to 1.14.0
import numpy as np

try:
    np.set_printoptions(legacy='1.13')
except TypeError:
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

# The theme to use for HTML and HTML Help pages.  See the documentation for
# a list of builtin themes.
qthelp_theme = "bootstrap"
html_theme = "bootstrap"

# Add any paths that contain custom themes here, relative to this directory.
html_theme_path = sphinx_bootstrap_theme.get_html_theme_path()

# The "title" for HTML documentation generated with Sphinx' templates. This is appended to the <title> tag of individual pages
# and used in the navigation bar as the "topmost" element.
html_title = ""

# The name of an image file (relative to this directory) to place at the top
# of the sidebar.
html_logo = os.path.relpath(os.path.join("..", "..", "images", "Mantid_Logo_Transparent.png"))

# Add any paths that contain custom static files (such as style sheets) here,
# relative to this directory. They are copied after the builtin static files,
# so a file named "default.css" will overwrite the builtin "default.css".
html_static_path = ["_static"]

# Add any extra paths that contain custom files (such as robots.txt or
# .htaccess) here, relative to this directory. These files are copied
# directly to the root of the documentation.
# html_extra_path = []

# If true, Smart Quotes will be used to convert quotes and dashes to
# typographically correct entities.
smartquotes = True

# Hide the Sphinx usage as we reference it on github instead.
html_show_sphinx = False

# If true, "(C) Copyright ..." is shown in the HTML footer. Default is True.
html_show_copyright = False

# Do not show last updated information in the HTML footer.
html_last_updated_fmt = None

# Hide the navigation sidebar, we use a table of contents instead.
html_sidebars = {"**": []}

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

# -- Options for selected builder output ---------------------------------------
# Default is to use standard HTML theme unless the qthelp tag is specified
html_theme_cfg = "conf-qthelp.py" if "qthelp" in [k.strip() for k in tags.tags] else "conf-html.py"
# Python 3 removed execfile...
exec(compile(open(html_theme_cfg).read(), html_theme_cfg, "exec"))

# -- Link to other projects ----------------------------------------------------

intersphinx_mapping = {
    "h5py": ("https://h5py.readthedocs.io/en/stable/", None),
    "matplotlib": ("https://matplotlib.org", None),
    "numpy": ("https://numpy.org/doc/stable/", None),
    "python": ("https://docs.python.org/3/", None),
    "SciPy": ("https://docs.scipy.org/doc/scipy/reference", None),
    "pandas": ("https://pandas.pydata.org/pandas-docs/stable", None),
    "pystog": ("https://pystog.readthedocs.io/en/latest/", None),
    "mantid-dev": ("https://developer.mantidproject.org/", None),
}

# Suppress build warnings of the type:
# "WARNING: document isn't included in any toctree"
# for individual release notes files.
exclude_patterns = ["release/templates/*.rst", "release/**/Bugfixes/*.rst", "release/**/New_features/*.rst", "release/**/Used/*.rst"]
