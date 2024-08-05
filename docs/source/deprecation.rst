.. _deprecation_policy:

=========================
Mantid Deprecation Policy
=========================

.. contents::


Introduction
------------

This policy lays out how we deprecate algorithms, interfaces and other parts of Mantid. It is intended as a reference
for both users and developers.


What will we deprecate?
-----------------------

- Algorithms that have not been used for 3 years or more. We will use Usage Data to determine this. For more information
  about how we record Usage data please see our `Privacy Policy: Usage Data Recorded in Mantid
  <https://www.mantidproject.org/privacy.html#usage-data-recorded-in-mantid>`_
- Other Features covered by Usage Data that have not been used for 3 or more years
- Soft deprecation of Algorithms superseded by newer versions of an algorithm e.g. v1 deprecated when v2 has been created.
  See below for more about soft deprecation.

Apart from soft deprecation, the decision to mark an algorithm or feature as deprecated must be agreed with the `Technical
Working Group <https://github.com/mantidproject/governance/tree/main/technical-working-group>`_.

Timescales
----------

When an algorithm or feature has been identified for deprecation it will be marked as deprecated for a minimum of 2
releases before finally being removed.

Communication
-------------

To ensure users are aware of deprecated algorithms and other features
- Mark as deprecated in code base with suitable warning message for users in the log
- Mark as deprecated in supporting documentation
- Add deprecation to release notes

When the code is being removed from the release that is being developed
- Post to the `Mantid Forum <https://forum.mantidproject.org/>`_
- E-mail the Mantid Announcements e-mail list (to join this list see our `Contact Us <https://www.mantidproject.org/contact>`_ page.
- Technical Working Group members will communicate the removal to the facilities they represent
- A list of removed algorithms/features will be listed in the :ref:`release_notes` for that release

Process of deprecating an algorithm
-----------------------------------

- Check usage reporting as to whether algorithm/feature has been used within last 3 years.
- If it has not be used within last three years make a request to the Technical Working Group (TWG) to have it deprecated
- If approved by the TWG follow the guidance under the Communication section above.
- If removing more than one algorithm/feature from the code base do each one in a separate pull request. This is to allow
  for deprecations to be reversed if necessary.

Soft Deprecation
----------------

When algorithms are replaced with newer versions, the previous version(s) will be subject to a soft deprecation. We will
make it clear that we are no longer supporting (i.e. no longer developing) the previous version(s). Although previous
versions will still be available and safe to use, users will be encouraged to use more recent versions.

Once older versions have gone out of use for at least 3 years they will then be subject to the deprecation process as
outlined above.
