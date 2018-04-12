.. _DeveloperAccounts:

==================
Developer Accounts
==================

.. contents::
  :local:

User Names
----------

Simple, easy to recognise user names are preferred. For example "Nick Draper", or if spaces are not allowed "NickDraper".

Account Creation
----------------

- Create a **Mantid Wiki** account; follow `this link <https://www.mantidproject.org/Special:RequestAccount>`__.
- Sign up to the **Mantid Developers** email list; follow `this link <http://lists.mantidproject.org/mailman/listinfo/mantid-developers>`__.
- Sign up for the **Mantid Slack** channel; follow `this link <https://mantid.slack.com/>`__.
- If you don't already have one, sign up for a **Github** account; follow `this link <https://github.com/>`__.
	+ Remember that your username should be easily identifiable.
	+ Contact one of the "Owners" on `this list <https://github.com/orgs/mantidproject/people?query=role%3Aowner>`__ to add you to the developer team.
	+ Set up Git on your workstation; see `this guide <https://help.github.com/articles/set-up-git/>`__.
	+ The Git workflow is described on the :ref:`GitWorkflow` page.

- Consider signing up for **Skype**; follow `this link <https://www.skype.com/>`__.
- Sign up for a **Gravatar** account; follow `this link <https://en.gravatar.com/>`__.

SNS Git
-------

If you are based at SNS, in order to be able to ssh out of the lab, you need to do the following:

- Install "Corkscrew" using your package manager.
- Add the following lines to ~/.ssh/config:


.. code:: bash

    ProxyCommand corkscrew snowman.ornl.gov 3128 %h %p
    Host github.com

Introducing Yourself
--------------------

- Post a short introduction of yourself to the rest of the team in the General chatroom on Mantid Slack.
- Add yourself together with your contact details and a photo (selfies are fine) to :ref:`DevelopmentTeam`.

Admin Notes
-----------

These are notes for account admins on how to add new users.

- **Wiki**
    + Go to the `special pages index <https://www.mantidproject.org/Special:SpecialPages>`_.
    + Select Login/Create Account.
    + Select the Create Account link at the top of the box.
    + Username should be first name (space) surname.
    + User will be sent an email to verify.

- **Github**
	- Add the username provided to the mantid-developers team at `https://github.com/organizations/mantidproject/teams <https://github.com/organizations/mantidproject/teams>`_.
