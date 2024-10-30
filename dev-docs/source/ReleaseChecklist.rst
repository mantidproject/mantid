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

+-----------------------------+-----------------------------------------------+--------------------------------------------------------------------------+--------------------------+
| | Key Event(s)              | | Task Priorities                             | | Actions Required from                                                  | | Time Until Release     |
|                             |                                               |                                                                          |                          |
+=============================+===============================================+==========================================================================+==========================+
| 2-3 weeks before Feature    | Development, Testing & Documentation          | | :ref:`Release Manager <release-manager-checklist>`                     |  4-6+ weeks              |
| Freeze                      |                                               |                                                                          |                          |
+-----------------------------+-----------------------------------------------+--------------------------------------------------------------------------+--------------------------+
| | 1 week before Feature     | Development, Testing & Documentation          | | :ref:`Local Project Manager(s) <local-project-managers-checklist>`     |  4+ weeks                |
| | Freeze                    |                                               | | :ref:`Release Manager <release-manager-checklist>`                     |                          |
+-----------------------------+-----------------------------------------------+--------------------------------------------------------------------------+--------------------------+
| **Feature Freeze Begins**   | Final Development, Testing & Documentation    | | :ref:`Local Project Manager(s) <local-project-managers-checklist>`     |  3 weeks + 1 working day |
|                             |                                               | | :ref:`Release Manager <release-manager-checklist>`                     |                          |
|                             |                                               | | :ref:`Technical Release Manager <technical-release-manager-checklist>` |                          |
+-----------------------------+-----------------------------------------------+--------------------------------------------------------------------------+--------------------------+
| **Manual Testing**          | Blocker bug fixes, Testing & Release Notes    | | :ref:`Quality Assurance Manager <quality-assurance-manager-checklist>` |  3 weeks                 |
|                             |                                               | | :ref:`Release Manager <release-manager-checklist>`                     |                          |
+-----------------------------+-----------------------------------------------+--------------------------------------------------------------------------+--------------------------+
| **Beta Testing Begins**     | Blocker bug fixes, Testing, Release Notes,    | | :ref:`Local Project Manager(s) <local-project-managers-checklist>`     |  2.5 weeks               |
|                             | Maintenance Tasks & Next release development  | | :ref:`Release Editor <release-editor-checklist>`                       |                          |
|                             |                                               | | :ref:`Release Manager <release-manager-checklist>`                     |                          |
+-----------------------------+-----------------------------------------------+--------------------------------------------------------------------------+--------------------------+
| Reminder Email for Beta     | Blocker bug fixes, Testing, Release Notes,    | | :ref:`Release Manager <release-manager-checklist>`                     |  1.5 weeks               |
|                             | Maintenance Tasks & Next release development  |                                                                          |                          |
+-----------------------------+-----------------------------------------------+--------------------------------------------------------------------------+--------------------------+
| **Beta Testing Ends**       | Blocker bug fixes, Testing, Release Notes,    | | :ref:`Release Editor <release-editor-checklist>`                       |  ~ 4 working days        |
|                             | Maintenance Tasks & Next release development  | | :ref:`Release Manager <release-manager-checklist>`                     |                          |
|                             |                                               | | :ref:`Quality Assurance Manager <quality-assurance-manager-checklist>` |                          |
+-----------------------------+-----------------------------------------------+--------------------------------------------------------------------------+--------------------------+
| **Smoke Testing**           | Blocker bug fixes, Testing & Packaging        | | :ref:`Quality Assurance Manager <quality-assurance-manager-checklist>` |  1 working day           |
|                             |                                               | | :ref:`Release Manager <release-manager-checklist>`                     |                          |
|                             |                                               | | :ref:`Technical Release Manager <technical-release-manager-checklist>` |                          |
+-----------------------------+-----------------------------------------------+--------------------------------------------------------------------------+--------------------------+
| **Release Day**             | Blocker bug fixes, Testing & Release          | | :ref:`Release Manager <release-manager-checklist>`                     |  0                       |
|                             | Announcements                                 | | :ref:`Technical Release Manager <technical-release-manager-checklist>` |                          |
+-----------------------------+-----------------------------------------------+--------------------------------------------------------------------------+--------------------------+

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
*  It is important to ensure that for each operating system, there is at least one person responsible for installing
   the conda version and one person responsible for installing the non-conda version of Mantid.

