.. _Gatekeeping:

==========================
Gatekeeping a Pull Request
==========================

The final step in our :ref:`development workflow <GitWorkflow>` after a :ref:`pull request has been reviewed <ReviewingAPullRequest>` is gatekeeping. This is second review stage where a senior developer / code owner does a final check of the work before merging it to the ``main`` or ``release-next`` branch.

Purpose
=======

This document offers some guidance for gatekeepers as to what to look for and some quirks of Mantid to be aware of that the first reviewer may have missed. There are 3 main purposes to gatekeeping:

* **Audit:** A detailed code review is generally not required as long as the first stage reviewer has done a thorough job - one of the main purposes of the gatekeeping role is to ensure that this is the case, i.e. that the review process has been followed correctly and all of the appropriate checks have been made.
* **Code quality:** The other main purpose is to ensure all code changes have been seen by an experienced developer. We very much encourage new/junior team members to take part in first-stage reviews, so having this second stage ensures we have a chance to catch things that a newer team member may not be aware of.
* **Knowledge sharing:** An additional benefit of the process is the opportunity for more experienced developers to offer advice and spot learning and development opportunities for individuals and/or things the whole team could benefit from. Depending on your capacity and interest in this do feel free to offer tips and/or feed into team processes or training materials as you think appropriate.


With great power comes great responsibility
===========================================

*Be very careful not to push to* ``main`` or ``release-next``

* When you are made a gatekeeper on the project you will be given **full, unprotected access to the** ``main`` **branch**. This means you lose the normal protection that stops you being able to push directly to the ``main`` branch - you therefore need to be careful at all times that you do not accidentally push to ``main``, e.g. when you are using Git in your terminal or Git GUI.
* You should be enough of a Git expert to know the implications of directly pushing to a branch, including being able to overwrite a branch completely with the history of another branch, which of course would be pretty catastrophic if done to ``main``. You should have solid working practices with Git that ensure you are never likely to do this. Working on a fork is recommended, and some of the :ref:`recommended config <GitConfig>` regarding default behaviour for ``git push`` will also help (although recent versions of Git are much safer by default in this regard so in reality you are unlikely to have problems).
* Of course, you should never deliberately make any changes directly on ``main``, either via Github or a direct push to ``main`` - always raise a pull request regardless of how minor the change is otherwise you run a high risk of it not passing the CI tests and breaking the build for all developers.


Performing a merge on Github
============================

*The* ``Merge pull request`` *button*

* Generally we perform the default merge which is to create a merge commit, so you can just click ``Merge pull request``.
* This button should be green if all checks have passed and there is an approved review. The button will be red if this is not the case but you have the power to override it and merge anyway - it is not recommended to do this but very occasionally may be needed if e.g. it is urgent and being held up by an unreliable test you are certain is unrelated. If you do need to do this, make sure you add a clear comment describing why.
* Occasionally you might want to use ``Squash and merge`` from the drop-down menu, but typically this is only done if the developer specifically asked for it (although feel free to ask the developer if you can squash if you think that's more appropriate).
* When you click the button you will get the chance to edit the commit message before committing. Typically we leave the commit title as per the default text so it is clear it is a merge, and paste the PR title into the body of the commit message. (Any other useful information can also be added here.)
* Click ``Confirm merge`` to merge the commit to the destination branch.
* If a PR is approved but still finishing its CI checks then the button will read ``Enable auto merge``. Feel free to use this if you are happy for the PR to be automatically merged once it passes all the CI checks.


Don't panic
===========

There is a lot here to consider but bear in mind that you are not expected to catch everything. Things will always slip through and we have good processes in place to catch them, and we can always revert changes if necessary. You have been invited to be a gatekeeper because you have valuable knowledge that we think the rest of the team can benefit from, so please share what you can :)


Things to check
===============

Tips
----

* Generally I will start with a quick scan to the bottom of the comments to check the reviewers final comment - often this can inform what else to look for and save time (and it is good to encourage developers to write a concise summary of everything considered in the review in a final comment to make the gatekeeping process quicker).
* Have a quick scan to see what files are changed - this informs how widespread the change is. Use tools to help - Github has a side bar that displays a file hierarchy; Octotree is a useful plugin that also shows the number of lines changed in each file, and the Pro version has some nice functionality to show which files you've marked as viewed.
* Be aware that other gatekeepers may be looking at PRs at the same time as you, so if you think you will be spending any significant time doing your review, assign yourself as a reviewer and/or put a comment on it so that other gatekeepers know not to look at it (particularly if you think you might need to ask for changes, because another gatekeeper might merge it in the meantime!).


