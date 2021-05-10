.. _ReleaseChecklist:

=================
Release Checklist
=================

.. contents::
  :local:

These are the steps involved in performing a full Mantid release. To
request or perform a patch release look at the
:ref:`patch release checklist <PatchReleaseChecklist>`.

Roles
#####

The roles are defined in terms of person responsible.
This does not mean that the person needs to do the jobs himself/herself, but that they
are responsible for ensuring that the work gets done.

* :ref:`Local Project Manager(s) <local-project-managers-checklist>` - People in charge
  of communicating with local development teams, facility management, and other people
  at their sponsoring facility.
* :ref:`Quality Assurance Manager <quality-assurance-manager-checklist>` - Person responsible
  for making sure that manual testing has been performed. They will ensure Mantid meets
  quality requirements before delivery in consultation with the *Release Manager*.
* :ref:`Release Editor <release-editor-checklist>` - Person responsible for ensuring the
  release notes are edited to give them a common language, layout, and illustrative images.
* :ref:`Release Manager <release-manager-checklist>` - Person in charge of the go/no go
  decision of the release. The main task is to reiterate the timeline and be the collection
  point for information between all of the *Local Project Managers*.
* :ref:`Technical Release Manager <technical-release-manager-checklist>` - Person responsible
  for technical tasks such as renaming branches, creating tags, configuring build servers, and
  ensuring problems on the Release Pipeline get fixed (by themselves or others).

Timeline
########

Releases are normally planned to occur on a Monday, therefore this page will refer
to days assuming that is correct. If the final release day is not planned to be
Monday then the names of the days will have to be changed.

+---------------------------------+---------------------------+-----------------------------------------------+--------------------------------------------------------------------------+
| | Day,                          | | Key Event(s)            | | Task Priorities                             | | Actions Required from                                                  |
| | Countdown                     |                           |                                               |                                                                          |
+=================================+===========================+===============================================+==========================================================================+
| | **Friday**,                   | Code Freeze in 1 week     | Development, Testing & Documentation          | | :ref:`Local Project Manager(s) <local-project-managers-checklist>`     |
| | 4 weeks & 1 day               |                           |                                               | | :ref:`Release Manager <release-manager-checklist>`                     |
+---------------------------------+---------------------------+-----------------------------------------------+--------------------------------------------------------------------------+
| | **Friday**,                   | Code Freeze Begins        | Final Development, Testing & Documentation    | | :ref:`Local Project Manager(s) <local-project-managers-checklist>`     |
| | 3 weeks & 1 day               |                           |                                               | | :ref:`Release Manager <release-manager-checklist>`                     |
+---------------------------------+---------------------------+-----------------------------------------------+--------------------------------------------------------------------------+
| | **Monday**,                   |                           | Blocker bug fixes, Testing & Release Notes    | | :ref:`Quality Assurance Manager <quality-assurance-manager-checklist>` |
| | 3 weeks                       |                           |                                               | | :ref:`Release Manager <release-manager-checklist>`                     |
|                                 |                           |                                               | | :ref:`Technical Release Manager <technical-release-manager-checklist>` |
+---------------------------------+---------------------------+-----------------------------------------------+--------------------------------------------------------------------------+
| | **Wednesday**,                | Beta Testing Begins       | Blocker bug fixes, Testing, Release Notes,    | | :ref:`Local Project Manager(s) <local-project-managers-checklist>`     |
| | 2 weeks & 3 days              |                           | Maintenance Tasks & Next release development  | | :ref:`Release Editor <release-editor-checklist>`                       |
|                                 |                           |                                               | | :ref:`Release Manager <release-manager-checklist>`                     |
+---------------------------------+---------------------------+-----------------------------------------------+--------------------------------------------------------------------------+
| | **Wednesday**,                |                           | Blocker bug fixes, Testing, Release Notes,    | | :ref:`Release Manager <release-manager-checklist>`                     |
| | 1 week & 3 days               |                           | Maintenance Tasks & Next release development  |                                                                          |
+---------------------------------+---------------------------+-----------------------------------------------+--------------------------------------------------------------------------+
| | **Tuesday**,                  | Beta Testing Ends         | Blocker bug fixes, Testing, Release Notes,    | | :ref:`Quality Assurance Manager <quality-assurance-manager-checklist>` |
| | 4 days                        |                           | Maintenance Tasks & Next release development  | | :ref:`Release Editor <release-editor-checklist>`                       |
|                                 |                           |                                               | | :ref:`Release Manager <release-manager-checklist>`                     |
+---------------------------------+---------------------------+-----------------------------------------------+--------------------------------------------------------------------------+
| | **Friday**,                   | Release Eve               | Blocker bug fixes, Testing, Release Notes,    | | :ref:`Release Manager <release-manager-checklist>`                     |
| | 1 day                         |                           | Maintenance Tasks & Next release development  |                                                                          |
+---------------------------------+---------------------------+-----------------------------------------------+--------------------------------------------------------------------------+
| | **Monday**,                   | Release Day               | Blocker bug fixes, Testing, Release Notes,    | | :ref:`Release Manager <release-manager-checklist>`                     |
| | Release Day                   |                           | Maintenance Tasks & Next release development  | | :ref:`Technical Release Manager <technical-release-manager-checklist>` |
+---------------------------------+---------------------------+-----------------------------------------------+--------------------------------------------------------------------------+

