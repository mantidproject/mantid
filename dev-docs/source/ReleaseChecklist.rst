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
-----

The roles are defined in terms of the people responsible.
Those in the role do not need to carry out all duties themselves, but they
are responsible for ensuring that the work gets done.

* :ref:`Local Project Manager(s) <local-project-managers-checklist>` - People in charge
  of communicating with local development teams, facility management, and other people
  at their sponsoring facility.
* :ref:`Quality Assurance Manager <quality-assurance-manager-checklist>` - People responsible
  for making sure that manual testing has been performed. They will ensure Mantid meets
  quality requirements before delivery in consultation with the *Release Manager*.
* :ref:`Release Editor <release-editor-checklist>` - People responsible for ensuring the
  release notes are edited to give them a common language, layout, and illustrative images.
* :ref:`Release Manager <release-manager-checklist>` - Person in charge of the go/no go
  decision of the release. The main task is to reiterate the timeline and be the collection
  point for information between all of the *Local Project Managers*.
* :ref:`Technical Release Manager <technical-release-manager-checklist>` - People responsible
  for technical tasks such as renaming branches, creating tags, configuring build servers, and
  ensuring problems on the Release Pipeline get fixed (by themselves or others).

Timeline
--------

+---------------------------+-----------------------------------------------+--------------------------------------------------------------------------+--------------------------+
| | Key Event(s)            | | Task Priorities                             | | Actions Required from                                                  | | Time Until Release     |
|                           |                                               |                                                                          |                          |
+===========================+===============================================+==========================================================================+==========================+
| 1 week before Code Freeze | Development, Testing & Documentation          | | :ref:`Local Project Manager(s) <local-project-managers-checklist>`     |  4+ weeks                |
|                           |                                               | | :ref:`Release Manager <release-manager-checklist>`                     |                          |
+---------------------------+-----------------------------------------------+--------------------------------------------------------------------------+--------------------------+
| **Code Freeze Begins**    | Final Development, Testing & Documentation    | | :ref:`Local Project Manager(s) <local-project-managers-checklist>`     |  3 weeks + 1 working day |
|                           |                                               | | :ref:`Release Manager <release-manager-checklist>`                     |                          |
|                           |                                               | | :ref:`Technical Release Manager <technical-release-manager-checklist>` |                          |
+---------------------------+-----------------------------------------------+--------------------------------------------------------------------------+--------------------------+
| **Manual Testing**        | Blocker bug fixes, Testing & Release Notes    | | :ref:`Quality Assurance Manager <quality-assurance-manager-checklist>` |  3 weeks                 |
|                           |                                               | | :ref:`Release Manager <release-manager-checklist>`                     |                          |
+---------------------------+-----------------------------------------------+--------------------------------------------------------------------------+--------------------------+
| **Beta Testing Begins**   | Blocker bug fixes, Testing, Release Notes,    | | :ref:`Local Project Manager(s) <local-project-managers-checklist>`     |  2.5 weeks               |
|                           | Maintenance Tasks & Next release development  | | :ref:`Release Editor <release-editor-checklist>`                       |                          |
|                           |                                               | | :ref:`Release Manager <release-manager-checklist>`                     |                          |
+---------------------------+-----------------------------------------------+--------------------------------------------------------------------------+--------------------------+
| Reminder Email for Beta   | Blocker bug fixes, Testing, Release Notes,    | | :ref:`Release Manager <release-manager-checklist>`                     |  1.5 weeks               |
|                           | Maintenance Tasks & Next release development  |                                                                          |                          |
+---------------------------+-----------------------------------------------+--------------------------------------------------------------------------+--------------------------+
| **Beta Testing Ends**     | Blocker bug fixes, Testing, Release Notes,    | | :ref:`Release Editor <release-editor-checklist>`                       |  ~ 4 working days        |
|                           | Maintenance Tasks & Next release development  | | :ref:`Release Manager <release-manager-checklist>`                     |                          |
|                           |                                               | | :ref:`Quality Assurance Manager <quality-assurance-manager-checklist>` |                          |
+---------------------------+-----------------------------------------------+--------------------------------------------------------------------------+--------------------------+
| **Smoke Testing**         | Blocker bug fixes, Testing & Packaging        | | :ref:`Quality Assurance Manager <quality-assurance-manager-checklist>` |  1 working day           |
|                           |                                               | | :ref:`Release Manager <release-manager-checklist>`                     |                          |
|                           |                                               | | :ref:`Technical Release Manager <technical-release-manager-checklist>` |                          |
+---------------------------+-----------------------------------------------+--------------------------------------------------------------------------+--------------------------+
| **Release Day**           | Blocker bug fixes, Testing & Release          | | :ref:`Release Manager <release-manager-checklist>`                     |  0                       |
|                           | Announcements                                 | | :ref:`Technical Release Manager <technical-release-manager-checklist>` |                          |
+---------------------------+-----------------------------------------------+--------------------------------------------------------------------------+--------------------------+