Beta Testing Ends - Prepare for Smoke Testing
#############################################

* Liaise with the technical release manager and project manager to decide on an appropriate time for Smoke Testing.
* Send an invite to developers for 1.5 hours maximum Smoke Testing. Include an introduction message to assign all testers to a certain operating system.
  Link to the release pipeline builds where the release packages *WILL* be. Encourage testers to download
  in the 30 minutes before smoke testing. Inform that ticking on a testing issue means that someone has assigned themselves and will tackle that task.

Smoke Testing
#############

*  Make sure to follow the preparation steps listed above.
*  It is likely that many changes have been made over the beta test period, therefore
   we must do some more manual testing to ensure everything still works. This stage is
   called Smoke testing. Generate the Smoke testing issues by following the instructions
   `here <https://github.com/mantidproject/documents/tree/main/Project-Management/Tools/RoadmapUpdate/SmokeTesting>`__.
*  Liaise with the Technical Release Manager and together announce the creation of the
   Smoke testing issues and Release Candidates in the *\#general* slack channel.
*  During smoke testing it may be easier if at least one QA Manager acts as facilitator during the session. They will answer questions, co-ordinate testing
   (especially when an arising issue needs testing on other OS) and ensure all testing is covered.


.. _release-editor-checklist:

Release Editors Checklist
-------------------------

**Role**: People responsible for editing the release notes and giving them a common
language, layout, and collecting images.

Beta Testing Begins
###################

* Initial amalgamation of the release notes:

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

  * Check for uncollected release notes using the `unused_release_note_finder.py script <https://github.com/mantidproject/mantid/blob/main/tools/ReleaseNotes/unused_release_note_finder.py>`__.
  * Check ``main`` for any release notes that have been merged into the wrong branch.
  * Remove any unused headings which have not already been removed from the release notes.
  * Remove all the "Used" release note files and their sub-structure.

.. _release-manager-checklist:

Release Manager Checklist
-------------------------

**Role**: Person in charge of the go/no go decision of the release. The main task
is to reiterate the timeline and be the collection point for information between
all of the Local Project Managers.

2-3 weeks before Feature Freeze
###############################

* Schedule a release showcase meeting for all facilities to present work that
  is intended to be part of the upcoming release.
  This meeting should aim to be in the week leading up to the feature freeze
  and include a timeline for the release along with a description of those
  taking on each of the release roles.
  It can also include a preview of work aimed for the release after the
  upcoming one.

1 week before Feature Freeze
############################

* Check that all people with release roles are added to the *\#release-roles*
  Slack channel and others are removed.
* Post on the *\#annoucements* slack channel reminding developers of the impending
  release and stating that they have only 5 days left before the feature freeze.
* Hold the release showcase meeting described above.

Feature Freeze Begins
#####################

* Ask the technical release managers to organize for the release branch to be created.
* Create a `project board <https://github.com/orgs/mantidproject/projects/>`__ to
  track the issues for the release
* After the message that the release branch has been created, post on the
  *\#annoucements* slack channel that only critical work should be merged to
  that branch. You can use an adapted version of the
  following announcement:

  * We are now in feature freeze. Only critical work should be added to the release-next branch.
    I have created a project board to track work for release *<version>* - please add any critical issues/PRs to this project, and ensure they have the *<version>* milestone.
    I will be clearing the *<version>* milestone from anything not in this project later today (you can always re-add it if necessary).
    Non critical work can be added to the *<version+1>* milestone and merged to the main branch as usual.

    *<project link url>*


