.. _DocumentationGuideForDevs:

============================
Documentation Guide for Devs
============================

.. contents::
  :local:

Rationale
=========

The algorithm documentation approach aims to:

#. Keep .cpp files clean and easy to maintain.
#. Make the documentation files easier to edit, supporting the use of external editors.
#. Simplify the Mantid documentation workflow.

To do this we have harmonised most of our documentation workflow to use sphinx, extended a bit by some Mantid custom tag extensions.

Prerequisites
=============

The documentation build requires:

Sphinx
------

* `Sphinx <http://www.sphinx-doc.org/en/master/>`__
* `Sphinx bootstrap theme <https://pypi.python.org/pypi/sphinx-bootstrap-theme/>`__

These are bundled with the Python distrbution on Windows but other platforms will need to install them following the installation instructions `here <https://github.com/mantidproject/mantid/blob/master/docs/README.md>`__. It is recommended that ``pip`` is used over ``easy_install``.

LaTeX
-----

To view the equations built with the documentation you will need an installation of LaTeX on your system.

Linux
#####

If you have installed the mantid-develop package then you should have a working latex distribution. If not, use your package manager and search for a suitable candidate, most likely named something like ``texlive``.

MacOS
#####

See `here <http://tug.org/mactex/>`__ for instructions on installing ``MacTeX``.

Windows
#######

Download and install ``MikTeX`` from `here <https://miktex.org/download>`__.

During installation there will be a question with a drop-down box relating to installing packages on demand - make sure "Ask first" is selected.

The first build of one of the ``docs-*`` targets after ``MikTeX`` has installed will raise a dialog, similar to this asking you if it is okay to install a package. For developers behind a proxy you will need to click on the "Change" button and enter the proxy information and select a new download location on the set of dialogs that appear before returning back to the main dialog. Check the box saying don't ask again and click install.

reST editors/IDE plugins
========================

Several free & proprietary editors support syntax-highlighting reST. A list of the most notable ones can be found `here <https://stackoverflow.com/questions/2746692/restructuredtext-tool-support>`__.

Restview
--------

A really handy tool whilst writing out .rst files is `restview <https://pypi.python.org/pypi/restview>`__ which can be easily installed using ``pip``. It opens a webpage with the rst file processed and refreshes the page automatically whenever the .rst file is saved. This can help you quickly track down that unexpected space or missing newline without having to rebuild the documentation each time. It does not support sphinx directives so links will produce errors on the page which need to be checked by building the documentation using Mantid. The syntax to use it is:

``restview path/to/file.rst``

The reStructuredText File
=========================

`reStructuredText <http://docutils.sourceforge.net/rst.html>`__ is a markup format and is converted into themed html pages using Sphinx. A primer on reStructuredText can be found here along with a single-page cheat sheet.

The source files are .rst files which are located in the ``docs/source`` directory in the repository. There are various subdirectories based on the type of object being documented, i.e. ``docs/source/algorithms``, ``docs/source/functions``.

The documentation pages is aimed at *users*, not developers, and all information should be targeted for the users. Information for developers should go into doxygen/code-comments in the code or into ``dev-docs``.

Directives
----------

Sphinx is built on the ``docutils`` package that allows for directives to be inserted in to reST comments. For example:

.. code-block:: rest

    .. warning::
       This text will show up surrounded by a coloured box.

tells sphinx to treat the the given text differently and flag it so that a user will see it as a warning. The name of the directive is ``warning`` and the ``..`` is a reST comment syntax. The directive name must be followed by ``::`` so that Sphinx process knows it has a directive command and not just plain text. For a list of directives known to Sphinx, see `here <http://www.sphinx-doc.org/en/master/rest.html#directives>`__.

Comments
--------

If you wish to place comments in the reST file that will not be rendered anywhere then start the line/block with ``..``. See `here <http://sphinx-doc.org/rest.html#comments>`__ for more details.

