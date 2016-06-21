The Mantid documentation is written in [reStructuredText](http://docutils.sourceforge.net/rst.html) and processed using [Sphinx](http://sphinx.pocoo.org/). It uses a custom
boostrap theme for Sphinx and both are required to build the documentation.

To install Sphinx and the bootstrap theme use `easy_install`:

    easy_install -U Sphinx
    easy_install -U sphinx-bootstrap-theme

or `pip`:

    pip install Sphinx
    pip install sphinx-bootstrap-theme

This may require admin privileges on some environments.

CMake produces a `docs-html` target that is used to build the documentation (only if you set the cmake variable DOCS_HTML=ON). The output files will appear in a `html` sub directory of the main `build/docs` directory.