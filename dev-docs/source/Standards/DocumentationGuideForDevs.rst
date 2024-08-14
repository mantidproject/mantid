.. _DocumentationGuideForDevs:

============================
Documentation Guide for Devs
============================

.. contents::
  :local:

This page gives an overview of the documentation process for mantid.

The documentation is written in `reStructuredText <https://docutils.sourceforge.io/rst.html>`__
and processed using `Sphinx <http://www.sphinx-doc.org/en/master/>`__ along with
`Sphinx bootstrap theme <https://pypi.python.org/pypi/sphinx-bootstrap-theme/>`__ and custom css.


Configuration
-------------

The documentation is configured using CMake and assumes the instructions in the
:ref:`Getting Started <GettingStarted>` section have been followed.

The following settings, relating to the documentation, are available:

* ``DOCS_DOTDIAGRAMS``: If enabled then the workflow diagrams are generated and included
  otherwise a placeholder image is included with a caption indicating the processing is disabled. Default=OFF.
* ``DOCS_SCREENSHOTS``: If enabled then the automatic screenshots of algorithm dialogs and interfaces are included
  otherwise a placeholder image is included with a caption indicating the processing is disabled. Default=OFF.
* ``DOCS_MATH_EXT``: Extension used to provide the ``:math:`` processing ability.
  Default=``sphinx.ext.mathjax`` in CMake but this is set to ``sphinx.ext.imgmath``
  for clean package builds to remove the javascript dependency.
* ``DOCS_PLOTDIRECTIVE``: If enabled then the ``:plot:`` directive is processed and
  the code/plots are included. WARNING: This can add ~15mins to the build time. Default=OFF.

The defaults are set to produce a reasonable build time for developers but set to build everything on clean package builds.

reST editors/IDE plugins
------------------------

Several free & proprietary editors support syntax-highlighting reST.
A list of the most notable ones can be found `here <https://stackoverflow.com/questions/2746692/restructuredtext-tool-support>`__.

Other tools:

* `restview <https://pypi.python.org/pypi/restview>`__: which can be easily installed using ``pip``.
  It opens a webpage with the rst file processed and refreshes the page automatically whenever the .rst file is saved.
  This can help you quickly track down that unexpected space or missing newline without having to rebuild the documentation each time.
  It does not support sphinx directives so links will produce errors on the page which need to be checked by building the documentation using Mantid.
  The syntax to use it is: ``restview path/to/file.rst``
* VSCode with `reStructuredText <https://marketplace.visualstudio.com/items?itemName=lextudio.restructuredtext>`__ and `esbonio <https://marketplace.visualstudio.com/items?itemName=swyddfa.esbonio>`__ enables previews of documents inside VSCode. The preview does not support the mantid directives but is useful for a general idea of structure.

The reStructuredText File
-------------------------

`reStructuredText <http://docutils.sourceforge.net/rst.html>`__ is a markup format and is converted into themed html pages using Sphinx.
A primer on reStructuredText can be found here along with a single-page cheat sheet.

The source files are .rst files which are located in the ``docs/source`` directory in the repository.
There are various subdirectories based on the type of object being documented, i.e. ``docs/source/algorithms``, ``docs/source/functions``.

The documentation pages is aimed at *users*, not developers, and all information should be targeted for the users.
Information for developers should go into doxygen/code-comments in the code or into ``dev-docs``.

Directives
##########

Sphinx is built on the ``docutils`` package that allows for directives to be inserted in to reST comments. For example:

.. code-block:: rest

    .. warning::
       This text will show up surrounded by a coloured box.

tells sphinx to treat the given text differently and flag it so that a user will see it as a warning. The name of the directive is ``warning`` and the ``..`` is a reST comment syntax. The directive name must be followed by ``::`` so that Sphinx process knows it has a directive command and not just plain text. For a list of directives known to Sphinx, see `here <http://www.sphinx-doc.org/en/master/rest.html#directives>`__.

Comments
########

If you wish to place comments in the reST file that will not be rendered anywhere then start the line/block with ``..``. See `here <http://sphinx-doc.org/rest.html#comments>`__ for more details.

Algorithms
##########

The algorithm documentation has a slightly more rigid structure and is described in more detail :ref:`here <AlgorithmDocumentation>` and :ref:`here <AlgorithmUsageExamples>`.

Interfaces
##########

For documenting custom interfaces, it is recommended that you consult :ref:`this <InterfaceDocumentation>`  page,
which explains how to document them, and which directives may be used in more detail.

