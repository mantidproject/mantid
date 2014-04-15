* In order to be able to use xvnc in a matrix job, the "Disable Xvnc execution on this node" should be selected for all Mac nodes.
* Machines that are set up to build Mantid should have a label of the form OS-build, where OS is one of rhel6, ubuntu-12.04, osx-10.8, win7.
* Machines that have any of the labels rhel6, ubuntu-12.04, osx-10.8 or win7 will be considered eligible for running system tests. Be sure not to add these specific labels if a machine should not be used for system tests!
