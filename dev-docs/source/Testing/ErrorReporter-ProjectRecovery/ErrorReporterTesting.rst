.. _error_reporter_testing:

Error Reporter Testing
======================

.. contents::
  :local:

Error Reporter test
-------------------

*Preparation*

- Ensure that `Report usage data` is checked in the First Time Setup screen
- Below you will be asked to produce a hard crash (by running the ``Segfault``
  Algorithm) or a soft crash (bring up an error report pop-up without also
  closing Mantid, by Loading ``Training_Exercise3a_SNS.nxs`` available from
  the `Training Course Data <https://sourceforge.net/projects/mantid/files/Sample%20Data/TrainingCourseData.zip/download>`__
  and running the ``NormaliseToMonitor`` Algorithm. If one or both of these
  methods to crash Mantid no longer works, then ask other developers for a
  suitable method and update these instructions.
- Make sure the location of this file is included in your search directories. Full instructions :ref:`are available online <mantid:getting started>`.
- These tests should be run with someone who has access to the error reports database.
  Please contact the developer managing support before you run these tests.

**Time required 10 - 20  minutes**

--------------

1. Open MantidWorkbench

- Cause a crash by running the ``Segfault`` algorithm, Workbench should close
- This should cause an error reporter dialog saying Mantid has thrown an unexpected exception
- Make sure the hyperlink to the privacy policy works
- Click the ``Show More Details`` button to open the Show More Details dialog. This should show user details such as OS.
  The python stacktrace should be empty as the Workbench unhandled exception occurred in C++. Close this dialog.
- Test that the ``Yes, share information`` is disabled and the ``Email`` box is outlined in red until a valid email
  (``something@something.somthing``) is entered in the ``Email`` box.
- In the ``Name`` box enter ``Private`` and in the ``Email`` box enter ``private@private.com``
- Input some additional information into the main textbox. Check that the character count label updates as you type, copy paste etc.
- Check that the character count label changes to red and the button to send the report are disabled if you exceed the character limit.
- Check that these changes revert when brining the number of characters back within the limit.
- Input some additional information into the main textbox. Try to include characters that need to be escaped such as ``"``
- Tick the ``Remember Me`` checkbox
- Click the ``Don't share any information`` button
- Check with the database admin that an error report was **not** sent.

---------------

2. Open MantidWorkbench

- Cause a crash by running the ``Segfault`` algorithm, Workbench should close
- This should cause an error reporter dialog saying Mantid has thrown an unexpected exception
- The private contact details from Test 1 should **NOT** be displayed and the ``Remember Me`` checkbox should **NOT** be ticked
- In the ``Name`` box enter ``Public`` and in the ``Email`` box enter ``public@public.com``
- Input some additional information into the main textbox. Try to include characters that need to be escaped such as ``"``
- Tick the ``Remember Me`` checkbox
- Click the ``Yes, share information`` button
- Check with the database admin that an error report was sent **WITH** the correct name, email and textbox.

---------------

3. Open MantidWorkbench

- Load the file ``Training_Exercise3a_SNS.nxs``
- Run the ``NormaliseToMonitor`` algorithm
- This should cause an error reporter dialog saying Mantid has thrown an unexpected exception
- The shared contact details from test 2 should be displayed and the ``Remember Me`` checkbox ticked
- Make sure the hyperlink to the privacy policy works
- In the ``Name`` box enter ``Private2`` and in the ``Email`` box enter ``private2@private.com``
- Input some additional information into the main textbox. Try to include characters that need to be escaped such as ``"``
- Make sure the ``Continue`` radio button is checked
- Click the ``Don't share any information`` button
- You should be returned to the main Mantid window
- Check with the database admin that an error report was **not** sent.

---------------

4. MantidWorkbench is still open

- Load the file ``Training_Exercise3a_SNS.nxs``
- Run the ``NormaliseToMonitor`` algorithm
- This should cause an error reporter dialog saying Mantid has thrown an unexpected exception
- The shared contact details from test 2 should be displayed and the ``Remember Me`` checkbox ticked
- Click the ``Show More Details`` button to open the Show More Details dialog. This should show user details such as OS.
- Input some additional information into the main textbox. Try to include characters that need to be escaped such as ``"``
- Leave the ``Name`` box EMPTY and in the ``Email`` box enter ``public2@public.com``
- Tick the ``Remember Me`` checkbox
- Make sure the ``Continue`` radio button is checked
- Click the ``Yes, share information`` button
- You should be returned to the main Mantid window
- Check with the database admin that an error report was sent **WITH** a name, email, stacktrace and a textbox.

---------------

5. MantidWorkbench is still open

- Load the file ``Training_Exercise3a_SNS.nxs``
- Run the ``NormaliseToMonitor`` algorithm
- This should cause an error reporter dialog saying Mantid has thrown an unexpected exception
- Only the ``public2@public.com`` email from test 4 should be displayed in the email box and the ``Remember Me`` checkbox ticked.
- In the `Name` box enter ``Public3`` and in the ``Email`` box enter ``public3@public.com``
- Input some additional information into the main textbox. Try to include characters that need to be escaped such as ``"``
- Tick the ``Remember Me`` checkbox
- Make sure the ``Terminate`` radio button is checked
- Click the ``Yes, share information`` button
- Mantid should shut down
- Check with the database admin that an error report was sent **WITH** a name, email, stacktrace and a textbox.

--------------

6. Open MantidWorkbench

- Load the file ``Training_Exercise3a_SNS.nxs``
- Run the ``NormaliseToMonitor`` algorithm
- This should cause an error reporter dialog saying Mantid has thrown an unexpected exception
- The shared contact details from test 5 should be displayed and the ``Remember Me`` checkbox ticked
- Close the error reporter and MantidWorkbench

--------------

7. Open your ``Mantid.user.properties`` file

- Add the incorrect rooturl ``errorreports.rooturl = https://fake.mantidproject.org`` anywhere in the file (correct url is ``https://errorreports.mantidproject.org``)
- This will cause the error reporter to fail to send the report
- Open MantidWorkbench
- Cause a crash using either of the previous methods
- Enter a fake email address into the ``Email`` box
- Click the ``Yes, share information`` button to send the report
- A message box should appear informing you that the error report has failed to send
- Close the message box and the error reporter should stay open
- Check you can still access the additional information
- Click the ``Don't share any information`` button and close MantidWorkbench
- Remove the line added to the ``Mantid.user.properties`` file

Test the error reporter with any weird and wonderful ideas.
Note any problems with Workbench or these testing instructions.

**Thanks for testing!!!**
