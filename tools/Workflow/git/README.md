A small set of git extension commands to support the Mantid [workflow](http://www.mantidproject.org/Git_Workflow).

Installation
------------

* Mac/Linux: Run 'sudo make install'. The files will be placed in /usr/local/bin
* Windows: Open this directory in explorer, right-click on install_git_macros.bat and click 'Run as administrator'

Syntax
------

These extensions add the following commands to 'git':
* new - Start a new ticket
* checkbuild - Checks the current branch on build servers (merges to develop)
* finish - Publishes branch ready for testing (and also checks the it has been merged to develop first)
* test - Pass/fail a given ticket number

