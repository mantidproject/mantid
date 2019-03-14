.. _WorkbenchMessagesWindow:

===============
Messages Window
===============

All of the output from your scripts and algorithms run from interfaces or
other places will be output here. This is alongside the ability change the
logging level. The logging level has 5 different options to display the right
level of detail to choose the correct amount of detail for your circumstances.

.. image:: ../images/Workbench/MessageWindow/MessagesWidget.png
    :align: right
    :scale: 70%

All output from running your scripts or Mantid algorithms is logged in
this window. To display only the information most relevant to you, you are
provided with 5 logging levels (in ascending order of priority):

    - Debug:
          Anything that may be useful to understand what the code has been
          doing for debugging purposes. Mostly used by developer or users
          reporting problems.
    - Information:
          Useful information. Used by users who want more detail regarding
          Mantid's status.
    - Notice:
          Important updates about Mantid's status. Algorithms log at this level
          when starting/finishing. This is the default logging level.
    - Warning:
          Something was wrong but Mantid was able to continue despite the
          problem.
    - Error:
          An error has occurred but Mantid is able to handle it and continue.

To select your logging level right-click inside the message window.

.. image:: ../images/Workbench/MessageWindow/MessagesWidgetContextMenu.png

Any log message with a priority higher or equal to the selected level will be
displayed. To distinguish between priority levels each one has an associated
colour, as shown in the figure on the right.