.. _local-project-managers-checklist:

Local Project Manager(s) Checklist
##################################

**Role**: People in charge of communicating with local development teams, facility
management, and other people at their sponsoring facility.

Friday, 4 weeks & 1 day
-----------------------

*  Before the code freeze is in place the PM and Technical Steering Committee (TSC)
   need to agree the maintenance tasks for the next release period.

Friday, 3 weeks & 1 day
-----------------------

*  Attempt to drive the pull requests for this milestone down to 0, in collaboration
   with the Release Manager.

Wednesday, 2 weeks & 3 days
---------------------------

*  Ensure that developers arrange to meet with their beta testers.
*  Triage when necessary the issues discovered during beta testing.

.. _quality-assurance-manager-checklist:

Quality Assurance Manager Checklist
###################################

**Role**: Person responsible for making sure that manual testing has been performed.
They will ensure Mantid meets quality requirements before delivery in consultation
with the Release Manager.

Monday, 3 weeks
---------------

*  Ensure that Manual testing begins. The instructions for Manual testing are found
   `here <https://www.mantidproject.org/Unscripted_Manual_Testing>`__.

Tuesday, 4 days
---------------

*  It is likely that many changes have been made over the beta test period, therefore
   ensure the manual testing is redone following the instructions described `here
   <https://www.mantidproject.org/Unscripted_Manual_Testing>`__.

.. _release-editor-checklist:

Release Editor Checklist
########################

**Role**: Person responsible for editing the release notes and giving them a common
language, layout, and collecting images.

Wednesday, 2 weeks & 3 days
---------------------------

*  Create issues for people to neaten up the release notes and add images etc.
*  Ensure an image for the release is found to highlight the main changes for this
   release. This can be a collage of images if there is not a big 'headline' feature
   or change.

Tuesday, 4 days
---------------

*  Review the complete set of release notes to make sure there are no glaring mistakes.

.. _release-manager-checklist:

Release Manager Checklist
#########################

**Role**: Person in charge of the go/no go decision of the release. The main task
is to reiterate the timeline and be the collection point for information between
all of the Local Project Managers.

Friday, 4 weeks & 1 day
-----------------------

*  Post on the *\#general* slack channel reminding developers of the impending
   release and stating that they have only 5 days left before the code freeze.
*  Send an email to beta test users explaining the dates for the testing, and
   stating they will have more detail on the start of the first day (cc the Local
   Project Manager(s) so they can organise a similar message at their facilities).

Friday, 3 weeks & 1 day
-----------------------

