===========================
The Automated Build Process
===========================

.. contents:: Contents
   :local:

Summary
^^^^^^^

If your changes break the ``main`` builds in any way, on any platform,
then it is your responsibility to fix the error immediately!

The Details
^^^^^^^^^^^

You should follow the :ref:`GitWorkflow`. When you open a
pull request (or commit to an already open pull request) the automated
build process will start. There will be a different build for each
platform/job. A status will appear for each job in the pull request.

The status for each build will be either pending, success or failed.

.. image:: images/BuildStatuses.png

To see the details of a particular build in Jenkins, click on Details
next to the status. To restart a build, if it failed with a spurious
error not related to your code changes, then you can restart that
particular build by selecting Rebuild in Jenkins. Then press "Rebuild"
on the next screen while not changing any of the parameters. If
you don't have permission to restart builds in Jenkins, you will have
to ask someone who does.

.. image:: images/RestartBuild.png

Other Notes
^^^^^^^^^^^

The build will fail if it cannot be cleanly merged with main.

Leeroy will check every 10 minutes for any missed builds, should the
GitHub hooks fail to activate or the build server was down when the
pull request was opened.

The pull request builder we are using is called `Leeroy
<https://github.com/mantidproject/leeroy>`_.

You can find a list of all the pull request Jenkins jobs at `here
<http://builds.mantidproject.org/view/Pull%20Requests/>`_.

Nightly Pipelines
^^^^^^^^^^^^^^^^^

The nightly pipelines are Jenkins jobs responsible for building Mantid packages
and deploying them to the Mantid conda channel. They build both nightly and release versions.
You can find them on `this page <https://builds.mantidproject.org/view/Nightly%20Pipelines/>`_.

The `build_packages_from_branch <https://builds.mantidproject.org/view/Nightly%20Pipelines/>`_
is a Jenkins job based on these nightly jobs that allows you to manually build packages
from an upstream branch in the Mantid repository. For the full guide on how to use the
job, please follow `this link <https://developer.mantidproject.org/Packaging.html#build-packages-from-branch-using-jenkins>`_.