Algorithms
----------

The algorithm documentation has a slightly more rigid structure and is described in more detail `here <AlgorithmDocumentation.html>`__ and `here <AlgorithmUsageExamples.html>`__.

Interfaces
----------

For documenting custom interfaces, it is recommended that you consult `this <InterfaceDocumentation.html>`__  page, which explains how to document them, and which directives may be used in more detail. 

How to define titles, sections etc.
-----------------------------------

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

Things to Avoid
===============

If you have weird messages about sphinx warnings that happen on “Console output”, those are coming either from summary functions in algorithms or from parameter descriptions. In these

* *Do not* use ``*`` in parameter names or summary. This yields “Inline emphasis start-string without end-string” warnings.
* *Do not* use things like ``|Q|``. This yields sphinx error “Undefined substitution referenced”.
* When using hyperlinks with a label, try to use anonymous hyperlinks (two underscores instead of one) to avoid name clashes. 
   * ```MD <http://mysite.com/MD1.html>`__`` and ```MD <http://mysite.com/MD2.html>`__`` instead of ```MD <http://mysite.com/MD1.html>`_`` and ```MD <http://mysite.com/MD2.html>`_``. The second on will result in a warning.

Common Warnings and Fixes
-------------------------

While building the final output, Sphinx will emit warning messages if it things the input restructured text is malformed. This section lists some more common warnings along with suggestions for fixes. 

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

This indicates the that image referenced by ``.. image::`` or ``.. figure::`` cannot be accessed. Either the image is not there or the reference is incorrect.

Image links in Sphinx are either relative, in which case it is relative to the current document or absolute in which case the path is assumed relative to the root of the source tree (the directory containing the conf.py)

Unknown directive type "foo"
############################

Sphinx has encountered a line starting with ``.. foo::``, where ``foo`` is expected to be a known directive.

The fix is to correct the name of the directive.

Warnings on console (in the build servers)
##########################################

These type of errors occur in the summary function and/or in documentation of parameters in the init function. See `Things to Avoid`_.

Running documentation tests locally
===================================

The usage tests are executed using a driver script, ``runsphinx_doctest.py``, that is generated by CMake in the ``docs`` directory. A top-level target, ``docs-test``, is created for each generator that invokes the script without any arguments and subsequently executes all of the available usage tests.

The driver script has been written to accept additional arguments in order to be able to limit the number of tests that are executed. To run a subset of the available tests, the script must be called manually and supplied with the ``-R TESTREGEX`` argument. The regex is applied to the filename of the document and will match anywhere within the name. The script can be called using either a plain Python interpreter or the MantidPlot executable. If using a plain Python interpreter then you will need to either have your ``PYTHONPATH`` set to find the ``mantid`` module or you can provide the ``-m MANTIDPATH`` option to have the script find the module for you.

It is recommended that the tests are run with MantidPlot as this is the easiest way to be sure that they are being run with the current build copy. As an example, to run any files that have Rebin in the filename you would type (assuming you are in the build directory):

::

   bin/MantidPlot -xq docs/runsphinx_doctest.py -R Rebin

or with vanilla python

::

   python docs/runsphinx_doctest.py -m $PWD/bin -R Rebin

For multi-configuration generators such as Visual Studio or XCode you will need to pick the configuration by choosing the apporiate directory, e.g. for MSVC debug (remembering that the slashes need to be backslash and not forward slash):

::

   bin\Debug\MantidPlot -xq docs\runsphinx_doctest.py -R Rebin

Building the HTML Development Documentation
===========================================

The developer documentation is written as `.rst` files in the mantid source folder under ``dev-docs/``, the html files can be built using the `dev-docs-html` target. This will build all the development documentation into the mantid build folder under ``dev-docs/html/``.

In Visual Studio, this can be found in the "Documentation" folder in the solution explorer for the Mantid solution. Simply right click `dev-docs-html` and select build.