.. _local-project-managers-checklist:

Local Project Managers Checklist
----------------------------------

**Role**: People in charge of communicating with local development teams, facility
management, and other people at their sponsoring facility.

1 week before Code Freeze
#########################

*  Before the code freeze is in place the PM and Technical Steering Committee (TSC)
   need to agree the maintenance tasks for the next release period.

Code Freeze
###########

*  Attempt to drive the pull requests for this milestone down to 0, in collaboration
   with the Release Manager.

Beta Testing Begins
###################

*  Ensure that developers arrange to meet with their beta testers.
*  Triage when necessary the issues discovered during beta testing.

.. _quality-assurance-manager-checklist:

Quality Assurance Managers Checklist
------------------------------------

**Role**: People responsible for making sure that manual testing has been performed.
They will ensure Mantid meets quality requirements before delivery in consultation
with the Release Manager.

Manual Testing
###############

*  Ensure that Manual testing begins by creating the testing tasks as GitHub issues, assigning them and posting on
   Slack. Most of our Manual testing instructions are :ref:`here <Testing>`. Generate the Manual testing issues by
   following the instructions in the
   `README file <https://github.com/mantidproject/documents/tree/main/Project-Management/Tools/RoadmapUpdate>`__.
   Please raise the issues from the ISIS and Non-ISIS manual testing spreadsheets.
*  Over the next week, check the Manual testing GitHub issues. Testers should raise any
   serious problems as separate GitHub issues with a relevant milestone. When testing tasks are complete and all serious
   problems raised as issues, then the testing GitHub issue should be closed.
*  Manual testing at ISIS as of release 6.3, has taken the form of
   `Ensemble Manual Testing <https://github.com/mantidproject/documents/blob/main/Project-Management/Tools/RoadmapUpdate/Ensemble%20Manual%20Testing.pptx>`__.
   In short, testing teams of around 3-5 developers, spread across sub-teams
   are assigned tasks with the code expert in that testing team.

Beta Testing Ends - Prepare for Smoke Testing
#############################################

* Liaise with the technical release manager and project manager to decide on an appropriate time for Smoke Testing.
* Send an invite to developers for 1.5 hours maximum Smoke Testing. Include an introduction message to assign all testers to a certain operating system.
  Link to the release pipeline builds where the release packages *WILL* be. Encourage testers to download
  in the 30 minutes before smoke testing. Inform that ticking on a testing issue means that someone has assigned themselves and will tackle that task.
* The QA manager should pre-setup 3 ISIS IDAaaS Mantid dev instances and manually install the release package before testing
  so the 1.5 hours is clear for testing time. Then share the instances with the relevant testers from the IDAaaS workspace settings.

Smoke Testing
#############

*  Make sure to follow the preparation steps listed above.
*  It is likely that many changes have been made over the beta test period, therefore
   we must do some more manual testing to ensure everything still works. This stage is
   called Smoke testing. Generate the Smoke testing issues by following the instructions
   `here <https://github.com/mantidproject/documents/tree/main/Project-Management/Tools/RoadmapUpdate/SmokeTesting>`__.
*  Liaise with the Technical Release Manager and together announce the creation of the
   Smoke testing issues and Release Candidates in the *\#general* slack channel.


.. _release-editor-checklist:

Release Editors Checklist
-------------------------

**Role**: People responsible for editing the release notes and giving them a common
language, layout, and collecting images.

Beta Testing Begins
###################

* Initial amalgamation of the the release notes:

  * ``git pull`` on ``release-next``.
  * Create a new branch using the `Mantid Git Workflow guidance <https://developer.mantidproject.org/GitWorkflow.html#new-branches>`__.
  * Navigate to your Mantid 'build' directory and open ``command-prompt.bat``.
  * In the new command prompt, navigate to the `release_editor.py script <https://github.com/mantidproject/mantid/blob/main/tools/ReleaseNotes/release_editor.py>`__ and run, parsing the correct version number. The script copies all of the separate release notes under the correct heading of their upper level file, e.g. ``framework.rst``, and moves the original release notes into a 'Used' directory.

    .. code-block:: bash

      python release_editor.py --release 6.5.0

  * Check the script has run correctly by checking all individual rst files have been moved into their respective 'used' directories. You could use the `unused_release_note_finder.py script <https://github.com/mantidproject/mantid/blob/main/tools/ReleaseNotes/unused_release_note_finder.py>`__ for this (explained further bellow).
  * Look over the files to make sure they look roughly correct then submit a PR to be merged into ``release-next``.