*  Post on the *\#general* slack channel asking everyone to ensure they have moved
   any incomplete issues to the next milestone, stating the code freeze is in place,
   and warning developers that non-blocker issues will be moved from the milestone
   on Monday morning.
*  Attempt to drive the pull requests for this milestone down to 0, in collaboration
   with the Local Project Managers.

Monday, 3 weeks
---------------

*  Ensure that PR testing has been completed for PRs from before the code freeze.

**Clearing the project board**

Go through the issues for the release milestone (not the sprint milestone), ensuring that:

*  All issues are intended for the release.
*  Any new issues are triaged on a daily basis, and allocated to staff.
*  Issues that are not important for the release should be moved to a more
   appropriate milestone. Don't leave anything in the release milestone that is not
   definitely for that release.

**Maintenance**

*  Present to the whole development team the maintenance tasks for this release period.
*  Emphasize the order of work priorities as noted by the task priorities in this
   checklist. Maintenance tasks may need to be paused to work on tasks for the release.

Wednesday, 2 weeks & 3 days
---------------------------

*  Before sending an email to users regarding the beginning of beta testing, ensure that
   the Usage data *.zip file containing usage data is up-to-date. This is done by
   downloading the current *.zip from sourceforge, adding any missing files, and
   resending it.
*  Send an email to beta test users explaining where to download the installers and how
   to report issues (cc the Local Project Managers so they can organise a similar message
   at their facilities).

Wednesday, 1 week & 3 days
--------------------------

*  Send a beta test reminder email to beta test users thanking them for there feedback so
   far and reminding them to feedback as soon as possible and not to send in a list of
   issues at the end of testing (cc the Local Project Managers so they can organise a
   similar message at their facilities).

Tuesday, 4 days
---------------

*  At the end of the day email the beta test users thanking them.
*  Review the complete set of release notes to make sure there are no glaring mistakes.

Friday, 1 day
-------------

* This is the final day for code changes to the build for blocker issues.

Monday, Release Day
-------------------

After the Technical Release Manager has finished their release day tasks:

*  Send an email, including the text of the release notes, to the following lists
  *  ``nobugs@nobugsconference.org``
  *  ``news@neutronsources.org``
  *  ``neutron@neutronsources.org``
*  Also post the contents of the message to the *\#announcements* channel on
   Slack.
*  Create a new item on the forum news.
*  Close the release milestone on github.

.. _technical-release-manager-checklist:

Technical Release Manager Checklist
###################################

**Role**: Person responsible for technical tasks such as renaming branches, creating
tags, configuring build servers, and ensuring problems on the Release Pipeline get fixed
(by themselves or others).

Monday, 3 weeks
---------------