*  Monitor the release project board and ensure items are assigned and moving through the board.
   Show the board at standups.

Manual Testing
##############

*  Ensure that PR testing has been completed for PRs from before the feature freeze.

**Clearing the Project Board**

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

*  On the date when the beta-testing is scheduled to start check with the technical
   release managers that a build has been successful that we are happy to put out.
*  Send an email to beta test users explaining where to download the installers and how
   to report issues (cc the Local Project Managers so they can organise a similar message
   at their facilities). The following emails are used for beta testing:

   ``ISISInstsci<AT>stfc.ac.uk``

   ``twg<AT>mantidproject.org``

   ``swg<AT>mantidproject.org``

   using the following templates as a guide (the first template is for users at ISIS, the second one for everyone else):

   * Dear all,

     We are busy making preparations for the release of version *<version>* of Mantid.
     We have completed our first round of developer testing and are now ready for beta-testing feedback.
     The beta testing period for this release is between today (*<start date>*) and the end of play on *<end date>*.
     We then hope to release the following week.

     Packages

     To test the Mantid nightly version, it is recommended to install it as a Conda package in a new Conda environment. To achieve this, use the
     following command: ``mamba create -n mantid_env_test -c mantid/label/nightly mantidworkbench``
     Alternatively, the nightly test installers for this version are available here to download: https://www.mantidproject.org/installation/index#nightly-build.
     The nightly builds install alongside a full release and so will not affect its operation but will overwrite any other nightly builds you have.
     For Windows users at ISIS, install Mantid as your standard user account (not an 03 account).
     It will install just for your user, rather than for the whole PC.
     Another possibility is to conduct testing on IDAaaS. Please be aware that the version on IDAaaS is typically one day behind the nightly version available on Conda.

     We have an early draft of the release notes at https://docs.mantidproject.org/nightly/release/<version>/index.html.

     Please report any bugs to ``mantid-help@mantidproject.org`` and
     if the problem is a bug that would prevent your normal workflow from working then start the email subject with ``URGENT:``.
     It would be most helpful for the team if bugs are communicated back to us as soon as possible.

     Thank you all for your help.

     Regards,

     Mantid Team

   * Dear all,

     We are busy making preparations for the release of version *<version>* of Mantid.
     We have completed our first round of developer testing and are now ready for beta-testing feedback.
     The beta testing period for this release is between today (*<start date>*) and the end of play on *<end date>*.
     We then hope to release the following week.

     Packages

     To test the Mantid nightly version, it is recommended to install it as a Conda package in a new Conda environment. To achieve this, use the
     following command: ``mamba create -n mantid_env_test -c mantid/label/nightly mantidworkbench``
     Alternatively, the nightly test installers for this version are available here to download: https://github.com/mantidproject/mantid/releases.
     The nightly builds install alongside a full release and so will not affect its operation but will overwrite any other nightly builds you have.

     We have an early draft of the release notes at https://docs.mantidproject.org/nightly/release/<version>/index.html.

     Please report any bugs to ``mantid-help@mantidproject.org`` and
     if the problem is a bug that would prevent your normal workflow from working then start the email subject with ``URGENT:``.
     It would be most helpful for the team if bugs are communicated back to us as soon as possible.

     Thank you all for your help.

     Regards,

     Mantid Team

* Ensure the other facilities forward the beta-testing email to their relevant internal lists.

Reminder Email for Beta
#######################

*  Mid-way through the beta testing period, send a reminder email to beta test users thanking them for their feedback so
   far and reminding them to feedback as soon as possible and not to send in a list of
   issues at the end of testing (cc the Local Project Managers so they can organise a
   similar message at their facilities).

Beta Testing Ends
#################

*  At the end of the day, email the beta test users thanking them.
*  Review the complete set of release notes to make sure there are no glaring mistakes.

Smoke Testing
#############

* This is the final day for code changes to the build for blocker issues.

.. _release-manager-announcements:

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

**Create the Release Branch (once most PRs are merged)**