* Initial changes:

  * For each file which needs changes, create a new branch (example name ``6.5_workbench_release_notes``) and work on changes to then be merged back into ``release-next``.

* Incoming release notes:

  * As the release sprint goes on, new release note files will be created (existing outside of the 'Used' directories). The text from these will need to be copped into the main release note pages (``diffraction.rst``, ``mantidworkbench.rst`` etc.) and the file itself moved to it's corresponding 'Used' directory.
  * It is best to wait until several of these have built up before making a new branch / pr.
  * To help with finding the new release notes, use the `unused_release_note_finder.py script <https://github.com/mantidproject/mantid/blob/main/tools/ReleaseNotes/unused_release_note_finder.py>`__ which will print the location of release notes not within a 'Used' directory.

    .. code-block:: bash

      python unused_release_note_finder.py --release 6.5.0

* Images:

  * Images or GIFs should be added to highlight important and/or visual changes.
  * An image for the 'headline' feature (or a collage if there is none) should be added to the main page (``index.rst``).

Beta Testing Ends
#################

*  Review the complete set of release notes to make sure there are no glaring mistakes.

Just before release
###################

* As one of the final steps in preparing to tag the release:

  * Add any final release notes manually to the main release note pages. (Make sure to check `main` for any release notes that have been merged into the wrong branch)
  * Remove unused headings from the main release note pages.
  * Remove all separate release note files and sub-file structure to leave just the main release note pages.

.. _release-manager-checklist:

Release Manager Checklist
-------------------------

**Role**: Person in charge of the go/no go decision of the release. The main task
is to reiterate the timeline and be the collection point for information between
all of the Local Project Managers.

1 week before Code Freeze
#########################

*  Check that all people with release roles are added to the *\#release-roles*
   Slack channel and others are removed.
*  Post on the *\#general* slack channel reminding developers of the impending
   release and stating that they have only 5 days left before the code freeze.
*  Send an email to beta test users explaining the dates for the testing, and
   stating they will have more detail on the start of the first day (cc the Local
   Project Manager(s) so they can organise a similar message at their facilities).

Code Freeze Begins
##################

*  Post on the *\#general* slack channel asking everyone to ensure they have moved
   any incomplete issues to the next milestone, stating the code freeze is in place,
   and warning developers that non-blocker issues will be moved from the milestone
   on Monday morning.
*  Attempt to drive the pull requests for this milestone down to 0, in collaboration
   with the Local Project Managers.

Manual Testing
##############

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

Beta Testing Begins
###################

*  Before sending an email to users regarding the beginning of beta testing, ensure that
   the Usage data .zip file containing usage data is up-to-date. This is done by
   downloading the current .zip from sourceforge, adding any missing files, and
   resending it.
*  Send an email to beta test users explaining where to download the installers and how
   to report issues (cc the Local Project Managers so they can organise a similar message
   at their facilities).

Reminder Email for Beta
#######################

*  Mid-way through the beta testing period, send a reminder email to beta test users thanking them for their feedback so
   far and reminding them to feedback as soon as possible and not to send in a list of
   issues at the end of testing (cc the Local Project Managers so they can organise a
   similar message at their facilities).

Beta Testing Ends
#################

*  At the end of the day email the beta test users thanking them.
*  Review the complete set of release notes to make sure there are no glaring mistakes.

Smoke Testing
#############

* This is the final day for code changes to the build for blocker issues.

Release Day
###########

After the Technical Release Manager has finished their release day tasks:

*  Send an email, including the text of the release notes, to the following lists, replacing <at> with the appropriate sign:

   ``nobugs<at>nobugsconference.org``

   ``news<at>neutronsources.org``

   ``neutron<at>neutronsources.org``

   ``announcements<at>mantidproject.org``

   ``ISIS Instrument Scientists + Other``

   ``supportanalysis<at>stfc.ac.uk``

*  Also post the contents of the message to the *\#announcements* channel on
   Slack.
