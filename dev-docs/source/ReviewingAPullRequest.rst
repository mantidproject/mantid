.. _ReviewingAPullRequest:

========================
Reviewing a Pull Request
========================

An important step in our :ref:`development workflow <GitWorkflow>` is the testing of individual issues/tickets after the development on them is complete, and before the code is merges into the main branch. Developers pick one from `the list <https://github.com/mantidproject/mantid/pulls>`_ of completed issues and perform a number of verification steps on it. The mechanics of testing a pull request (e.g. the git commands to use) are described :ref:`here <GitWorkflow>`. This page is concerned with the aspects that should be considered in deciding whether a pull request should be recommended to merge or sent back to the developer for further work. *There should be very little reluctance to reopen a ticket even for minor issues.*

Code Review
===========

The code changes should be manually reviewed (the github compare view is ideal for this). A couple of pieces on the value of code review can be found at `scientopia <http://scientopia.org/blogs/goodmath/2011/07/06/things-everyone-should-do-code-review>`_ and `codinghorror <http://www.codinghorror.com/blog/2006/01/code-reviews-just-do-it.html>`_.

* The primary aim is to find bugs that the developer and tests so far have not spotted.
* But also consider whether the code is 'clean', well-structured and easy to read/maintain.
* Part of this is that:

  * There should be are no compiler (or doxygen) warnings coming from any modified classes
  * The code conforms to our :ref:`coding standards <MantidStandards>`.

* Unit tests (or system tests if more appropriate) should be checked that they:

  * Exist and give adequate coverage (see :ref:`unit testing practices <UnitTestGoodPractice>`).
  * If the ticket is fixing a bug there should be a test that makes sure we don't have to fix the same bug again!
  * Do not load real data (data loading algorithms get a free pass on this one).
  * Leave the system in the same state that they found it (i.e. clean up).
  * Have a performance test, if appropriate.

* Check that any user documentation is adequate and that there are release notes.  In the case of new algorithms, there should be an accompanying ``*.rst`` file that has been added, containing an explanation of what exactly the algorithm does along with Python usage examples.

Functional Testing
==================

The first thing to note is that this should **not** just be a quick check of whatever the ticket says it does. Testing should be as much, if not more, about making sure the code *does not do what it's not supposed to do* as that it *does do what it's supposed to*.

* All of the builds pass
* The developer should have included instructions in the ticket of how to test things work.
* But, as noted above, don’t just do that – also try to break it: click random buttons on GUIs, give unexpected/invalid inputs, etc.
* Note down what you did in the ticket, and the platform you did it on.

If all the requirements have been met and documented approve the PR using `GitHub's review mechanism <https://help.github.com/articles/about-pull-request-reviews/>`_.

Gatekeeper
==========

The ``@mantidproject/gatekeepers`` group is who is meant to merge pull requests into ``main``. This is done by social contract. A gatekeeper can ``merge`` code if:

* Green tick on the last build indicating all automated testing has succeeded
* Adequate tests, both success and failure cases have been performed
* There is comment on the code being reviewed

See the :ref:`guidance for gatekeepers <Gatekeeping>` for more information.