* Ensure the latest `main nightly deployment pipeline
  <https://builds.mantidproject.org/view/Nightly%20Pipelines/job/main_nightly_deployment/>`__
  has passed for all build environments. If it fails, decide if a fix is needed before moving on to
  the next steps.
* Ask a mantid gatekeeper or administrator to update the ``release-next`` branch so that it's up to
  date with the ``main`` branch, pushing the changes directly to GitHub:

.. code-block:: bash

    git checkout release-next
    git fetch origin main
    git reset --hard origin/main
    git push origin release-next --force

* Verify that the latest commit on ``release-next`` is correct before moving to the next step.
* Click ``Build Now`` on `open-release-testing
  <https://builds.mantidproject.org/view/All/job/open-release-testing/>`__. This will
  set the value of the Jenkins global property ``BRANCH_TO_PUBLISH`` to ``release-next``,
  which will re-enable package publishing for the ``release-next`` nightly pipeline.
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
* Ensure that all changes, including release notes, have been merged into the ``release-next`` branch.
* Make sure the ``release-next`` branch is fully merged into ``main``. If required, manually run the `GitHub workflow
  <https://github.com/mantidproject/mantid/actions/workflows/automerge.yml/>`__ using the ``release-next`` branch to
  merge the changes.
* Run the `close-release-testing <https://builds.mantidproject.org/view/All/job/close-release-testing>`__ Jenkins job.
  This will set the value of the Jenkins global property ``BRANCH_TO_PUBLISH`` to ``main``, which will re-enable package
  publishing for the ``main`` nightly pipeline.

**Create the Release Candidates For Smoke Testing**

We are now ready to create the release candidates for Smoke testing.

* Build the `release-next_nightly_deployment Jenkins pipeline
  <https://builds.mantidproject.org/view/Nightly%20Pipelines/job/release-next_nightly_deployment/>`__
  with the following parameters (most are already defaulted to the correct values):

  * set ``BUILD_DEVEL`` to ``all``
  * set ``BUILD_PACKAGE`` to ``all``
  * set ``PACKAGE_SUFFIX`` to an **empty string**
  * tick the ``PUBLISH_TO_ANACONDA`` checkbox
  * tick the ``PUBLISH_TO_GITHUB`` checkbox
  * set the ``ANACONDA_CHANNEL`` to ``mantid``
  * set the ``ANACONDA_CHANNEL_LABEL`` to ``vX.Y.ZrcN`` where ``X.Y.Z`` is the release number,
    and ``N`` is an incremental build number for release candidates, starting at ``1`` (e.g. ``v6.7.0rc1``)
  * set ``GITHUB_RELEASES_REPO`` to ``mantidproject/mantid``
  * set ``GITHUB_RELEASES_TAG`` to ``vX.Y.ZrcN``, where ``X.Y.Z`` is the release number,
    and ``N`` is an incremental build number for release candidates, starting at ``1``.

* Once the packages have been published to GitHub and Anaconda, ask someone in the ISIS core or DevOps
  team to manually sign the Windows binary and re-upload it to the GitHub
  `draft release <https://github.com/mantidproject/mantid/releases>`__.
* Liaise with the Quality Assurance Manager and together announce the creation of the smoke testing
  issues and Release Candidates in the *\#general* slack channel.


.. _technical-release-manager-release-candidates:

Create Final Release Candidates
###############################

Check with the Quality Assurance Manager that the Smoke testing has been completed, and any issues
have been fixed. Additionally, ensure that the version of the `mslice` package in conda_build_config.yaml is correct.
If there have been any updates to MSlice since the last release, it must be released first. The release candidates must
now be recreated with their final version numbers. To do this, build the
`release-next_nightly_deployment Jenkins pipeline
<https://builds.mantidproject.org/view/Nightly%20Pipelines/job/release-next_nightly_deployment/>`__
with the following parameters (most are already defaulted to the correct values):

