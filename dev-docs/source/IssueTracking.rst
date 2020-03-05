.. _issue_tracking:

=====================
Mantid Issue Tracking
=====================

Mantid uses GitHub issues to track new feature and bugfix
development. The issue tracker can be found at `GitHub
<https://github.com/mantidproject/mantid/issues>`_

.. contents:: Contents
    :local:

Workflow
^^^^^^^^

Every piece of work starts with an open issue. To find issues that are
assigned to you see the section
:ref:`IssueTrackingYourIssues`. How an issue is closed is
then dependent on its requirements:

- Code changes required: generate a **pull request**
  (:ref:`GitWorkflowPullRequests`)

  - Make sure the issue is referenced with ``#issue-number``. It will
    then be closed when the pull request is merged
- No code changes required: Close the issue, with a comment explaining
  why the issue should be closed, for example:

  - This is a duplicate of #1234
  - This works for me, when I try the following steps ....
  - I've spoken to the author of this issue and it does not happen for
    them either anymore
  - Contacted the instrument scientists that raised this issue and it
    has been solved since version 3.4
  - After further investigation we are not going to continue with this
    for the following reasons ... The author of this ticket has been
    informed.
  - (If the issue is an **umbrella** issue, indicated by the
    **Roadmap** label) All points in this issue have been addressed
    elsewhere (see issues X, Y, Z)

Creating an Issue
^^^^^^^^^^^^^^^^^

Go `here <https://github.com/mantidproject/mantid/issues/new>`__ to
create a new issue. Make sure you assign the appropriate labels (see
:ref:`IssueTrackingLabels`) and milestone.

.. _IssueTrackingLabels:

Labels
^^^^^^

GitHub allows you to assign various labels to your issues and pull
requests. Labelling your issues and pull requests adequately will
ensure that they are picked up by the right people.

Labels fall into several groups: (*please note that 'ticket' is used
to refer to either an issue or a PR on this page*)

- **Component** - which area of Mantid the ticket concerns. The ticket
  may be relevant to more than one area. Some commonly used
  **Component** labels are:

  - **Framework**: tickets relating to Algorithms, Workspaces and
    SimpleAPI
  - **GUI** - tickets relating either to the main MantidPlot GUI, or
    one if its custom interfaces
  - **Python** - whether the change will be implemented in
    Python. This is useful for labelling PRs, as branches in which all
    changes are in Python often don't need to be rebuilt to be tested,
    so can be picked up more quickly
- **Group** - you may belong to a group within Mantid, such as the
  Reflectometry group. Use the corresponding label so that others in
  your group can easily find your work to review it
- **Misc** - some of the more common **misc** labels are described
  below:

  - **Bug** - used for issues or PRs which reference a specific bug in
    the software (as opposed to a new feature, or general
    improvements)
  - **Induction** - if you come across an issue that may be good for a
    new starter (not too difficult, low risk, the relevant code is
    comprehensively tested, low(ish) priority) don't hog it for
    yourself! Put this label on it so that it can be picked up as an
    introduction to the Mantid codebase
  - **Roadmap** - useful when planning out a large piece of work - use
    this to map out the steps you'll need to take along the way. This
    will usually subsume several smaller issues
- **Priority**

  - **Priority: High** - use this if your ticket needs picking up
    right away, for instance if it relates to a bug which affects a
    large number of users. If you are unsure about an issue that could
    be high priority, discuss with a senior developer. Issues marked
    high priority must also have a justification for the priority 
    stated in the ticket
  - **No priority set** - use this for tickets of intermediate priority
    this will probably be your default setting, most issues will fall 
    into this category
  - **Priority: Low** - use this for the "nice to have" tickets
    this means that they are not necessarily urgent but can be
    worked on if there is spare time
- **Patch candidate** - following a release, low-risk tickets with
  high impact on users will be considered for a follow-up (or *patch*
  release). If your ticket matches this description, consider
  labelling it as a patch candidate
- **Quality** - not all of Mantid is well-written. Use one of these
  labels if your ticket involves improving the quality of the Mantid
  codebase (these changes will usually be invisible to users)
- **State: In Progress** - use this label on a PR if the work in that
  PR is still ongoing, and you don't want a review yet. **REMEMBER TO
  REMOVE IT ONCE YOUR WORK IS READY FOR REVIEW**


.. _IssueTrackingZenHub:

ZenHub
^^^^^^

Using the ZenHub browser extension set an estimate, this should be the number of 
days you expect to take from begining to work on an issue, to opening a pull-request about it.
Once you actually begin working on the issue change the pipeline to In Progress, 
then again to Review/QA when you open a pull-request for it. This will be used to to aid with 
future estimations of time for an issue.


Filtering Issues
^^^^^^^^^^^^^^^^

GitHub has a powerful issue filtering system that is described `here
<https://help.github.com/articles/searching-issues>`__. Below we list
some common searches that developers will need. It is advised to
bookmark these URLs.

.. _IssueTrackingYourIssues:

Your Issues
-----------

You can view the issues assigned to you by visiting the `list of
issues <https://github.com/mantidproject/mantid/issues>`_, clicking on
the assignee drop down box and finding your name.

For Review
----------

These are useful links to view when you are looking to review/test an
issue or pull request:

- Go `here
  <https://github.com/mantidproject/mantid/pulls?utf8=%E2%9C%93&q=-author%3AGITHUB-NAME-HERE+is%3Apr+is%3Aopen+-label%3A%22State%3A+In+Progress%22+no%3Aassignee+status%3Asuccess>`__
  for pull requests that you did not create and no one else is
  assigned. Please replace GITHUB-NAME-HERE with your GitHub username
- Go `here
  <https://github.com/mantidproject/mantid/issues?utf8=%E2%9C%93&q=-assignee%3AGITHUB-NAME-HERE+is%3Aissue+is%3Aopen+label%3A%22State%3A+Review+Required%22+>`__
  for issues with no code changes to review. Please replace
  GITHUB-NAME-HERE with your GitHub username