How to define titles, sections etc.
###################################

The syntax for headers in restructuredText is the header followed by a line containing symbols such as hyphens. It is possible to use different punctuation to create headers but within the Mantid .rst files we standardize on the characters used as follows:

The title of the page
   Should be the first header of your .rst file, and generally only occur once. (This is done for you in an algorithm with the ``.. algorithm::`` directive)

.. code-block:: rest

   =============================================
   Page title (e.g. Workspace) - This outputs H1
   =============================================

Section headings
   Sections, such as the description of an algorithm, can be created with the following syntax

.. code-block:: rest

   # Description - This outputs H2
   -------------------------------

Sub-sections
   The following is used to create a sub-section of the above section. This must follow after the above to be parsed correctly.

.. code-block:: rest

   Sub-heading - This outputs h3
   #############################

Sub-sub-sections
   The following is used to create a sub-header for the sub-heading above. This must also follow after the above header to be parsed correctly.

.. code-block:: rest

   Sub-sub-heading - Outputs h4
   ^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Common Warnings and Fixes
-------------------------

If you have weird messages about sphinx warnings that happen on “Console output”,
those are coming either from summary functions in algorithms or from parameter descriptions.
In these

* *Do not* use ``*`` in parameter names or summary. This yields “Inline emphasis start-string without end-string” warnings.
* *Do not* use things like ``|Q|``. This yields sphinx error “Undefined substitution referenced”.
* When using hyperlinks with a label, try to use anonymous hyperlinks (two underscores instead of one) to avoid name clashes.
   * ```MD <http://mysite.com/MD1.html>`__`` and ```MD <http://mysite.com/MD2.html>`__`` instead of ```MD <http://mysite.com/MD1.html>`_`` and ```MD <http://mysite.com/MD2.html>`_``. The second on will result in a warning.



While building the final output, Sphinx will emit warning messages if it things the input restructured text is malformed.
This section lists some more common warnings along with suggestions for fixes.

Explicit markup ends without a blank line; unexpected unindent.
###############################################################

This is caused by the lack of a blank line between an indented explicit markup block and more unindented text, e.g.

.. code-block:: rest

   .. testcode:: ExHist

      print "This is a test"
    Output:                         <------------- There should be a blank line above this

    .. testoutput:: ExHist

It can be fixed by having a blank line between the indented block and the unindented text.

Inline interpreted text or phrase reference start-string without end-string
###########################################################################

This is caused by using one of the `inline markup tags <http://www.sphinx-doc.org/en/master/rest.html#inline-markup>`__, where the text being wrapped splits over multiple lines. In these cases the directive variant of the inline markup should be used. One example is the ``:math:`` tag being spread over multiple lines. The tag ``:math:`` must only be used for inline markup, i.e. when there is no newline in the math string. For multi-line maths markup you must use the ``.. math::`` directive instead.

.. code-block:: rest

   :math:`\rm U \rm B \left(
                                \begin{array}{c}
                                  h_i \\
                                  k_i \\
                                  l_i \\
                                \end{array}
                               \right) = \rm Q_{gon,i}` (1)

should be written

.. code-block:: rest

   .. math::
                                                                   <------------------ intentional blank line
               \rm U \rm B \left(
                                   \begin{array}{c}
                                     h_i \\
                                     k_i \\
                                     l_i \\
                                   \end{array}
                                  \right) = \rm Q_{gon,i} (1)
                                                                   <------------------ intentional blank line

where there is an explicit blank line after the final line of latex. See `here <http://sphinx-doc.org/ext/math.html>`__ for more information.

image file not readable
#######################

This indicates the that image referenced by ``.. image::`` or ``.. figure::`` cannot be accessed.
Either the image is not there or the reference is incorrect.

Image links in Sphinx are either relative,
in which case it is relative to the current document or
absolute in which case the path is assumed relative to the root of the source tree (the directory containing the conf.py)

Unknown directive type "foo"
############################

Sphinx has encountered a line starting with ``.. foo::``, where ``foo`` is expected to be a known directive.

The fix is to correct the name of the directive.

Building the HTML Development Documentation
-------------------------------------------

The developer documentation is written as ``.rst`` files in the mantid source folder under ``dev-docs/``,
the html files can be built using the `dev-docs-html` target.
This will build all the development documentation into the mantid build folder under ``dev-docs/html/``.

In Visual Studio, this can be found in the "Documentation" folder in the solution explorer for the Mantid solution. Simply right click `dev-docs-html` and select build.