* set ``BUILD_DEVEL`` to ``all``
* set ``BUILD_PACKAGE`` to ``all``
* set ``PACKAGE_SUFFIX`` to an **empty string**
* tick the ``PUBLISH_TO_ANACONDA`` checkbox
* tick the ``PUBLISH_TO_GITHUB`` checkbox
* set the ``ANACONDA_CHANNEL`` to ``mantid``
* set the ``ANACONDA_CHANNEL_LABEL`` to ``draft-vX.Y.Z`` where ``X.Y.Z`` is the release number
* set ``GITHUB_RELEASES_REPO`` to ``mantidproject/mantid``
* set ``GITHUB_RELEASES_TAG`` to ``vX.Y.Z``, where ``X.Y.Z`` is the release number.

.. _technical-release-manager-release-day:

Release Day
###########

Once the final release candidate pipeline has succeeded, the draft release will be available on GitHub and our
Anaconda channel.

*  Edit the `draft release <https://github.com/mantidproject/mantid/releases>`__ on
   GitHub. The description of the new release can be copied from the release notes ``index.rst`` file, and
   edited appropriately. See previous release descriptions for inspiration.
*  Ask someone in the ISIS core or DevOps team to manually sign the Windows binary and re-upload it to the draft
   release.
*  Publish the GitHub release. This will publish the tag required to generate the DOI.
*  Remove the smoke testing release from the GitHub releases page (the one tagged with ``vX.Y.ZrcN``).
*  Change the labels for the release candidates on our `Anaconda site <https://anaconda.org/mantid/mantid/files>`__
   from ``draft-vX.Y.Z`` to ``main``. You may need to ask a member of the ISIS core or DevOps team to do this.
*  Remove the smoke testing release candidates from our Anaconda channel (those with the ``vX.Y.ZrcN`` label).
*  Update the `download page <https://download.mantidproject.org>`__ by running the `Update latest release links
   <https://github.com/mantidproject/www/actions/workflows/update-latest-release.yml>`__ workflow in the
   `mantidproject/www repository <https://github.com/mantidproject/www>`__.
*  Ask someone with access to the `daaas-ansible-workspace repository
   <https://github.com/ral-facilities/daaas-ansible-workspace>`__ (a member of the ISIS core team or IDAaaS support)
   to add the new release to IDAaaS. They can do this by creating a PR targeting the ``preprod`` branch, adding
   the new release version to the list of versions installed on IDAaaS `here
   <https://github.com/ral-facilities/daaas-ansible-workspace/blob/main/roles/software/analysis/mantid/defaults/main.yml>`__.
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

*  The script will prompt you for the password. Ask a senior developer to share the username and
   password with you if you do not already have access to it.

**Update Citation File**

Open a PR updating the software ``doi``, ``date-released`` and ``version`` in the ``CITATION.cff`` file
at the root of the repository.

Notify the Release Manager when you complete all your tasks.

**Deploy Versioned Documentation**

Versioned documentation is accessible at https://docs.mantidproject.org/vX.Y.Z/.
This documentation is hosted at https://mantidproject.github.io/docs-versioned/vX.Y.Z/.
Documentation is deployed to GitHub via an action on the `docs-versioned <https://github.com/mantidproject/docs-versioned>`__ repository.
This action runs on a push to the ``main`` branch of the repository.

To do this:

* On a clone of the mantid repository, check out the commit tagged as the relevant release number: ``git checkout tags/<vX.Y.Z> -b <new branch name>``.
* On this branch, build the ``docs-html`` target (this target is produced by ``CMake``).
* Clone the repository: https://github.com/mantidproject/docs-versioned.
* Remaining on the ``main`` branch, create a directory for the relevant release in the form ``vX.Y.Z``.
* Copy the built documentation into this new directory. The built documentation will be in your mantid build directory at ``<build directory>/docs/html``.
* Stage the newly created directory and commit it to your branch.
* After double-checking that these instructions have been followed correctly, push your branch to the main repository to deploy.
