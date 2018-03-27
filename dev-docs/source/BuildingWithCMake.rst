.. _BuildingWithCMake:

===================
Building with CMake
===================

.. contents::
  :local:

CMake is the build system for the entirety of Mantid (Framework, MantidQt & MantidPlot). It is used to generate native build files for your platform, which can be Makefiles (for use with make, nmake or jom) for command line builds or project/solution files for an IDE (e.g. Visual Studio, Eclipse, Qt Creator, XCode).

Installing CMake
################

The first thing to do is get hold of CMake. You can get it from `here <http://www.cmake.org/cmake/resources/software.html>`_ or via your package manager (e.g. yum). It is advised to download a stable release and **not** a release candidate. For Mac/Windows check the option that adds ``cmake`` to the system path for all users

Installing Git
##############

Git is required to fetch the source code. Install with a package manager or from https://git-scm.com/download/win on Windows. Choose the option that adds ``git`` and minimal tools to the path

Installing Git LFS (Windows only)
#################################

The Windows dependencies are managed by `git-lfs <https://git-lfs.github.com/>`_. Download and install this. **At ISIS make sure you use the admin account and install it in Program Files**.

Please ensure that once installed you open a new "Git Bash" shell and type ``git lfs install``.

Configuring your environment
############################

Mantid has [[Mantid_Prerequisites|several prerequisites]] and CMake must be able to find these when the build is being set up. The set up varies depending on the environment you are running:

