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
This does not mean that the person needs to do the jobs himself/herself, but that they are responsible for ensuring that the work gets done.

* Technical Release Manager - Person responsible for technical tasks such as renaming branchs, creating tags, configuring build servers.
* Release Editor - Person responsible for editing the release notes and giving them a common language, layout, and collecting images.
* Release Manager - Person in charge of the go/no go decision of the release. The main task is to reiterate the timeline and be collection point for information between all of the Local Project Managers.
* Local Project Manager(s) - People in charge of communicating with local development teams, facility managment, and other people at their sponsoring facility.
* Quality Assurance Manager - Person responsible for making sure that manual testing has been performed. They will ensure Mantid meets quality requirements before delivery in consultation with the *Release Manager*.

Timeline
########

Releases are normally planned to occur on a Monday, therefore this
page will be refer to days assuming that is correct, if the final
release day is not planned to be Monday then the names of the days
will have to be changed.

.. note::
   We used to release on a Friday, but changed to a Monday as no support was available over the weekend in case issues
   were raised with the newly released version.

Friday, Release-4 weeks (one week before code freeze)
#########################################################

**Task Priorities**: Development, Testing, Documentation.

*  Post on the General slack channel reminding developers of the
   impending release and stating that they have only 5 days left before
   the code freeze.
*  Send an email to beta test users explaining the dates for the
   testing, and stating they will have more detail on the start of the
   first day (cc the Local PMs so they can organise a similar message at their facilities).
*  Before the code freeze is in place the PM and TSC need to agree the maintenance tasks for the next release period.

Friday, Release-3 weeks
#######################

**Task Priorities**: Final Development until code freeze, Testing,
Documentation.

Code Freeze
-----------

*  Post on the General slack channel asking everyone to ensure they
   have moved any incomplete issues to the next milestone, stating the code freeze is in place, and
   warning developers that non-blocker issues will be moved from the
   milestone on Monday morning.
*  Final Testing afternoon, attempt to drive the pull requests for this
   milestone down to 0.

Monday, Release-2 weeks & 4 days
################################

**Task Priorities**: Blocker bug fixes, Testing, Release Notes.

Clearing the project board
--------------------------

* Go through the Zenhub project board for the release milestone (not the sprint milestone), ensuring that:

 *  All issues are intended for the release.
 *  Any new issues are triaged on a daily basis, and allocated to staff.
 *  Issues that are not important for the release should be moved to a more appropriate milestone.
    Don't leave anything in the release milestone that is not definitely for that release.


Unscripted and Final Testing
----------------------------

*  Ensure the
   `master build and system
   test <http://builds.mantidproject.org/view/Master%20Builds/>`__
   jobs have passed for all build environments for this release.
*  Complete any PR testing remaining from Friday
*  Perform unscripted testing following the instructions described
   `here <https://www.mantidproject.org/Unscripted_Manual_Testing>`__.

Maintenance
-----------
*  Present to the whole development team the maintenance tasks for this release period.
*  Emphasize the order of work priorities as noted in the task priorities throughout this checklist.
   Maintenance tasks may need to be paused to work on tasks for the release.