**Create the Release Branch (once most PR's are merged)**

*  Ensure the `master build and system test
   <https://builds.mantidproject.org/view/Master%20Pipeline/>`__
   jobs have passed for all build environments for this release.
*  Run `open-release-testing
   <https://builds.mantidproject.org/view/All/job/open-release-testing/>`__
   to create the release branch and prepare build jobs by clicking ``Build Now``.
*  Check the state of all open pull requests for this milestone and decide which
   should be kept for the release, liaise with the Release Manager on this. Move any
   pull requests not targeted for release out of the milestone, and then change the base branch
   of the remaining pull requests to ``release-next``. You can use the following script
   to update the base branches of these pull requests `update-pr-base-branch.py
   <https://github.com/mantidproject/mantid/blob/master/tools/scripts/update-pr-base-branch.py>`__
   A quick example to show how the arguments should be provided to this script is seen below:

.. code-block:: bash

    python update-pr-base-branch.py [milestone] [newbase] --token [generated_token]
    python update-pr-base-branch.py "Release 6.1" "release-next" --token fake123gener8ed456token

*  Inform other developers that release-next has been created by posting to the
   *\#announcements* slack channel. You can use an adapted version of the
   following announcement:

  .. code

  The release branch for <version>, called release-next, has now been created: https://github.com/mantidproject/mantid/tree/release-next.  If you've not worked with the release/master-branch workflow before then please take a moment to familiarise yourself with the process: https://developer.mantidproject.org/GitWorkflow.html#code-freeze. The part about ensuring new branches have the correct parent is the most important part (although this can be corrected afterwards). All branches and PRs that were created before this release branch was created are fine, as their history is the same as master.

**Create Release Notes Skeleton**

*  Create a skeleton set of release notes on master for the next version using the
   `python helper tool
   <https://github.com/mantidproject/mantid/blob/master/tools/release_generator/release.py>`_
   and open a pull request to put them on ``master``. Make sure the
   ``docs/source/release/index.rst`` file has a link to the new release docs.

.. code-block:: bash

    python release.py --release [X.Y.Z] --milestone [milestone]
    python release.py --release 6.1.0 --milestone "Release 6.1"

Monday, Release Day
-------------------

**Release tasks**

Once the manual testing has passed (check with the Quality Assurance Manager):

*  Check the release notes and remove the "Under Construction" paragraph on the main
   index page.
*  Disable release deploy jobs by building the
   `close-release-testing <https://builds.mantidproject.org/view/All/job/close-release-testing>`__
   job.
*  On the ``release-next`` branch, update the git SHA for MSlice accordingly in
   ``scripts/ExternalInterfaces/CMakeLists`` in case MSlice has to be updated.
*  On the ``release-next`` branch, update major & minor versions accordingly in
   ``buildconfig/CMake/VersionNumber.cmake``. Also uncomment ``VERSION_PATCH`` and
   set it to ``0``.
*  Merge ``release-next`` branch back to ``master``
*  Comment out patch number on ``master`` branch
*  Hit build on `release kit builds <https://builds.mantidproject.org/view/Release%20Pipeline/>`__
   and set the ``PACKAGE_SUFFIX`` parameter to an empty string
*  Draft a `new release <https://github.com/mantidproject/mantid/releases>`__ on
   GitHub. The new tag should be created based of the release branch in the form ``vX.Y.Z``
*  After all of the packages have been smoke tested run the `release_deploy
   <https://builds.mantidproject.org/view/Release%20Pipeline/job/release_deploy/>`__
   job to put the packages, with the exception of Windows, on Sourceforge.

  *  Have someone at ISIS sign the Windows binary and upload this manually to Sourceforge

  *  Set the default package for each OS to the new version using the information icon
     next to the file list on Sourceforge

*  Upload packages to the GitHub release (essentially for a backup).
*  Publish the GitHub release. This will create the tag required to generate the DOI.
*  Update the `download <https://download.mantidproject.org>`__ page,
   following the instructions
   `here <https://github.com/mantidproject/download.mantidproject.org>`__. Once the new
   file in the `releases` directory is pushed Jenkins will publish the new page.
*  Publish the draft release on GitHub (this will create the tag too).
*  Kick off the build for ``mantidXY`` on RHEL7 for SNS:
   https://builds.mantidproject.org/job/release_clean-rhel7/ with suffix
   ``XY``.
* **ISIS**: If in cycle add a calendar reminder for when the current cycle ends for
  mantid to be updated on IDAaaS and cabin PCs. If out of cycle do this immediately.

**Generate DOI**

This requires that a tag has been created for this release. This is done automatically
if a new `release <https://github.com/mantidproject/mantid/releases>`__ has been
created on GitHub.

*  Make sure that you have updated your local copy of git to grab the new tag.
   ``git fetch -p``
*  If the script below fails you may need to update the authors list and push the
   updates to master. Look for ``authors.py`` in the ``tools/DOI`` directory.
   It does not matter that these are not on the release branch.

``python tools/DOI/doi.py  --username=_____  X.Y.Z``

*  Major/minor/patch version numbers must be supplied, as well as a username which can
   be found in the `Protected Information
   <https://www.mantidproject.org/Protected_Information>`__ section. The script will
   prompt for the password. Note that only MediaWiki admins have access rights to the page.
*  A corresponding version tag must be present in the Mantid repo.
