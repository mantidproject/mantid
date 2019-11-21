.. _error_reporter_testing:

Error Reporter Testing
======================

.. contents::
  :local:

Error Reporter test
-------------------

*Preparation*

- Ensure that `Report usage data` is checked in the First Time Setup screen
- Below you will be asked to produce a hard crash (by running the `Segfault` Algorithm) or a soft crash (bring up an error report pop-up without also closing Mantid, by Loading ``Training_Exercise3a_SNS.nxs`` available from the `Training Course Data <https://sourceforge.net/projects/mantid/files/Sample%20Data/TrainingCourseData.zip/download>`__ and running the `NormaliseToMonitor` Algorithm. `If one or both of these methods to crash Mantid no longer works, then ask other developers for a suitable method and update these instructions.
- Make sure the location of this file is included in your search directories. Full instructions :ref:`are available online <mantid:getting started>`.
- These tests should be run with someone who has access to the error reports database. Please contact Daniel Murphy when you are going to run these tests.

**Time required 10 - 20  minutes**

--------------

1. Open MantidPlot

- Cause a crash by running the `Segfault` algorithm
- The error report dialog should pop up
- Make sure the hyperlink to the privacy policy works
- Enter some text in the `Name` box; make sure the `Share non-identifiable information` button gets greyed out
- In the `Name` box enter `Private` and in the `Email` box enter `private`
- Click the `Don't share any information` box
- Check with the database admin

---------------

2. Open MantidPlot

- Cause a crash by running the `Segfault` algorithm
- The error report dialog should pop up
- In the `Name` box enter `Public` and in the `Email` box enter `public`
- Click the `Yes, share information` box
- Check with the database admin

---------------

3. Open MantidPlot

- Load the file `Training_Exercise3a_SNS.nxs`
- Run the `NormaliseToMonitor` algorithm
- This should cause an error box saying Mantid has thrown an unexpected exception
- Make sure the hyperlink to the privacy policy works
- Enter some text in the `Name` box; make sure the `Share non-identifiable information` button gets greyed out
- In the `Name` box enter `Private2` and in the `Email` box enter `private2`
- Make sure the `Continue` radio button is checked
- Click the `Don't share any information` box
- You should be returned to the main Mantid window
- Check with the database admin

---------------

4. Open MantidPlot

- Load the file `Training_Exercise3a_SNS.nxs`
- Run the `NormaliseToMonitor` algorithm
- This should cause an error box saying Mantid has thrown an unexpected exception
- Make sure the `Continue` radio button is checked
- Click the `Share non-identifiable information` box
- You should be returned to the main Mantid window
- Check with the database admin

---------------

5. Open MantidPlot

- Load the file `Training_Exercise3a_SNS.nxs`
- Run the `NormaliseToMonitor` algorithm
- This should cause an error box saying Mantid has thrown an unexpected exception
- Make sure the hyperlink to the privacy policy works
- Enter some text in the `Name` box; make sure the `Share non-identifiable information` button gets greyed out
- In the `Name` box enter `Public2` and in the `Email` box enter `public2`
- Make sure the `Continue` radio button is checked
- Click the `Yes, share information` box
- You should be returned to the main Mantid window
- Check with the database admin

---------------

6. Open MantidPlot

- Load the file `Training_Exercise3a_SNS.nxs`
- Run the `NormaliseToMonitor` algorithm
- This should cause an error box saying Mantid has thrown an unexpected exception
- Make sure the hyperlink to the privacy policy works
- Enter some text in the `Name` box; make sure the `Share non-identifiable information` button gets greyed out
- In the `Name` box enter `Public3` and in the `Email` box enter `public3`
- Make sure the `Terminate` radio button is checked
- Click the `Yes, share information` box
- Mantid should shut down
- Check with the database admin