Create the Release Branch (once most PR's are merged)
-----------------------------------------------------

*  Ensure the
   `master build and system
   test <http://builds.mantidproject.org/view/Master%20Builds/>`__
   jobs have passed for all build environments for this release.
*  Run
   `open-release-testing <http://builds.mantidproject.org/view/All/job/open-release-testing/>`__
   to create the release branch and prepare build jobs
*  Check state of all open pull requests for this milestone and decide which should be kept for the release,
   liase with PM on this. Move any pull requests not targeted for release out of the milestone
   and run `update-pr-base-branch.py <https://github.com/mantidproject/mantid/blob/master/tools/scripts/update-pr-base-branch.py>`__
   to update the base branches of those pull requests.
*  Inform other developers that release-next has been created by adapting/posting the following announcement:

  .. code

  The release branch for <version>, called release-next, has now been created: https://github.com/mantidproject/mantid/tree/release-next.  If you've not worked with the release/master-branch workflow before then please take a moment to familiarise yourself with the process: http://developer.mantidproject.org/GitWorkflow.html#code-freeze. The part about ensuring new branches have the correct parent is the most important part (although this can be corrected afterwards). All branches and PRs that were created before this release branch was created are fine, as their history is the same as master.

*  Create a skeleton set of release notes on master for the next version using the `python helper tool <https://github.com/mantidproject/mantid/blob/master/tools/release_generator/release.py>`_ and open a pull request to put them on ``master``.


Wednesday, Release- 2 weeks & 3 days
####################################

**Task Priorities**: Blocker bug fixes, Testing, Release Notes,  Maintenance Tasks, Next release development.

Beta Test Open
--------------

*  Before sending an email to users, ensure that the Usage data .zip
   file containing usage data is up-to-date. This is done by downloading
   the current .zip from sourceforge, adding any missing files, and
   resending it.
*  Send an email to beta test users explaining where to download the
   installers and how to report issues (cc the Local PMs so they can organise a similar message at their facilities).
*  Developers to arrange to meet with their beta testers.
*  Create issues for people to neaten up the release notes and add images etc.

Wednesday, Release- 1 week & 3 days
###################################

**Task Priorities**: Blocker bug fixes, Testing, Release Notes,  Maintenance Tasks, Next release development.

Beta Test Reminder
------------------

*  Send an email to beta test users thanking them for there feedback so far and reminding them to feedback as soon as possible
   and not to send in a list of issues at the end of testing (cc the Local PMs so they can organise a similar message at their facilities).


Tuesday, Release- 4 days
########################

**Task Priorities**: Blocker bug fixes, Testing, Release Notes, Maintenance Tasks, Next release development.

Beta Test Closes
----------------

*  At the end of the day email the beta test users thanking them.
*  PM should review the complete set of release notes

Manual re-testing
-----------------

*  Is is likely that many changes have been made over the beta test period, therefore redo the unscripted testing
   following the instructions described `here <https://www.mantidproject.org/Unscripted_Manual_Testing>`__.

Wednesday, Release-2 days
#########################

**Task Priorities**: Blocker bug fixes, Testing, Release Notes,  Maintenance Tasks, Next
release development.

Thursday, Release-1 day
-----------------------

**Task Priorities**: Blocker bug fixes, Testing, Release Notes,  Maintenance Tasks, Next
release development.

Final Code Changes
------------------

* This is the final day for code changes to the build for blocker
  issues

Friday, Release day
###################

**Task Priorities**: Blocker bug fixes, Testing, Release Notes,  Maintenance Tasks, Next
release development.

Release (technical tasks)
-------------------------

Once the unscripted testing has passed:

* Check the release notes and remove the "Under Construction" paragraph
  on the main index page.
* Disable release deploy jobs by executing
  `close-release-testing <http://builds.mantidproject.org/view/All/job/close-release-testing>`__
  job.
* On the ``release-next`` branch, update major & minor versions
  accordingly in ``buildconfig/CMake/VersionNumber.cmake``. Also
  uncomment ``VERSION_PATCH`` and set it to ``0``.
* Merge ``release-next`` branch back to ``master``
* Comment out patch number on ``master`` branch
* Hit build on `release kit
  builds <http://builds.mantidproject.org/view/Release%20Pipeline/>`__
  and set the ``PACKAGE_SUFFIX`` parameter to an empty string
* Draft a `new
  release <https://github.com/mantidproject/mantid/releases>`__ on
  GitHub. The new tag should be created based of the release branch in
  the form ``vX.Y.Z``
* After all of the packages have been smoke tested run the
  `release_deploy <https://builds.mantidproject.org/view/Release%20Pipeline/job/release_deploy/>`__
  job to put the packages, with the exception of Windows, on Sourceforge.

  * Have someone at ISIS signs the Windows binary and upload this
    manually to Sourceforge

  * Set the default package for each OS to the new version using the information icon
    next to the file list on Sourceforge

* Upload packages to the GitHub release (essentially for a backup).
* Publish the GitHub release. This will create the tag required to generate the DOI.
* Update the `download <http://download.mantidproject.org>`__ page,
  following the instructions
  `here <https://github.com/mantidproject/download.mantidproject.org>`__. Once the new
  file in the `releases` directory is pushed Jenkins will publish the new page.
* Publish the draft release on GitHub (this will create the tag too).
* Kick off the build for ``mantidXY`` on RHEL7 for SNS:
  http://builds.mantidproject.org/job/release_clean-rhel7/ with suffix
  ``XY``
* **ISIS**: If in cycle add a calendar reminder for when the current cycle ends for mantid to be updated on IDAaaS and cabin PCs. If out of cycle do this immediately.

Finalise
========

* Send an email, including the text of the release notes, to the
  following lists
* ``nobugs@nobugsconference.org``
* ``news@neutronsources.org``
* ``neutron@neutronsources.org``
* Also post the contents of the message on Announcements on Slack
* Create a new item on the forum news
* Close the release milestone on github

Generate DOI (technical tasks)
------------------------------

This requires that a tag has been created for this release, this is done
automatically if a new
`release <https://github.com/mantidproject/mantid/releases>`__ has been
created on GitHub.

* Make sure that you have updated your local copy of git to grab the
  new tag. ``git fetch -p``
* If the script below fails you may need to update the authors list and
  push the updates to master. Look for ``authors.py`` in the
  ``tools/DOI`` directory. It does not matter that these are not on the
  release branch.

``python tools/DOI/doi.py  --username=_____  X.Y.Z``

* Major/minor/patch version numbers must be supplied, as well as a
  username which can be found in the `Protected
  Information <https://www.mantidproject.org/Protected_Information>`__
  section. The script will prompt for the password. Note that only
  MediaWiki admins have access rights to the page.
* A corresponding version tag must be present in the Mantid repo.
