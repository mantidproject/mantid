.. _documentation_testing:

Documentation Testing
===============================

.. contents::
   :local:

*Prerequisites*

- For test 2, you may need to install *BeautifulSoup* on your conda environment if you haven't yet: ``mamba install conda-forge::beaufitulsoup4``


Documentation Test
------------------
There are many pages in the documentation, although this test doesn't inspect them all, try to open and check as many as possible.
On the first part of the tests, you will be guided through the Qt-help window on Mantid Workbench, to perform a number of actions in different
documentation pages.

On general, to inspect a given documentation page you need to make sure that the following items are displayed correctly, either online or offline:

- Pages are generated without any malformatted text, sections, numbered and bullet point lists.
- Snapshots for algorithms or interfaces as well as figure plots are rendered and framed correctly.
- Math formulae are rendered correctly.
- Code snippets are framed and formatted correctly.
- Workflow diagrams are rendered correctly.
- Links and hyperlinks are directing to the corresponding sections or pages.

Test 1: Offline documentation test on the workbench.
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

**Time required: 10-15 minutes**

------------------------------------

#. Go to ``Help -> Mantid Help`` on the navigation bar of **Mantid Workbench**. A **Qt-Help** window should appear with the documentation.
   Close the window.
#. Check that both clicking on ``Help -> Mantid Concepts`` and ``Help -> Algorithm Descriptions`` opens the documentation again, redirecting to the
   appropiate sections of it.
#. Check that clicking on ``Help -> Mantid Homepage`` and ``Help -> Mantid Forum`` prompts a new web browser window linking to the respective sites.
#. Check that clicking on ``Help -> About Mantid Workbench`` prompts the startup Mantid splash screen.
#. On **Mantid Workbench**, press F1 key, same help window as before should appear. Rest of the test will continue on the **Mantid - Help** window.
#. On the ``Index`` tab, scroll down until the end of the list. Double-click on ``ZFMuonium`` and ``ZFdipole`` and check that both pages are displayed.
#. Go to the ``Search`` tab, and search for ``ZFMuonium``, first result should link to ``ZFMuonium`` page of the documentation.
#. Click on ``Print...`` button. It should prompt the print dialog window. Close it.
#. Click on the ``Previous`` button (just right of ``Print...`` button) continuosly until you return to the **Home** page of the documentation.
#. On the **Home** page of the documentation. Click on ``MantidWorkbench`` hyperlink. Within ``MantidWorkbench``, try and open
   some of the documentation pages.
#. Click on ``Home`` button to return to the main documentation page.
#. The documentation page has three sections: **Getting Started , Reference Documentation** and **Other Help and Documentation**.
#. On **Getting started**:

   - Check that each hyperlink works. Additionally, ``Mantid Matplotlib Plot Gallery and Examples`` correctly displays the figures and code snippets.

#. On **Reference Documentation**:

   - Open and check each hyperlink, from ``Algorithms`` to ``Release notes``.
   - On Algorithms: Scroll down to ``Algorithm Index`` at the end of the page and click on it. Check that a few algorithms are displayed as intended. Also, clicking on the link to the ``Source``
     file at the end of the algorithm pages links to the corresponding git-hub page.
   - Check a few pages on ``Concepts``, ``Interfaces``, ``Techniques`` and ``Fitting Overview``.
   - Check that the ``Release Notes`` are properly formatted and updated to the latest release.

#. On **Other Help and Documentation**:

   - Check that the links to the Mantid Forum and the Developer Documentation are working.


Test 2: Online documentation test.
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

**Time required: 10-20 minutes (depending on how many pages you check)**

------------------------------------

On this test, you better use the ``OpenMostDocumentationForTesting.py`` script. This script will open a large number of documentation pages
on the web browser.

This script can be found in the Mantid source folder at ``/tools/scripts/OpenMostDocumentationForTesting.py``.

There are two optional input arguments to the script: ``-d`` controls the delay in opening pages, and ``-k`` allows to open a random subset of *k* pages per documentation section (if you don't want to open all documentation pages).

#. It's recommended that you check most documentation pages, open the script by using default arguments: ``python OpenMostDocumentationForTesting.py``. It would open approximately 397 tabs with documentation pages in your default browser.
#. Check that the documentation pages are displayed correctly. *Tip*: For faster browsing, use ``Ctrl+Tab`` for moving between tabs.