*  Create a new item on the forum news.
*  Add a news item linking to the forum post on the `mantidproject website <https://www.mantidproject.org>`__
   by manually editing `sidebar-news.html <https://github.com/mantidproject/www/blob/main/source/_templates/sidebar-news.html>`__.
   Restrict the number of news items to five.
*  Close the release milestone on GitHub.

.. _technical-release-manager-checklist:

Technical Release Managers Checklist
------------------------------------

**Role**: People responsible for technical tasks such as renaming branches, creating
tags, configuring build servers, and ensuring problems on the Release Pipeline get fixed
(by themselves or others).

Code Freeze
###########

**Create the Release Branch (once most PR's are merged)**

* Ensure the latest `main nightly deployment pipeline
  <https://builds.mantidproject.org/view/Nightly%20Pipelines/job/main_nightly_deployment_prototype/>`__
  has passed for all build environments. If it fails, decide if a fix is needed before moving on to
  the next steps.
* Click ``Build Now`` on `open-release-testing
  <https://builds.mantidproject.org/view/All/job/open-release-testing/>`__,
  which will perform the following actions:

  * Create or update the ``release-next`` branch.
  * Enable the job to periodically merge ``release-next`` into ``main``.
  * Enable the ``release-next_nightly_deployment`` pipeline.
  * Disable the ``main_nightly_deployment_prototype`` pipeline.

* Check the state of all open pull requests for this milestone and decide which
  should be kept for the release, liaise with the Release Manager on this. Move any
  pull requests not targeted for this release out of the milestone, and then change
  the base branch of the remaining pull requests to ``release-next``. You can either
  manually change the base branch in GitHub or use the `update-pr-base-branch.py
  <https://github.com/mantidproject/mantid/blob/main/tools/scripts/update-pr-base-branch.py>`__
  script to update the base branches of these pull requests.
  A quick example to show how the arguments should be provided to this script is seen below:

.. code-block:: bash

    python update-pr-base-branch.py [milestone] [newbase] --token [generated_token]
    python update-pr-base-branch.py "Release 6.1" "release-next" --token fake123gener8ed456token

* Inform other developers that ``release-next`` has been created by posting to the
  *\#announcements* slack channel. You can use an adapted version of the
  following announcement:

  * The release branch for <version>, called ``release-next``, has now been created:
    https://github.com/mantidproject/mantid/tree/release-next. If you've not worked with
    the release/main/-branch workflow before then please take a moment to familiarise
    yourself with the process: https://developer.mantidproject.org/GitWorkflow.html#code-freeze.
    The part about ensuring new branches have the correct parent is the most important part
    (although this can be corrected afterwards). All branches and PRs that were created
    before this release branch was created are fine, as their history is the same as ``main``.

**Create Release Notes Skeleton**

*  Create a skeleton set of release notes and subfolders on ``main`` for the next version using the
   `python helper tool
   <https://github.com/mantidproject/mantid/blob/main/tools/release_generator/release.py>`__
   and open a pull request to put them on ``main``. Make sure the
   ``docs/source/release/index.rst`` file has a link to the new release docs.

.. code-block:: bash

    python release.py --release [X.Y.Z] --milestone [milestone]
    python release.py --release 6.1.0 --milestone "Release 6.1"

Smoke Testing
#############

Check with the Quality Assurance Manager that the initial Manual testing has been completed, and any issues
have been fixed. Then:

* Liaise with the release editor to ensure that they have completed all of their tasks.
* Check the release notes and verify that the "Under Construction" paragraph on the main
  index page has been removed. Remove the paragraph if it still exists.
* Run the `close-release-testing <https://builds.mantidproject.org/view/All/job/close-release-testing>`__
  job, which will do the following:

  * Disable the job that periodically merges ``release-next`` into ``main``.
  * Disable the ``release-next_nightly_deployment`` pipeline.
  * Enable the ``main_nightly_deployment_prototype`` pipeline.

**Create the Release Candidates**

We are now ready to create the release candidates for Smoke testing.

* On the ``release-next`` branch, check whether the `git SHA
  <https://github.com/mantidproject/mantid/blob/343037c685c0aca9151523d6a3e105504f8cf218/scripts/ExternalInterfaces/CMakeLists.txt#L11>`__
  for MSlice is up to date. If not, create a PR to update it and ask a gatekeeper to merge it.
* On the ``release-next`` branch, create a PR to update the `major & minor
  <https://github.com/mantidproject/mantid/blob/release-next/buildconfig/CMake/VersionNumber.cmake>`__
  versions accordingly. Also, uncomment ``VERSION_PATCH`` and set it to ``0``. Ask a gatekeeper to merge the PR.
* Ask a gatekeeper to merge the ``release-next`` branch back to ``main`` locally, and then comment
  out the ``VERSION_PATCH`` on the ``main`` branch. They should then commit and push these changes
  directly to the remote ``main`` without making a PR.
* Build the `release-next_nightly_deployment Jenkins pipeline <https://builds.mantidproject.org/view/Release%20Pipeline/>`__
  with the following parameters (most are already defaulted to the correct values):

  * set ``BUILD_DEVEL`` to ``all``
  * set ``BUILD_PACKAGE`` to ``all``
  * set ``CONDA_RECIPES_BRANCH_NAME`` to ``main``
  * set ``PACKAGE_SUFFIX`` to an **empty string**
  * tick the ``PUBLISH_PACKAGES`` checkbox
  * set the ``ANACONDA_CHANNEL`` to ``mantid``
  * set the ``ANACONDA_CHANNEL_LABEL`` to ``rcN`` where ``N`` is an incremental build number for release
    candidates, starting at ``1``
  * set ``GITHUB_RELEASES_REPO`` to ``mantidproject/mantid``
  * set ``GITHUB_RELEASES_TAG`` to ``vX.Y.Z-rcN``, where ``X.Y.Z`` is the release number,
    and ``N`` is an incremental build number for release candidates, starting at ``1``.

* Once the packages have been published to GitHub and Anaconda, ask someone in the ISIS core or DevOps
  team to manually sign the Windows binary and re-upload it to the GitHub
  `draft release <https://github.com/mantidproject/mantid/releases>`__.
* Liaise with the Quality Assurance Manager and together announce the creation of the smoke testing
  issues and Release Candidates in the *\#general* slack channel.

Release Day
###########

Check with the Quality Assurance Manager that the Smoke testing has been completed, and any issues
have been fixed.

*  Edit the `draft release <https://github.com/mantidproject/mantid/releases>`__ on
   GitHub. A new tag should be created based off the release branch in the form ``vX.Y.Z``. The
   description of the new release can be copied from the release notes ``index.rst`` file, and
   edited appropriately. See previous release descriptions for inspiration.
*  Publish the GitHub release. This will create the tag required to generate the DOI.
*  Change the labels for the release candidates on our `Anaconda site <https://anaconda.org/mantid/mantid/files>`__
   from ``rcN`` to ``main``. You may need to ask a member of the ISIS core or DevOps team to do this.
*  Update the `download page <https://download.mantidproject.org>`__ by running the `Update latest release links
   <https://github.com/mantidproject/www/actions/workflows/update-latest-release.yml>`__ workflow in the
   `mantidproject/www repository <https://github.com/mantidproject/www>`__.
*  Ask someone with access to the `daaas-ansible-workspace repository
   <https://github.com/ral-facilities/daaas-ansible-workspace>`__ (a member of the ISIS core team or IDAaaS support)
   to add the new release to IDAaaS. They can do this by creating a PR targeting the ``preprod`` branch, adding
   the new release version to the list of versions installed on IDAaaS `here
   <https://github.com/ral-facilities/daaas-ansible-workspace/blob/master/roles/software/analysis/mantid/defaults/main.yml>`__.
   Make sure that there are only three ``mantid_versions`` in the list (delete the oldest one if applicable).
   The changes need to be verified on an IDAaaS test instance before the PR can be accepted.

**Generate DOI**

This requires that a tag has been created for this release. This tag is created when you draft and
publish a new `release <https://github.com/mantidproject/mantid/releases>`__ on GitHub.

*  Make sure that you have updated your local copy of git to grab the new tag.
   ``git fetch -p``
*  If the script below fails you may need to update the authors list and push the
   updates to ``main``. Look for ``authors.py`` in the ``tools/DOI`` directory.
   It does not matter that these are not on the release branch.

.. code-block:: bash

    python tools/DOI/doi.py --username=[username] [X.Y.Z]

for example

.. code-block:: bash

    python tools/DOI/doi.py --username="doi.username" 6.1.0

*  The script will prompt you for the password. Ask a senior developer to share the username and
   password with you if you do not already have access to it.

**Update Citation File**

Open a PR updating the software ``doi``, ``date-released`` and ``version`` in the ``CITATION.cff`` file
at the root of the repository.

Notify the Release Manager when you complete all your tasks.