Destination branch
------------------

*Be aware of where we are in the development/release cycle*

* Check that the destination branch is correct. During development sprints, the destination branch should be ``main``. During the release period, it may be ``main`` or ``release-next`` - ensure you are familiar with the release process and know the differences.
* Be more picky in the run up to release freeze - if large or risky changes are being made very close to release, question whether they can wait till the next release.
* The milestone should be set appropriately for the destination branch. If either doesn't look correct, query it.
* Use extra caution when merging into ``release-next``, particularly after release freeze when we should only be merging things that are critical and are considered "safe" fixes. The later we get into the release sprint, the more picky we need to be. If a PR can wait till the next release, get the developer to change the destination branch to ``main``.
* If we are on very close to releasing, further merges to ``release-next`` may miss the build and be lost entirely. Check with the release manager.


General checks
--------------

* Check that the PR description and testing instructions are clear.
* Check that the reviewer seems to have done a thorough job on the functional testing and code review - feel free to ask if not clear.
* Check that everything has been added/updated if applicable, e.g. release notes, unit tests, system tests, documentation, developer documentation, unscripted testing documentation.
* Check that all reviewer comments have been addressed - in particular if there are additional comments from someone who is not the approver, because these can easily get missed after one person approves. Also check if any additional reviewers have been requested who have not commented.
* If a PR makes changes that look like they need scientific validation it might be worth querying what has been done or what plans are in place for this.
* Check the list of changed files looks sensible - sometimes developers mistakenly ``git add`` things by mistake e.g. local file changes, temp files etc. Query anything that looks out of place.


Testing
-------

* Check that all functional code changes are covered by unit tests (this might be existing tests or new tests might be required with the change).
* Check that functional testing has been included in the PR description and is clear and is sufficient. Developers will often state that it just needs to pass the automated tests, or that just code review is required. However, any change that affects functional code should have some way of manually testing it so I would normally expect to see something here unless there is a good reason otherwise.


Code quality
------------

* Have a brief scan of some of the changes to check that they adhere to standards.
* Consider any design decisions that have been made and if these are appropriate and not breaking any obvious design patters, e.g. it is common to see MVP being broken where logic is added to the view and is not testable; the lack of tests can be an indicator here. Is there any pollution of e.g. Qt in a non-Qt class; again this may make testing more difficult/complicated.
* Have a brief check that any tests look to be of good quality; unit tests should be clear and test things in isolation. Check for appropriate use of mocks.
* Keep an eye out for any other code smells - unclear tests, files/classes too long, long comments etc.


Deeper checks
-------------

A more thorough code review may be required in some situations, e.g.:

* the change is to core components or a wide range of components;
* the ``Needs attention`` label has been applied;
* the reviewer is more junior or unfamiliar with the area of code;
* the change involves considerable design changes/additions.

A quick scan of the files changed can indicate whether the change is confined to a particular component or more widespread.


Version control
---------------

While not vital and often not something I'd ask for changes on, it is good to keep an eye on how clean and informative developers are keeping their Git commit history and offer tips, particularly to new developers, to help them improve their processes. This is sometimes the only chance to help some sole developers realise they could be working in a much more efficient manner. Things you might want to look for and offer advice on are:

* Nice clean history, in particular avoiding merge commits from ``main`` or the remote feature branch into the local feature branch - recommend rebase instead, and setting up config so this is done automatically.
* Clear commit messages, with a short title and more detailed message body if appropriate.
* Commit messages that clearly describe the actual changes - try to discourage comments to reference the review such as "add changes from review" and encourage describing the actual changes instead.
* Commits that nicely encapsulate increments of work - if developers are making multiple subsequent commits that should logically be part of a previous commit encourage use of ``amend`` and ``fixup``.
* Using ``#re`` or ``#refs`` in the comment where appropriate (either title or body; we have a mix of styles but a developer should pick one and be consistent).

.. _FixProtectedBranchMergeConflict:

Fixing a merge conflict between ``main`` and a protected branch
===============================================================

There may occasionally be a merge conflict when the automated "Merge protected branches" workflow attempts to merge a protected branch into ``main``. The following instructions detail how to fix the conflict, using the ``release-next`` branch as an example:

