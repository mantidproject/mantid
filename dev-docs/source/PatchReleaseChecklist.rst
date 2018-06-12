.. _PatchReleaseChecklist:

=======================
Patch Release Checklist
=======================

.. contents::
  :local:

These are the steps involved in performing a Mantid patch release. To
perform a full release look see :ref:`ReleaseChecklist`.

Request
#######

*  Anyone may request a patch release, but that request must be intially
   approved by Project manager (Nick) or one of the team leaders (Pete
   or Mattieu).

Authorisation
#############

*  The Project Manager and Team leaders must meet to authorise the patch
   release.
*  During the meeting other high value, low impact changes may be
   considered for inclusion for the release. Any that are to be included
   must be added to the patch release notes.
*  The Project Manager will create a new milestone in github, and all
   tickets to be included must be moved to that milestone.
*  A developer will be nominated to be the main reviewer and compiler of
   the patch.

Development
###########

The patch release will be prepared based off the branch used to
construct to most recent major point release, e.g. ``release-v3.9``
would be used for any ``3.9.x`` patches. Changes for the patch should be made using the standard GitHub
workflow for merging code with ``master``. The issue and pull request should then have the ``PatchCandidate`` label applied to them. These
commits will then be cherry picked from ``master`` on to the release branch.

Release Branch
##############

The release branch will currently have its version fixed to exact
version of the last major/patch release. It is not a requirement but
advised to unfix the patch number while the patch is being compiled.
This prevents the nightly builds from generating a collection of packages that have
exactly the same version. The patch number can be unfixed by commenting the line in
https://www.github.com/mantidproject/mantid/blob/release-vX.Y/buildconfig/CMake/VersionNumber.cmake#L9, where
``X.Y`` should be replace with the appropriate numbers.

Release Notes
-------------

Once the patch version has been unfixed the main reviewer should
create a skeleton set of patch release notes on the release branch
using the `python helper tool <https://www.github.com/mantidproject/mantid/blob/master/tools/release_generator/patch.py>`__.

Cherry Picking & Code Review
----------------------------

It is the job of the main reviewer of the release to review each
issue/pull request marked ``PatchCandiate`` and decide if the risks of
the changes are low enough to include in a release that will not
undergo full beta testing by scientists. If it is acceptable then on the release branch for each pull request:

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
   pull request link and description in the table.

Once cherry picked the milestone of the original pull request should be
updated to the patch milestone.

Nightly Builds
##############

The `release pipeline <release-pipeline>`_ contains jobs
that check for changes on the current release branch each night (00:00 GMT).
Any detected changes will cause a clean build of the code followed by a run
of the system tests. The Linux clean builds should have the `PACKAGE_SUFFIX` set
to `nightly` while testing the patch.

These jobs should be checked each morning to confirm that everything is green.

Release Day
###########

On the day of release a few steps are required:

* update the patch version:
* navigate to
  https://www.github.com/mantidproject/mantid/blob/release-X.Y./buildconfig/CMake/VersionNumber.cmake,
  where ``X`` & ``Y`` are the major and minor release versions
  respectively.
* edit the ``VERSION_PATCH`` to the required number for the patch and
  commit the result.
* run a manual build of all of the OS jobs under {{
  site.mantidreleasebuilds }} and when asked for a suffix use an empty
  string
* wait for the builds to finish (will take more than 1 cup of
  tea/coffee/beverage of choice)

While waiting for the builds create a new release on GitHub, using a tag
of the form ``v.X.Y.Z`` and populate with information from the release
notes (see a previous version of the format).

Once the builds complete have the development team run unscripted
testing on the packages generated by the clean release builds. In
particular the issues intended to be fixed should be tested.

Once the testing has passed:

* Use the manual deploy job at `release pipeline <release-pipeline>`_ to deploy
  packages and documentation to the public web.
* The windows binary will **not** be deployed and must be signed by
  someone at ISIS and uploaded to sourceforge manually
* Put packages on GitHub
* RHEL 7 only: Build the suffix-package ``mantidXY`` by running another
  clean RHEL 7 build from the `release pipeline <release-pipeline>`_ but use the
  suffix XY, where ``X`` is the major version and ``Y`` is the minor
  version (currently used at SNS)
* Have someone at the SNS follow the instructions
  `here <http://www.mantidproject.org/Fermi_cluster_at_ORNL>`__ to
  deploy an MPI version of the patch release.
* Create new DOI using the scripts in the codebase and instructions on
  :ref:`release checklist <ReleaseChecklist>`.
* Send an email, including the text of the release notes, to the
  following lists
* ``mantid-announce@mantidproject.org``
* ``mantid-developers@mantidproject.org``
* ``nobugs@nobugsconference.org``
* ``news@neutronsources.org``
* ``neutron@neutronsources.org``
* Add topic to the news page on the `forum <http://forum.mantidproject.org/>`__
* Close the release milestone in github
* Remove the patch candidate tag from pull requests (if not already done)

.. Link definitions

.. _release-pipeline: http://builds.mantidproject.org/view/Release%20Pipeline/
