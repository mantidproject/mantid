# -*- mode: python -*-
# -*- coding: utf-8 -*-
# All configuration values have a default; values that are commented out
# serve to show the default.

from datetime import datetime

# -- General configuration ------------------------------------------------

# The root toctree document.
root_doc = "index"

# General information about the project.
project = "MantidProject"
copyright = f"2007-{datetime.now().year}, Mantid"

# The full version, including alpha/beta/rc tags.
release = "main"
# The short X.Y version.
version = "main"

# Add any paths that contain templates here, relative to this directory.
templates_path = ["_templates"]

# The suffix of source filenames.
source_suffix = {
    ".rst": "restructuredtext",
    ".md": "markdown",
}

# Add any Sphinx extension module names here, as strings. They can be
# extensions coming with Sphinx (named 'sphinx.ext.*') or custom ones.
# Use mathjax over pngmath here as the developer docs are only published online,
# so we don't need to worry about offline support.
extensions = [
    "mantid_sphinx_theme",
    "sphinx.ext.autodoc",
    "sphinx.ext.intersphinx",
    "sphinx.ext.mathjax",
    # myst_parser enables markdown support
    "myst_parser",
]


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

# -- Options for pngmath --------------------------------------------------

# Load the preview package into latex
pngmath_latex_preamble = r"\usepackage[active]{preview}"

# Ensures that the vertical alignment of equations is correct.
# See http://sphinx-doc.org/ext/math.html#confval-pngmath_use_preview
pngmath_use_preview = True

# If true, Smart Quotes will be used to convert quotes and dashes to
# typographically correct entities.
smartquotes = True

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
    "h5py": ("https://h5py.readthedocs.io/en/stable/", None),
    "matplotlib": ("http://matplotlib.org", None),
    "numpy": ("https://numpy.org/doc/stable/", None),
    "python": ("https://docs.python.org/3/", None),
    "SciPy": ("http://docs.scipy.org/doc/scipy/reference", None),
    "mantid": ("http://docs.mantidproject.org/", None),
}
