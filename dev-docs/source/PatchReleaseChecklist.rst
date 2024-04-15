.. _PatchReleaseChecklist:

=======================
Patch Release Checklist
=======================

.. contents::
  :local:

These are the steps involved in performing a Mantid patch release. To
perform a full release see :ref:`ReleaseChecklist`.

Request
#######

*  Anyone may request a patch release, but that request must be initially
   approved by one of the local PMs.

Authorisation
#############

*  The Technical Working Group must meet to authorise the patch release.
*  During the meeting other high value, low impact changes may be
   considered for inclusion for the release. Any that are to be included
   must be added to the patch release notes.
*  The Project Manager will create a new milestone in github, and all
   tickets to be included must be moved to that milestone.
*  A developer will be nominated to be the main reviewer and compiler of
   the patch.

Development
###########

The patch release will be prepared based off the tag used to mark
the last minor release. A branch called ``release-next`` will be created from this tag.
Normally, the ``release-next`` branch will already be pointing to the correct commit.
Verify that this is the case, and if not, update the branch so that it is.
Changes for the patch should be incorporated into the release branch by either of the following methods:

*  If changes have already been merged into ``main``, the commits should be cherry-picked into the release
   branch (see :ref:`Cherry Picking <cherry_picking>`)
*  Any changes that have not yet been merged into ``main`` can be rebased so that the pull request targets
   ``release-next``. When they are merged, the changes will be automatically merged into ``main``.

Issues and pull requests should then have the ``PatchCandidate`` label applied to them.

Release Notes
-------------

The main reviewer should create a skeleton set of patch release notes on the release branch
using the `python helper tool <https://www.github.com/mantidproject/mantid/blob/main/tools/release_generator/patch.py>`__.
For example:

.. code-block:: bash

    python release_generator/patch.py --release 6.9.1 -p 37033 37047 37014 37016 36935

where the numbers after the ``-p`` argument are a list of existing pull requests to be included in the patch release.
Any future pull requests will need to be manually added to the release notes.
You will need to move the generated file to a `new vX.Y.Z directory <https://github.com/mantidproject/mantid/tree/main/docs/source/release>`__
and add it to the `release notes index <https://github.com/mantidproject/mantid/blob/main/docs/source/release/index.rst>`__.
Note that the `automerge <https://github.com/mantidproject/mantid/blob/main/.github/workflows/automerge.yml>`__ GitHub
action will probably fail with a conflict in the main index file. This will need to be resolved manually.


.. _cherry_picking:

Cherry Picking & Code Review
----------------------------

It is the job of the main reviewer of the release to review each
issue/pull request marked ``PatchCandidate`` and decide if the risks of
the changes are low enough to include in a release that will not
undergo full beta testing by scientists. New pull requests that target
``release-next`` can simply be merged provided that they add the appropriate
release notes. Existing pull requests that have already been merged into ``main``
should have their commits cherry-picked into the ``release-next`` branch,
either directly or via a new pull request branch. One advantage of creating
a new pull request branch is that you can ask the commit authors to verify
that all of the relevant commits have been added. For each of the ``PatchCandidate``
pull requests that were not merged directly into ``release-next``:

*  find the list of commit ``SHA1`` values in that pull request
*  check if any of these commits has altered the release notes for the
   next major release
*  if not then pass all commits in the order oldest->newest to
   ``git cherry-pick -x``
*  if release notes were modified in a commit on their own then pass all
   commits except this one in the order oldest->newest to
   ``git cherry-pick -x``
*  if a commit has a mixture of code/release note changes then:

   *  pass the list of commits up to but not including this commit to
      ``git cherry-pick -x``
   *  now pass this single commit to ``git cherry-pick -x -n`` and it
      will not make a commit. Remove the major release note changes and
      commit ``git add``/``git commit``
   *  if there are any remaining commits after this then pass them to
      ``git cherry-pick -x`` as before.

*  finally add a commit that updates the patch release notes with this
   pull request link and summary description.

Once cherry picked the milestone of the original pull request should be
updated to the patch milestone.

Nightly Builds
##############

The `release-next nightly pipeline <https://builds.mantidproject.org/view/Nightly%20Pipelines/job/release-next_nightly_deployment>`__
job checks for changes on the current release branch each night (00:00 GMT) and should
be monitored during the patch release period to check for any failures.

Release Procedure
#################

Once all the changes have been merged into ``release-next``, and the release notes
are complete, it is time to release the patch by performing the following tasks:

*  Build the release candidates by following the instructions to :ref:`technical-release-manager-release-candidates`.
*  After the release candidates have built successfully, ask the development team perform unscripted testing,
   with a focus on the areas that were modified for the patch release.
*  When you are happy with the quality of the release candidates, follow all of the
   :ref:`technical-release-manager-release-day` instructions to publish the packages.
*  Once packages are published, the Project Manager must announce the patch release by following the
   :ref:`release-manager-announcements` instructions.