From a fork
-----------

Assuming your fork has the following remote setup

.. code-block:: bash

    $ git remote -v
    mantid  https://github.com/mantidproject/mantid.git (fetch)
    mantid  https://github.com/mantidproject/mantid.git (push)
    origin  https://github.com/<username>/mantid.git (fetch)
    origin  https://github.com/<username>/mantid.git (push)


then you can follow these instructions to fix the merge conflict:

.. code-block:: bash

    git fetch --all
    git checkout release-next
    git pull mantid release-next
    git checkout main
    git pull mantid main
    git checkout -b 0-fix-conflicts
    git merge release-next

You should then fix the merge conflicts, git add and commit the changes. Then push the branch to the ``mantid`` remote repository and open a PR. The PR should be reviewed and then merged into ``main``.

From a non-fork
---------------

Assuming you have the following remote setup

.. code-block:: bash

    $ git remote -v
    origin  https://github.com/mantidproject/mantid.git (fetch)
    origin  https://github.com/mantidproject/mantid.git (push)


then you can follow these instructions to fix the merge conflict:

.. code-block:: bash

    git fetch --all
    git checkout release-next
    git pull origin release-next
    git checkout main
    git pull origin main
    git checkout -b 0-fix-conflicts
    git merge release-next

You should then fix the merge conflicts, git add and commit the changes. Then push the branch to the ``origin`` remote repository and open a PR. The PR should be reviewed and then merged into ``main``.

Specific quirks of Mantid
=========================

There are often multiple and/or confusing ways to do things in Mantid and it is not always clear which is the right approach, particularly when there is a lot of legacy code following old outdated approaches. This section attempts to point out some of the common areas where there can be pitfalls. There is no direct advice here, but if you see changes that include these things and don't look ideal then it might be worth digging deeper.

Algorithms
----------

* Workflow algorithms should only modify workspaces via calls to child algorithms; if you see one directly manipulating data in a workspace it is probably breaking the intention here and could have implications where the workspace history will be incomplete. There are already examples of this in Mantid so developers might follow the same pattern not realising that this is not the intended way of using workflow algorithms.
* If algorithm outputs change it may be worth checking if the algorithm should be versioned if users will still require the old results to be reproducible.


Workspaces
----------

* Use of the ADS can be inconsistent. A lot of code uses it unnecessarily and workspaces are passed around by name assuming they will be in the ADS when actually this might be unnecessary and cause problems with algorithm history if a workspace doesn't exist. Some algorithms have hacky code that dumps interim outputs into the ADS directly rather than using input/output properties, which again can cause problems with the history.
* Workspace history is often overlooked in testing and can be easily messed up by mis-use of workflow algorithms and mis-use of ADS and input/output workspace properties. Note however that history doesn't always work reliably anyway though.


Workspace Groups
----------------

* Workspace groups are not handled particularly well in various parts of the code. In particular developers often forget to test with them and the workspace history can be confusing/incorrect.


IDFs and IPFs
-------------

* These changes take effect as soon as they are merged to ``main`` - ensure the developer and their user(s) are aware of this. They may want to decouple these changes from code changes, and ensure backwards compatibility. Release notes may or may not be helpful (the changes are not tied to a release so in that case could be confusing, but some people prefer to add them because at least then the changes do get higlighted to users; it depends somewhat on the change and the audience).


Unit tests
----------

* The main thing to look out for is that unit tests are small and test a minimal piece of functionality. Often newer developers might add tests that do a full reduction on real data and just checks some output numbers, which can be unclear what a correct output looks like. These might be better as system tests and/or it might be better for them to create dummy data and make the tests smaller and clearer.
* Unit tests should be very quick, ideally under 1 second; push back if the developer is adding slower-running tests without a clear justification.
* Calling algorithms in tests requires initialising framework. This is sometimes necessary but often it is better to use mocks.
* Calling Python algorithms from C++ tests is not possible - either mock or create Python tests (calling C++ algorithms from Python works).
* Creating Qt objects in tests can often cause problems - ideally MVP should mean we can mock out view components and not need Qt at all in unit tests, although sometimes it is hard to avoid without a big refactor. Some creation of Qt components also needs testing but this is often better done as system/integration type tests.


How do people become gatekeepers?
=================================

Gatekeepers are selected by the technical working group and/or local team leaders. We aim to have a good balance of gatekeepers from each contributing facility where possible.
