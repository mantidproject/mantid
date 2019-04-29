# -*- mode: python -*-
# -*- coding: utf-8 -*-
# All configuration values have a default; values that are commented out
# serve to show the default.

import sys
import os
from sphinx import __version__ as sphinx_version
import sphinx_bootstrap_theme # checked at cmake time
import mantid
from mantid.kernel import ConfigService
from distutils.version import LooseVersion

# If extensions (or modules to document with autodoc) are in another directory,
# add these directories to sys.path here. If the directory is relative to the
# documentation root, use os.path.abspath to make it absolute, like shown here.
sys.path.insert(0, os.path.abspath(os.path.join('..', 'sphinxext')))

# -- General configuration ------------------------------------------------

if LooseVersion(sphinx_version) > LooseVersion("1.6"):
    def setup(app):
        """Called automatically by Sphinx when starting the build process
        """
        app.add_stylesheet("custom.css")


# Add any Sphinx extension module names here, as strings. They can be
# extensions coming with Sphinx (named 'sphinx.ext.*') or your custom
# ones.
extensions = [
     # we use pngmath over mathjax so that the the offline help isn't reliant on
     # anything external and we don't need to include the large mathjax package
    'sphinx.ext.autodoc',
    'sphinx.ext.intersphinx',
    'sphinx.ext.doctest',
    'mantiddoc.directives',
    'mantiddoc.autodoc',
    'mantiddoc.doctest',
    'matplotlib.sphinxext.plot_directive'
]
if LooseVersion(sphinx_version) > LooseVersion("1.8"):
    extensions.append('sphinx.ext.imgmath')
else:
    extensions.append('sphinx.ext.pngmath')

# Add any paths that contain templates here, relative to this directory.
templates_path = ['_templates']

# The suffix of source filenames.
source_suffix = '.rst'

# The master toctree document.
master_doc = 'index'

# General information about the project.
project = u'MantidProject'
copyright = u'2015, Mantid'

# The version info for the project you're documenting, acts as replacement for
# |version| and |release|, also used in various other places throughout the
# built documents.
#
version_str = mantid.__version__
# The short X.Y version.
version = ".".join(version_str.split(".")[:2])
# The full version, including alpha/beta/rc tags.
release = version_str

# The name of the Pygments (syntax highlighting) style to use.
pygments_style = 'sphinx'

# -- Options for doctest --------------------------------------------------

# Store certain config options so they can be restored to initial
# settings after each test.
mantid_init_config_keys = ('datasearch.directories', 'defaultsave.directory',
                           'default.facility', 'default.instrument')
mantid_config_reset = ["_cfg['{0}'] = '{1}'".format(k, ConfigService.Instance()[k]) for k in mantid_init_config_keys]
mantid_config_reset = '\n'.join(mantid_config_reset)

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
pngmath_latex_preamble=r'\usepackage[active]{preview}'

# Ensures that the vertical alignment of equations is correct.
# See http://sphinx-doc.org/ext/math.html#confval-pngmath_use_preview
pngmath_use_preview = True

# -- HTML output ----------------------------------------------------

# The theme to use for HTML and HTML Help pages.  See the documentation for
# a list of builtin themes.
html_theme = 'bootstrap'

# Add any paths that contain custom themes here, relative to this directory.
html_theme_path = sphinx_bootstrap_theme.get_html_theme_path()

# The "title" for HTML documentation generated with Sphinx' templates. This is appended to the <title> tag of individual pages
# and used in the navigation bar as the "topmost" element.
html_title = ""

# The name of an image file (relative to this directory) to place at the top
# of the sidebar.
html_logo = os.path.relpath(os.path.join('..', '..', 'images', 'Mantid_Logo_Transparent.png'))

# Add any paths that contain custom static files (such as style sheets) here,
# relative to this directory. They are copied after the builtin static files,
# so a file named "default.css" will overwrite the builtin "default.css".
html_static_path = ['_static']

# Add any extra paths that contain custom files (such as robots.txt or
# .htaccess) here, relative to this directory. These files are copied
# directly to the root of the documentation.
#html_extra_path = []

# If true, Smart Quotes will be used to convert quotes and dashes to
# typographically correct entities.
if sphinx_version < "1.7":
    html_use_smartypants = True
else:
    smartquotes = True

# Hide the Sphinx usage as we reference it on github instead.
html_show_sphinx = False

# If true, "(C) Copyright ..." is shown in the HTML footer. Default is True.
html_show_copyright = False

# Add the last updated information to the bottom of pages.
html_last_updated_fmt = '%Y-%m-%d'

# -- Options for Epub output ---------------------------------------------------
# This flag determines if a toc entry is inserted again at the beginning of its nested toc listing.
# This allows easier navigation to the top of a chapter, but can be confusing because it mixes entries of different depth in one list.
# The default value is True.
epub_tocdup = True

#This setting control the scope of the epub table of contents
epub_tocscope = 'includehidden'

#The author of the document. This is put in the Dublin Core metadata. The default value is 'unknown'.
epub_author = "The Mantid Project"

#The publisher of the document. This is put in the Dublin Core metadata. You may use any sensible string, e.g. the project homepage.
epub_publisher = "The Mantid Project"

#An identifier for the document. This is put in the Dublin Core metadata.
#For published documents this is the ISBN number, but you can also use an alternative scheme, e.g. the project homepage.
#The default value is 'unknown'.
epub_identifier = "www.mantidproject.org"

#The publication scheme for the epub_identifier. This is put in the Dublin Core metadata.
#For published books the scheme is 'ISBN'. If you use the project homepage, 'URL' seems reasonable.
#The default value is 'unknown'.
epub_scheme='URL'

#A unique identifier for the document. This is put in the Dublin Core metadata. You may use a random string.
#The default value is 'unknown'.
epub_uid = "Mantid Reference: " + version

# -- Options for selected builder output ---------------------------------------
# Default is to use standard HTML theme unless the qthelp tag is specified
html_theme_cfg = 'conf-qthelp.py' if 'qthelp' in [k.strip() for k in tags.tags] else 'conf-html.py'
# Python 3 removed execfile...
exec(compile(open(html_theme_cfg).read(), html_theme_cfg, 'exec'))

# -- Link to other projects ----------------------------------------------------

intersphinx_mapping = {
    'h5py': ('http://docs.h5py.org/en/latest/', None),
    'matplotlib': ('http://matplotlib.org', None),
    'numpy': ('https://docs.scipy.org/doc/numpy/', None),
    'python': ('https://docs.python.org/3/', None),
    'SciPy': ('http://docs.scipy.org/doc/scipy/reference', None),
    'pandas': ('http://pandas.pydata.org/pandas-docs/stable', None),
    'pystog': ('https://pystog.readthedocs.io/en/latest/', None)
}
