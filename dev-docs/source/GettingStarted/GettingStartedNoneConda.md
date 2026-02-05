# Develop without conda

## Environment

Some initial setup is required before being able to build the code.
This is platform specific and described here.

### Windows

Install the following:

- [Visual Studio 2019 Community Edition](https://visualstudio.microsoft.com/downloads/)
  - When asked about installation workloads choose `Desktop development with C++`
  - Under the "Installation details" section verify that the following are checked:
    - `Windows Universal CRT SDK`
    - The latest Windows 10 SDK
  - If your machine has less than 32GB of memory Mantid may not build.
    If you have problems change the maximum number of parallel project builds to 1 in Visual Studio in Tools -> Options -> Projects and Solutions -> Build And Run.
- [Git](https://git-scm.com/)
  - install the latest version and ensure that Git LFS is checked to be included
  - when the install has completed create a directory for storage of the LFS objects, e.g. `C:\GitLFSStorage`
  - open up Git Bash and run `git config --global lfs.storage C:/GitLFSStorage`
  - run `git lfs install` to initialize Git LFS.
    (Note that if you miss this step you may get errors due to the third party libraries not checking out properly.
    This can be fixed later by running `git lfs fetch` and `git lfs checkout` in the `external\src\ThirdParty` directory.)
- [CMake](https://cmake.org/download/) >= 3.21
- [MiKTeX](https://miktex.org/download).
  Installation instructions are [available here](https://miktex.org/howto/install-miktex).
  Once installed:
  - open the MikTeX console from the start menu
  - switch to administrator mode
  - settings -> select "Always install missing packages on-the-fly"
- [NSIS](http://nsis.sourceforge.net/Download) (optional). Used for building packages

[Graphviz](http://graphviz.org/download/) is required to generate the workflow diagrams in the documentation.
Unfortunately CMake can't find it out of the box and the following steps are required to make this link

- open regedit
- add a new key `[HKEY_LOCAL_MACHINE\SOFTWARE\ATT\Graphviz]`
- create a new string value named `InstallPath` within this key and set the value
  to point to the install directory of Graphviz.

#### Windows Subsystem for Linux (WSL2)

It is also possible to install a Linux subsystem within Windows by following [these](WindowsSubsystemForLinux) instructions.
This step is optional.

### Linux

#### Red Hat/Cent OS/Fedora

- Follow the [instructions here](https://fedoraproject.org/wiki/EPEL) to enable the EPEL repository for RHEL7
- Run the following to install the mantid-developer package

```sh
# Install copr plugin
yum install yum-plugin-copr

# Enable the mantid repo from copr
yum copr enable mantid/mantid

# Install dependencies
yum install mantid-developer

# Install pre-commit
pip3 install pre-commit --user
```

On fedora, the `yum` commands should be replaced with `dnf`.
For systems with default python3 the `pip3` command can be replaced with `pip`, but it should work either way.

Make sure you install [devtoolset-7](https://developer.mantidproject.org/BuildingWithCMake.html#from-the-command-line) as described in the link.

Now you can [get the mantid code](https://developer.mantidproject.org/GettingStarted.html#getting-the-mantid-code), and build it:

```sh
mkdir build
cd build
scl enable devtoolset-7 "cmake3 [mantid source]"
cmake3 --build .
```

See the instructions on [this](RunningTheUnitTests) page to run the Mantid unit tests.

#### Ubuntu 18.04

- Setup the Kitware APT repository to get a recent version of CMake by
  following [these instructions](https://apt.kitware.com/)
- Follow the [Ubuntu instructions](http://download.mantidproject.org/ubuntu.html)
  to add the stable release repository and mantid ppa and
- Download the latest
  [mantid-developer](https://sourceforge.net/projects/mantid/files/developer)
  package and install it:

```sh
apt install gdebi-core
gdebi ~/Downloads/mantid-developer.X.Y.Z.deb
```

where `X.Y.Z` should be replaced with the version that was downloaded.

Install pre-commit for use in our current developer workflow

```sh
pip install pre-commit --user
```

if you wish to setup eclipse for use developing mantid, then instructions can be found [here](Eclipse).

Now you can [Clone Mantid](wsl-cloning-mantid-ref), and build as follows:

```sh
mkdir build
cd build
cmake -G Ninja [mantid source]
cmake --build .
```

See the instructions on [this](RunningTheUnitTests) page to run the Mantid unit tests.

#### Ubuntu 20.04

- Mantid uses `qtpy` to talk to Python bindings of Qt.
  It is recommended to have the environment var `QT_API=pyqt5` exported to the shell before building with CMake.
- The header and lib shipped with Anaconda (if installed) could interfere with Mantid building
  process. It is highly recommended to remove Anaconda Python from your env prior to building
  using `conda deactivate`.
- Mantid is not yet officially supported on Ubuntu 20.04, but mantid-developer package (see Ubuntu 18.04 instructions) has been modified to support it.
- Install pre-commit for use in our current developer workflow

```sh
pip install pre-commit --user
```

### OSX

The build environment on OS X is described here [Building On Osx](BuildingOnOSX).

Install pre-commit for use in our current developer workflow

```sh
brew install pre-commit
```

### Docker

On Docker supported systems you may use the
[mantid-development](https://github.com/mantidproject/dockerfiles/tree/main/development)
images to develop Mantid without having to configure your system as a suitable
build environment. This will give you an out of the box working build
environment, Python 3 (where available) and ccache.

More details and instructions can be found at the GitHub link above.

## Getting the Mantid code

We use [Git](https://git-scm.com/) as our version control system (VCS).
The master copies of our repositories are located at [GitHub](http://github.com/mantidproject).
We have a number of repositories, of which the main one (the one containing all the source code for Mantid itself) is called simply [mantid](http://github.com/mantidproject/mantid).

If you are not already set up with Git, you can follow these [instructions](https://git-scm.com/book/en/v2/Getting-Started-First-Time-Git-Setup).

There are a number of URLs via which the code can be checked out using various protocols.
The easiest way to get the one you want is to select the protocol you want on the right side of the [mantid](http://github.com/mantidproject/mantid) repository page on github and copy the url into your clipboard.
The way to clone the repository via ssh on the command line, into a directory called Mantid, is:

```sh
git clone git@github.com:mantidproject/mantid.git
```

Alternatively, one can use the `https` protocol for cloning the repository.
This requires one to supply an authentication token when pushing or re-type their password.

```sh
git clone https://github.com/mantidproject/mantid.git
```
