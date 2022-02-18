.. _ReleaseNotesGuide:

===================
Release Notes Guide
===================

.. contents::
  :local:

This page gives an overview of the release note process for Mantid.

Like all documentation, release notes are written in `reStructuredText <https://docutils.sourceforge.io/rst.html>`__
and processed using `Sphinx <http://www.sphinx-doc.org/en/master/>`__ along with
`Sphinx bootstrap theme <https://pypi.python.org/pypi/sphinx-bootstrap-theme/>`__ and custom css. For the basics of how to format your release notes please see the :ref:`Documentation Guide For Devs <DocumentationGuideForDevs>` .


Adding release notes
--------------------
Release notes are added as new files in a pull request to avoid merge conflicts. Automated scripts then amalgamate these into the release notes available for users. If your work requires more than one note for the same subheading then it is fine to put them all in the same file. However,
**new release notes should not be added to any existing files**.

Amending release notes
----------------------
If you need to amend a release note from a previous Pull Request (PR) during sprint it is fine to do this as the automated script will update the release note. Provided developers use the naming conventions for files outlined below it should be easy to find the release note you wish to amend.
If the release note you wish to amend is within a ``Used`` directory you will need to contact the Release Editor to discuss how to get the release note amended.

File naming conventions
-----------------------
Release note files need to be named in one of the following ways:-

- Using the Pull Request (PR) number (preferred method)
- Using the Issue number (to be used if multiple PRs stem from the same issue)

Files need to have unique names. By using either the Issue number or PR number this makes them easier to find if they need to be updated at a later date.

Files should always be in .rst format.

File location
-------------
Release note files need to be saved in the directory that best represents their position within the release notes. First identify which release version your note relates to e.g. ``v6.3.0`` . Navigate to this directory and then navigate through the sub-directories until you reach a suitable sub heading. All notes need to be placed within a ``New_features``, ``Improvements``
or ``Bugfixes`` directory. For example a Bugfix release note for Engineering Diffraction should sit within ``/Diffraction/Engineering/Bugfixes`` .

Release notes should not be placed in any directory outside of ``New_features``, ``Improvements`` or ``Bugfixes`` e.g. do not place release notes in ``/Diffraction/Engineering``. You should also not save release notes in any directory titled ``Used`` as this is for notes that have already been collated into the release notes.

If you are uncertain where your release note should be see the :ref:`Standard File Structure <ReleaseNoteFileStructure>`.

Adding sub-headings
-------------------
There will be occasions when the range of standard headings is not suitable for your needs. If you want to add a heading you need to do the following:-

- Add a new directory into the correct part of the filing system. E.g. to add Algorithms to Workbench create the directory ``/Workbench/Algorithm``. The directory name should not contain any spaces.
- Add ``New_features``, ``Improvements`` and ``Bugfixes`` directories within your new directory. The automated script only works with these directories.
- Add a blank .rst file to each of the ``New_features``, ``Improvements`` and ``Bugfixes`` directories that has the same name as the directory - this is where the automated script amalgamates the separate release notes.
- Update the top level release note file with your new heading and sub-headings. For each subheading you need to add an include statement with a link to each new black .rst file to ensure the notes will be visible for users. For this example it may look something like this

.. code-block:: python

	Algorithms
	----------

	New features
	############
	.. include:: Workbench/Algorithm/New_features/New_features.rst

	Improvements
	############
	.. include:: Workbench/Algorithm/Improvements/Improvements.rst

	Bugfixes
	############
	.. include:: Workbench/Algorithm/Bugfixes/Bugfixes.rst


Once all the directories and files are in place you can add your release note as a separate file as outlined above.

During release
--------------
During the release period the automated scripting is turned off and the Release Editor will manually amalgamate release notes as part of their role. This should have no impact on adding new release notes provided you continue to follow the conventions above and do not save any files in the ``Used`` directories.
If you have any queries or concerns about release notes, particularly if you want to edit previous release notes, please contact the Release Editor.

.. _ReleaseNoteFileStructure:

Standard Release file structure
-------------------------------

This is the basic directory structure that is available to you for release notes.

* Diffraction

  - Powder Diffraction

	  + New features
	  + Improvements
	  + Bugfixes

  - Engineering Diffraction

	  + New features
	  + Improvements
	  + Bugfixes

  - Powder Diffraction

	  + New features
	  + Improvements
	  + Bugfixes

* Direct Geometry

  - General

	  + New features
	  + Improvements
	  + Bugfixes

  - CrystalField

	  + New features
	  + Improvements
	  + Bugfixes

  - MSlice

	  + New features
	  + Improvements
	  + Bugfixes

* Framework

  - Algorithms

	  + New features
	  + Improvements
	  + Bugfixes

  - Fit Functions

	  + New features
	  + Improvements
	  + Bugfixes

  - Data Objects

	  + New features
	  + Improvements
	  + Bugfixes

  - Python

	  + New features
	  + Improvements
	  + Bugfixes

* Indirect Geometry

  - New features
  - Improvements
  - Bugfixes

  - Algorithms

	  + New features
	  + Improvements
	  + Bugfixes

* Mantid Workbench

  - New features
  - Improvements
  - Bugfixes

  - InstrumentViewer

	  + New features
	  + Improvements
	  + Bugfixes

  - SliceViewer

	  + New features
	  + Improvements
	  + Bugfixes

* Muon

  - Frequency Domain Analysis

	  + New features
	  + Improvements
	  + Bugfixes

  - Muon Analysis

	  + New features
	  + Improvements
	  + Bugfixes

  - Muon and Frequency Domain Analysis

	  + New features
	  + Improvements
	  + Bugfixes

  - ALC

	  + New features
	  + Improvements
	  + Bugfixes

  - Elemental Analysis

	  + New features
	  + Improvements
	  + Bugfixes

  - Algorithms

	  + New features
	  + Improvements
	  + Bugfixes

* Reflectometry

  - New features
  - Improvements
  - Bugfixes

* SANS

  - New features
  - Improvements
  - Bugfixes