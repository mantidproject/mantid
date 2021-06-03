..  _05_logging:

=======
Logging
=======

In Mantid, logging can be used to issue messages that appear in a variety of
places depending on the running environment. For example, in MantidWorkbench
the Messages Box is used to display log messages to users and everything
is also logged to a file.

The level of detail is controlled by right-clicking on the Messages Box in
MantidWorkbench and selecting "log level". Alternatively it can be controlled
by the **logging.loggers.root.level** key in the *Mantid.user.properties*
file. There are several levels of logging available, ordered in terms of
increasing priority:

* Debug
* Information
* Notice
* Warning
* Error

Any log message that is sent at a priority level above or equal to the value
set in the properties file will appear in the appropriate location, i.e.
Messages Box, log file.

Within a Python algorithm the log can be accessed using ``self.log()``. To
post a message, simply choose a priority level and call a function that has
that name with the message as an argument, i.e.

.. code-block:: python

    def PyExec(self):
        param = self.getProperty("Inputvalue")

        self.log().information("The cube of the input value " +
                               str(param) + " is " + str(i*i*i))

Avoid excessive logging as it is an expensive operation, i.e. try to avoid
posting a log message at every iteration through a loop as it can slow the
algorithm down dramatically. Instead build the message up as the loop is
traversed and call the logging function once, e.g.

.. code-block:: python

    def PyExec(self):
        limit = self.getProperty('LoopLimit')
        sum = 0
        msg = ""
        for i in range(1, limit+1):
            sum += i*i
            msg += str(i*i) + ' '

        # This will be much more efficient,
        # especially if LoopLimit is large
        self.log().information(msg)