.. _CondaPackageManager:

=====================
Conda Package Manager
=====================

.. contents::
   :local:

Mantid uses `Conda <https://docs.conda.io/en/latest/>`_ as its package management system. This document gives a
developer overview on how we use the Conda package manager, including tips on how to debug dependency issues, and
our policy towards using pip packages (it is strongly discouraged).

Useful Conda Guides
-------------------

Tips for finding a broken dependency
------------------------------------

Fixing a dependency issue
-------------------------

After identifying the Conda dependency and version which is causing the unwanted behaviour, there are several
options we can take to fix the issue. The following options are in order of preference:

1. Raise an issue in the dependencies feedstock repository with a minimum reproducible example. If appropriate,
   request that they mark the package version as "Broken". See `Removing broken packages <https://conda-forge.org/docs/maintainer/updating_pkgs.html#maint-fix-broken-packages>`_ to understand this procedure.

2. If we need a fix urgently, you can consider pinning the package in question. This is not an ideal solution,
   and so you should also open an issue to un-pin the package in future. When pinning a package, consider
   using the not-equals-to operator ``!=x.y.z`` because this allows the package to upgrade automatically when
   a new version arrives (which is hopefully a working version).

Pip package policy
------------------

We have a strict policy with regards to using PyPi packages within Mantid. This policy can be summarised as
follows:

.. code-block:: none

  We strongly encourage PyPi dependencies be built into Conda packages and uploaded to conda-forge. PyPi packages
  will not be automatically installed into our Mantid Conda environments, and should instead be installed by
  users of the software, if required.

We do not want to include pip packages as dependencies in our Conda recipes because there is no guarantee of
compatibility between the two package managers. In the past, attempting to resolve compatibile package versions
when two package managers are involved have caused broken Mantid installations. Furthermore, the original
motivation for moving towards Conda was so that we had a unified package manager rather than using several
different systems or mechanisms. Including pip packages in our dependencies would be a backwards step.

The other solution we considered was installing our pip dependencies downstream within our DAaaS workspace
configuration repository. We decided against this because it feels like bad practise to have a formalised
way of installing dependencies of a software in a way which is completely detached. The prevailing message is
this: please only use Conda packages.
