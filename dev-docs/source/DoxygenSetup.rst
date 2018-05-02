Doxygen Setup
=============

Unix Console Doxygen Setup
--------------------------

+-----------------+-----------------------------------------------------------------------------------------+
| | Check for     | You may well already have doxygen installed but is it most likely                       |
| | doxygen       | in your systems repositories.                                                           |
|                 | If not, build from source                                                               |
|                 | `here <http://www.stack.nl/~dimitri/doxygen/download.html#srcbin>`__.                   |
+-----------------+-----------------------------------------------------------------------------------------+
| Run cmake       | CMake will genereate the doyxgen config file                                            |
|                 | in ${CMAKE_DIR}/Framework/Doxygen/Mantid.doxyfile                                       |
+-----------------+-----------------------------------------------------------------------------------------+
| | You're done!  |  -  Type 'make doxygen'                                                                 |
| | Try!          |  -  This will run doxygen, showing the output in the console. You may                   |
|                 |     want to pipe warnings to a file to make them easy to read later:                    |
|                 |     'make  doxygen 2> doxygen_errors.log'                                               |
|                 |  -  The documentation will go into a subdir doxygen/html of the                         |
|                 |     directory where cmake was run from.                                                 |
+-----------------+-----------------------------------------------------------------------------------------+

Visual Studio Doxygen Setup
---------------------------

+-----------------+-----------------------------------------------------------------------------------------+
| Install doxygen | Download the                                                                            |
| binaries        | `Windows binaries <http://www.stack.nl/~dimitri/doxygen/download.html#latestsrc>`__     |
|                 | and install them. I'll assume in the following you installed doxygen in                 |
|                 | c:\program files\doxygen                                                                |
+-----------------+-----------------------------------------------------------------------------------------+
| Rerun CMake     | Run cmake for the build to ensure that the Mantid.doxyfile is created                   |
+-----------------+-----------------------------------------------------------------------------------------+
| Add VC++ Tool:  | - Tools\External Tool then click Add                                                    |
| "DoxyGen"       | - Title: &DoxyGen                                                                       | 
|                 | - Command: C:\Program Files\Doxygen\bin\doxygen.exe                                     |
|                 | - Arguments: "$(SolutionDir)\Framework\Doxygen\Mantid.doxyfile" (include the quotes!)   |
|                 | - Initial Directory: $(SolutionDir)\Build                                               |
|                 | - Check the "Use output window" box                                                     |
+-----------------+-----------------------------------------------------------------------------------------+
| Add VC++ Tool:  | - Tools\External Tool then click Add                                                    |
| "view DoxyGen"  | - Title: &View DoxyGen                                                                  |
|                 | - Command your favorite browser, e.g. C:\program Files\internet Explorer\iexplore.exe   |
|                 |   or C:\Program Files (x86)\Google\Chrome\Application\chrome.exe                        |
|                 | - Arguments: "$(SolutionDir)doxygen\html\index.html"                                    |
|                 | - Initial Directory: leave empty                                                        |
+-----------------+-----------------------------------------------------------------------------------------+
| You're done!    | - Choose Tools/DoxyGen from the menu, and watch the magic happen (DoxyGen will log      |
| Try! "DoxyGen"  |   it's progress and complaints to the output window). Clicking on a warning message     |
|                 |   will take you to the location in the code of the warning.                             |
|                 | - Choose Tools/View DoxyGen to explore the documentation.                               |
|                 | - The "Main Page" is probably rather boring. Click on "Namespaces" in the menu line to  |
|                 |   browse your classes etc.                                                              |
+-----------------+-----------------------------------------------------------------------------------------+

Eclipse Doxygen Setup
---------------------

+-----------------+-----------------------------------------------------------------------------------------+
| Check for       | You may well already have doxygen installed, but if not you can install it at the same  |
| doxygen         | time as the plugin below via the update site                                            |
+-----------------+-----------------------------------------------------------------------------------------+
| Run cmake       | This will generate the doxygen config file in                                           |
|                 | ${CMake_DIR}/Framework/Doxygen/Mantid.doxygen                                           |
+-----------------+-----------------------------------------------------------------------------------------+
| Install Eclipse | - `Eclox <http://eclox.eu/>`_ is a frontend plugin for Eclipse.                         |
| plugin: "Eclox" | - Install it using the Eclipse Update Manager                                           |
|                 | - To do this go to Help -> Software Updates...                                          |
|                 | - Select the 'Available Software' tab then the 'Add Site...' button                     |
|                 | - Enter `http://download.gna.org/eclox/update` as the location                          |
|                 | - Eclipse will add the site to the list and you can open the tree to select and install |
|                 |   Eclox                                                                                 |
+-----------------+-----------------------------------------------------------------------------------------+
| You're done!    | - You'll now have a 'build doxygen' button in your toolbar (a blue '@')                 |
| Try!            | - The first time you click it you'll be prompted for the configuration file. Point it   |
|                 |   at ${CMake_DIR}/Framework/Doxygen/Mantid.doxygen                                      |
|                 | - This will run doxygen, showing the output in the console and adding warnings symbols  |
|                 |   on the source files (as for compilation warnings). Hovering over these will show the  |
|                 |   warning.                                                                              |
|                 | - The documentation will go into a subdir doxygen/html of the directory where cmake was |
|                 |   run from.                                                                             |
+-----------------+-----------------------------------------------------------------------------------------+
