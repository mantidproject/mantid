.. _error_reporter_testing:

Error Reporter Testing
======================

.. contents::
  :local:

Error Reporter test
-------------------

*Preparation*

- Ensure that `Report usage data` is checked in the First Time Setup screen
- Below you will be asked to produce a hard crash (by running the `Segfault`
  Algorithm) or a soft crash (bring up an error report pop-up without also
  closing Mantid, by Loading ``Training_Exercise3a_SNS.nxs`` available from
  the `Training Course Data <https://sourceforge.net/projects/mantid/files/Sample%20Data/TrainingCourseData.zip/download>`__
  and running the `NormaliseToMonitor` Algorithm. If one or both of these
  methods to crash Mantid no longer works, then ask other developers for a
  suitable method and update these instructions.
- Make sure the location of this file is included in your search directories. Full instructions :ref:`are available online <mantid:getting started>`.
- These tests should be run with someone who has access to the error reports database. Please contact Daniel Murphy when you are going to run these tests.

**Time required 10 - 20  minutes**

--------------

1. Open MantidWorkbench

- Cause a crash by running the `Segfault` algorithm, Workbench should close
- This should cause an error reporter dialog saying Mantid has thrown an unexpected exception
- Make sure the hyperlink to the privacy policy works
- Click the `Show More Details` button to open the Show More Details dialog. This should show user details such as OS.
  The python stacktrace should be empty as the Workbench unhandled exception occurred in C++. Close this dialog.
- Enter some text in the `Name` box; make sure the `Share non-identifiable information` button gets greyed out
- In the `Name` box enter `Private` and in the `Email` box enter `private`
- Input some additional information into the main textbox. Try to include characters that need to be escaped such as ``"``
- Tick the `Remember Me` checkbox
- Click the `Don't share any information` button
- Check with the database admin that an error report was sent **without** a name, email or stacktrace.

---------------

2. Open MantidWorkbench

- Cause a crash by running the `Segfault` algorithm, Workbench should close
- This should cause an error reporter dialog saying Mantid has thrown an unexpected exception
- The `Private` contact details from Test 1 should **NOT** be displayed and the `Remember Me` checkbox should **NOT** be ticked
- In the `Name` box enter `Public` and in the `Email` box enter `public`
- Input some additional information into the main textbox. Try to include characters that need to be escaped such as ``"``
- Tick the `Remember Me` checkbox
- Click the `Yes, share information` button
- Check with the database admin that an error report was sent **WITH** the correct name, email and textbox.

---------------

3. Open MantidWorkbench

- Load the file `Training_Exercise3a_SNS.nxs`
- Run the `NormaliseToMonitor` algorithm
- This should cause an error reporter dialog saying Mantid has thrown an unexpected exception
- The `Public` contact details from Test 2 should be displayed and the `Remember Me` checkbox ticked
- Make sure the hyperlink to the privacy policy works
- Enter some text in the `Name` box; make sure the `Share non-identifiable information` button gets greyed out
- In the `Name` box enter `Private2` and in the `Email` box enter `private2`
- Input some additional information into the main textbox. Try to include characters that need to be escaped such as ``"``
- Make sure the `Continue` radio button is checked
- Click the `Don't share any information` button
- You should be returned to the main Mantid window
- Check with the database admin that an error report was sent **without** a name, email, stacktrace or textbox.

---------------

4. MantidWorkbench is still open

- Load the file `Training_Exercise3a_SNS.nxs`
- Run the `NormaliseToMonitor` algorithm
- This should cause an error reporter dialog saying Mantid has thrown an unexpected exception
- The `Public` contact details from Test 2 should be displayed and the `Remember Me` checkbox ticked
- Click the `Show More Details` button to open the Show More Details dialog. This should show user details such as OS.
  There should be a python stacktrace as this unhandled exception occurred in Python. Close this dialog.
- Input some additional information into the main textbox. Try to include characters that need to be escaped such as ``"``
- This time, UNTick the `Remember Me` checkbox
- Make sure the `Continue` radio button is checked
- Click the `Share non-identifiable information` button
- You should be returned to the main Mantid window
- Check with the database admin that an error report was sent **without** a name, email or stacktrace, BUT **WITH** a textbox.

---------------

5. MantidWorkbench is still open

- Load the file `Training_Exercise3a_SNS.nxs`
- Run the `NormaliseToMonitor` algorithm
- This should cause an error reporter dialog saying Mantid has thrown an unexpected exception
- Contact details from before should **NOT** be displayed and the `Remember Me` checkbox should **NOT** be ticked
- Enter some text in the `Name` box; make sure the `Share non-identifiable information` button gets greyed out
- Input some additional information into the main textbox. Try to include characters that need to be escaped such as ``"``
- Leave the `Name` box EMPTY and in the `Email` box enter `public2`
- Tick the `Remember Me` checkbox
- Make sure the `Continue` radio button is checked
- Click the `Yes, share information` button
- You should be returned to the main Mantid window
- Check with the database admin that an error report was sent **WITH** a name, email, stacktrace and a textbox.

---------------

6. MantidWorkbench is still open

- Load the file `Training_Exercise3a_SNS.nxs`
- Run the `NormaliseToMonitor` algorithm
- This should cause an error reporter dialog saying Mantid has thrown an unexpected exception
- Only the `public2` email from Test 5 should be displayed in the email box and the `Remember Me` checkbox ticked.
  Make sure the `Share non-identifiable information` button is currently enabled (not greyed out)
- Enter some text in the `Name` box; make sure the `Share non-identifiable information` button gets greyed out
- In the `Name` box enter `Public3` and in the `Email` box enter `public3`
- Input some additional information into the main textbox. Try to include characters that need to be escaped such as ``"``
- Tick the `Remember Me` checkbox
- Make sure the `Terminate` radio button is checked
- Click the `Yes, share information` button
- Mantid should shut down
- Check with the database admin that an error report was sent **WITH** a name, email, stacktrace and a textbox.

--------------

7. Open MantidWorkbench

- Load the file `Training_Exercise3a_SNS.nxs`
- Run the `NormaliseToMonitor` algorithm
- This should cause an error reporter dialog saying Mantid has thrown an unexpected exception
- The `Public3` contact details from Test 6 should be displayed and the `Remember Me` checkbox ticked

Test the error reporter with any weird and wonderful ideas.
Note any problems with Workbench or these testing instructions.

**Thanks for testing!!!**