* Windows: Install `Visual Studio 2015 Community Edition <https://go.microsoft.com/fwlink/?LinkId=532606&clcid=0x409>`_. Make sure you register the copy as by default it is a trial edition with a 30-day license
* Mac OS X: All dependencies are installed via homebrew. Please follow [https://github.com/mantidproject/mantid/wiki/Building-Mantid-on-OS-X-10.10-&-10.11-using-clang-and-Xcode these] instructions to install the required packages.
* Linux: All prerequisites are installed in system paths from repositories so no set up should be necessary.

Ubuntu
------

* To install all pre-requisites for building mantid you will first need access to the Mantid PPA for a newer version of Poco:
<pre>
sudo apt-get install software-properties-common gdebi-core
sudo apt-add-repository ppa:mantid/mantid
sudo apt-get update
</pre>

Now download the latest version of [https://sourceforge.net/projects/mantid/files/developer/ mantid-developer] package and install it (this may take a while)
<pre>
sudo gdebi ~/Downloads/mantid-developer-X.Y.Z_all.deb
</pre>

You are now ready to configure your build with CMake.

Red Hat 7/CentOS 7
------------------

* First we need to set up the the yum repos for the Mantid dependencies. Create a file called <code>/etc/yum.repos.d/isis-rhel.repo</code> with the following content:
<pre>
[isis-rhel]
name=ISIS Software Repository for Redhat Enterprise Linux $releasever - $basearch
baseurl=http://yum.isis.rl.ac.uk/rhel/$releasever/$basearch
failovermethod=priority
enabled=1
gpgcheck=0

[isis-rhel-noarch]
name=ISIS Software Repository for Redhat Enterprise Linux $releasever - noarch
baseurl=http://yum.isis.rl.ac.uk/rhel/$releasever/noarch
failovermethod=priority
enabled=1
gpgcheck=0

[isis-rhel-debuginfo]
name=ISIS Software Repository for Redhat Enterprise Linux $releasever - $basearch - Debug
baseurl=http://yum.isis.rl.ac.uk/rhel/$releasever/$basearch/debug
failovermethod=priority
enabled=1
gpgcheck=0

[isis-rhel-source]
name=ISIS Software Repository for Redhat Enterprise Linux $releasever - $basearch - Source
baseurl=http://yum.isis.rl.ac.uk/rhel/$releasever/SRPMS
failovermethod=priority
enabled=0
gpgcheck=0
</pre>
* Now set up the EPEL repository by running:
<pre>
su -c 'rpm -Uvh https://download.fedoraproject.org/pub/epel/7/x86_64/Packages/e/epel-release-7-11.noarch.rpm'
</pre>

* On RHEL enable the optional repository by running:
<pre>
su -c 'subscription-manager repos --enable rhel-7-server-optional-rpms'
</pre>

* Next install the <code>mantid-developer</code> rpm
<pre>
yum install mantid-developer
</pre>

You should now be set up to clone the code and build with cmake. Please note that the executable is called <code>cmake3</code>.

Also, if you use the Ninja generator then the executable is called <code>ninja-build</code>

CCache
######

Mantid's cmake is configure to use the [https://ccache.samba.org/ ccache] tool if it is available. It is highly recommended that this be used on Linux/macOS systems.

For Linux either run either

* <code>sudo yum install ccache</code> (RedHat) or
* <code>sudo apt-get install ccache</code> (Ubuntu)

For macOS run:

* <code>brew install ccache</code>

After it is installed run <code>ccache --max-size=20G</code> to increase the size of the cache.

If you're build with <code>ccache</code> exhibits warnings that are not usually present then try setting the <code>ccache --set-config=run_second_cpp="true"</code> config option (or set <code>CCACHE_CPP2=yes</code> environment variable on older versions).

Network Drives
--------------

The default location for the cache directory is <code>$HOME/.ccache</code>. If you're home directory is on a network-mounted drive then the location of this cache be moved to provide the best performance. On newer versions of <code>ccache</code> run <code>ccache --set-config=cache_dir=PATH_TO_CACHE</code>. Older versions (<3.2) do not allow this and must fall back to setting the <code>CCACHE_DIR</code> environment variable in your shell profile.

Configuring your build
######################

CMake encourages the use of 'out of source' builds. This means that all generated files are placed in a separate directory structure to the source files. This separation makes a full clean easier (you just delete everything) and means that you can have different types of build (Release, Debug, different compiler versions, ....) in separate places (N.B. For Visual Studio & XCode, you can still select the type of build from within the IDE).

Red Hat Enterprise Linux (RHEL) 6 Special Notes
-----------------------------------------------

As RHEL6 contains versions of various software that are out of date, we need to use newer versions.  In order for these new versions to not interfere with the default versions on the system, we install them using something called [https://access.redhat.com/site/documentation//en-US/Red_Hat_Developer_Toolset/1/html/Software_Collections_Guide/index.html Software Collections].  The basic upshot of using these is that you have to prefix your configure and make commands with <code>scl enable mantidlibs34</code>.  You always need to enclose the command you want to run in single quotes.  <br/>
So some examples of commands we might use are:<br/>
<code>
scl enable mantidlibs34 'cmake-gui'<br/>
scl enable mantidlibs34 'cmake -G"Ninja" /path/to/Mantid' <br/>
scl enable mantidlibs34 'ninja -j10'
</code>

From the command line ...
-------------------------

* If wanting an out of source build, create the directory you want to build in and <code>cd</code> into it.
* On Windows, you may need to be in a Visual Studio Command Prompt.
* Run <code>cmake /path/to/Mantid</code>, or to <code>/path/to/Mantid/Framework</code> if you only want a build of the Framework (typically not recommended, but possible nonetheless). This will generate build files using the default generator for your platform (e.g. Unix Makefiles on Linux).
* If you want to use a specific generator (run <code>cmake --help</code> for a list of available generators for your platform), use the <code>-G</code> option, e.g. <code>cmake -G"NMake Makefiles" /path/to/Mantid</code>.
* If you want to set the build type (e.g. Release, Debug) you can run cmake with the <code>-i</code> option or by passing the argument <code>-DCMAKE_BUILD_TYPE=Debug</code> to cmake. The default is Release.

From the CMake gui ...
----------------------

* The cmake gui is available from, e.g., the Windows Program menu or the command line executable <code>cmake-gui</code>.
* Start it and click the "Browse Source" button to point to <code>/path/to/Mantid</code>, or to <code>/path/to/Mantid/Framework</code> if you only want a build of the Framework (typically not recommended, but possible nonetheless).
* Click "Browse Build" and point to the directory you want to build into - it's recommended that you create a new directory for this (see above), though it can be the same as the source directory.
* Click "Configure" down near the bottom of the window.
* A new window will appear asking which 'Generator' you want to use. Choose one and click OK (N.B. VS2010 = Visual Studio 10, and note that you should append Win64 to this for a 64 bit build).
* Wait a while....
* You will be presented with a list of options in red that can in principle be changed. You probably don't want to change anything, except perhaps checking "MAKE_VATES" if you want to build that.
* Click "Configure" again and wait....
* Finally, click "Generate". This will create the build files, e.g. there will be a Mantid.sln in the directory you selected as your build directory.

Data Files Location
-------------------

Mantid used the CMake ExternalData system for managing testing data. See [[Data Files in Mantid#Developer_Setup]] for further instructions.

With Qt Creator ...
-------------------

[http://qt.nokia.com/products/developer-tools/ Qt Creator] has some really nice features (it's cross-platform, you can directly open Qt Designer within it, you can highlight a Qt type and go directly to it's help page, it knows about Qt types when debugging....).
The nice feature in this context is that it has CMake support built in. So you can just open the project by pointing to the main CMakeLists file and then run CMake all within the IDE itself.

Building and working with CMake
###############################

* You can now start your IDE and point to or import the generated solution/project files or run <code>make</code>,<code>nmake</code> or <code>jom</code> to build the whole of Mantid (sub-targets are available - run <code>make help</code> to see them).
* '''Visual Studio users''': Use the <code>visual-studio.bat</code> generated in the build directory to start the IDE. This sets up the environment correctly.
* You should typically never have to run CMake manually again (unless you want to create a new, separate build) - it will be run automatically if one of the CMake input files changes.
* It should be rare that you will need to edit the CMake build ("CMakeLists.txt") files. The most common occurrence will be when you add a new file. This must be added to the corresponding CMakeLists file, e.g. if you add a file to Kernel, edit <code>Mantid/Framework/Kernel/CMakeLists.txt</code> to add the source, header and test files to the long lists of filepaths at the top of the file.
* The [[Useful_Tools_for_Developers#class_maker.py|class maker]] utility can edit the CMakeList.txt for you automatically
* There are similar places in the Qt projects for ui files and files that need moc-ing.
* If you add a new dependency, that will need to be added (this is less straightforward - do ask for help).
* Cache variables can be added via the CMake Gui or by running <code>ccmake</code>.

Building the installer package
##############################

* For WIndows only, you first need to install NSIS, available at: http://nsis.sourceforge.net/Download. Ensure that the install directory is added to the PATH. You should be able to type <code>makensis /?</code> in a command prompt.
* Run CMake with "ENABLE_CPACK" enabled. If using the GUI you need to click the "Advanced" checkbox to see this option.
* You will now have a build target called "PACKAGE" available to create the installer package.

More information can be found at: [[MantidPlot_Windows_Installer]]

Troubleshooting on OSX
----------------------

* If you have problems building the package because macdeplotqt fails to find the plugins folder you need to update your homebrew version of Qt (See [https://github.com/cartr/homebrew-qt4/issues/38 this] bug report for more details).

Caveats and Known Issues
########################

* For Visual Studio & XCode, the libraries and executable are put into <code>Mantid/bin/Release</code>, <code>Debug</code>, etc.
* There is a known issue with using source control with Eclipse on an out of source build. Set the cache variable ECLIPSE_CDT4_GENERATE_SOURCE_PROJECT to true and CMake will generate a set of 'dummy' project files within the source tree so that you can import that project and use it for source control actions.

Tips
####

* Running unit test executables directly with the CMake-generated Mantid.properties file will lead to a bunch of logging output to the console. You are encouraged to use CTest instead, which suppresses this output automatically. Otherwise, adding the line <code>logging.channels.consoleChannel.class = NullChannel</code> to your Mantid.user.properties file will turn if off.
* If you have more than one gcc and want to build with a version other than the default (e.g. on RedHat), setting CC & CXX environment variables is one way to make it so.
