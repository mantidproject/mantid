.. _error_recovery_testing:

Error Reporter and Project Recovery Testing
===========================================

.. contents::
  :local:

Error Reporter test
-------------------

*Preparation*

-  Ensure that `Report usage data` is checked in the First Time Setup screen
-  Files ``SANS2D00005512.nxs``
-  Make sure the location of these files is included in your search
   directories. Full instructions `are available
   online <http://www.mantidproject.org/MBC_Getting_set_up#MantidPlot_First-Time_Setup>`__.
- These tests should be run with someone who has access to the error reports database. Please
contact Keith Butler when you are going to run these tests.

**Time required 10 - 20  minutes**

--------------

1. Open MantidPlot

- Cause a crash by running the `Segfault` algorithm
- The error report dialog should pop up
- Make sure the hyperlink to the privacy policy works
- Enter some text in the `Name` box; make sure the `Share non-identifiable information` button gets greyed out
- In the `Name` box enter `Private` and in the `Email` box enter `private`
- Click the `Don't share any information` box

---------------

2. Open MantidPlot

- Cause a crash by running the `Segfault` algorithm
- The error report dialog should pop up
- In the `Name` box enter `Public` and in the `Email` box enter `public`
- Click the `Yes, share information` box

---------------

3. Open MantidPlot

- Load the file `SANS2D00005512.nxs`
- Click on the new workspace group, this should give a list of workspaces
- Right-click one of the workspaces and select `Sample Logs` 
- This should cause an error box saying Mantid has thrown an unexpected exception
- Make sure the hyperlink to the privacy policy works
- Enter some text in the `Name` box; make sure the `Share non-identifiable information` button gets greyed out
- In the `Name` box enter `Private2` and in the `Email` box enter `private2`
- Make sure the `Continue` radio button is checked
- Click the `Don't share any information` box
- You should be returned to the main Mantid window

---------------

4. Open MantidPlot

- Load the file `SANS2D00005512.nxs`
- Click on the new workspace group, this should give a list of workspaces
- Right-click one of the workspaces and select `Sample Logs` 
- This should cause an error box saying Mantid has thrown an unexpected exception
- Make sure the `Continue` radio button is checked
- Click the `Share non-identifiable information` box
- You should be returned to the main Mantid window

---------------

5. Open MantidPlot

- Load the file `SANS2D00005512.nxs`
- Click on the new workspace group, this should give a list of workspaces
- Right-click one of the workspaces and select `Sample Logs` 
- This should cause an error box saying Mantid has thrown an unexpected exception
- Make sure the hyperlink to the privacy policy works
- Enter some text in the `Name` box; make sure the `Share non-identifiable information` button gets greyed out
- In the `Name` box enter `Public2` and in the `Email` box enter `public2`
- Make sure the `Continue` radio button is checked
- Click the `Yes, share information` box
- You should be returned to the main Mantid window

---------------

6. Open MantidPlot

- Load the file `SANS2D00005512.nxs`
- Click on the new workspace group, this should give a list of workspaces
- Right-click one of the workspaces and select `Sample Logs` 
- This should cause an error box saying Mantid has thrown an unexpected exception
- Make sure the hyperlink to the privacy policy works
- Enter some text in the `Name` box; make sure the `Share non-identifiable information` button gets greyed out
- In the `Name` box enter `Public3` and in the `Email` box enter `public3`
- Make sure the `Terminate` radio button is checked
- Click the `Yes, share information` box
- Mantid should shut down

---------------

Project Recovery test
---------------------

*Preparation*

- Before running these tests, set project recovery to run every 2 seconds. The instructions for this
  are on the `Project Recovery concepts page <http://docs.mantidproject.org/nightly/concepts/ProjectRecovery.html>`_.


**Time required XX - YY  minutes**

--------------

1. Open MantidPlot
- Right-click in the Results Log and set `Log level` to `Debug`
- 
