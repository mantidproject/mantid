# -*- mode: python -*-
# -*- coding: utf-8 -*-
# All configuration values have a default; values that are commented out
# serve to show the default.

import os

from sphinx import __version__ as sphinx_version
import sphinx_bootstrap_theme


# -- General configuration ------------------------------------------------

if sphinx_version > "1.6":
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
    'sphinx.ext.pngmath',
    'sphinx.ext.autodoc',
    'sphinx.ext.intersphinx'
]

# Add any paths that contain templates here, relative to this directory.
templates_path = ['_templates']

# The suffix of source filenames.
source_suffix = '.rst'

# The master toctree document.
master_doc = 'index'

# General information about the project.
project = u'MantidProject'
copyright = u'2007-2018, Mantid'

# The version info for the project you're documenting, acts as replacement for
# |version| and |release|, also used in various other places throughout the
# built documents.

# The short X.Y version.
version = "master"
# The full version, including alpha/beta/rc tags.
release = version

# The name of the Pygments (syntax highlighting) style to use.
pygments_style = 'sphinx'

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

# Theme-specific options to customize the look and feel of a theme.
# We config the bootstrap settings here, and apply CSS changes in
# custom.css rather than here.
html_theme_options = {
    # Navigation bar title.
    'navbar_title': " ", # deliberate single space so it's not visible
    # Tab name for entire site.
    'navbar_site_name': "Mantid",
    # Add links to the nav bar. Third param of tuple is true to create absolute url.
    'navbar_links': [
        ("Home", "index"),
        ("Download", "http://download.mantidproject.org", True),
        ("Wiki", "http://www.mantidproject.org", True),
        ("User Documentation", "http://docs.mantidproject.org", True),
        ("Contact Us", "http://www.mantidproject.org/Contact", True),
    ],
    # Do not show the "Show source" button.
    'source_link_position': "no",
    # Remove the local TOC from the nav bar
    'navbar_pagenav': False,
    # Hide the next/previous in the nav bar.
    'navbar_sidebarrel': True,
    # Use the latest version.
    'bootstrap_version': "3",
    # Ensure the nav bar always stays on top of page.
    'navbar_fixed_top': "false",
}

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

# -- Link to other projects ----------------------------------------------------

intersphinx_mapping = {
    'h5py': ('http://docs.h5py.org/en/latest/', None),
    'matplotlib': ('http://matplotlib.org', None),
    'numpy': ('https://docs.scipy.org/doc/numpy/', None),
    'python': ('https://docs.python.org/3/', None),
    'SciPy': ('http://docs.scipy.org/doc/scipy/reference', None),
    'mantid': ('http://docs.mantidproject.org/', None)
}
