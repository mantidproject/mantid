Name:           mantid-developer
Version:        1.0
Release:        5%{?dist}
Summary:        Meta Package to install dependencies for Mantid Development

Group:          Development/Tools
License:        GPL

BuildRoot:      %{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)


Requires: cmake-gui >= 2.8.5
Requires: boost-devel
Requires: epel-release 
Requires: google-perftools-devel
Requires: gcc-c++
Requires: gsl-devel
Requires: hdf-devel
Requires: hdf5-devel
Requires: muParser-devel
Requires: mxml-devel
Requires: nexus-devel >= 4.2
Requires: numpy
Requires: OpenCASCADE-devel
Requires: poco-devel
Requires: PyQt4-devel
Requires: python-devel
Requires: qscintilla-devel
Requires: qt-devel >= 4.6
Requires: qwt-devel
Requires: qwtplot3d-qt4-devel
Requires: redhat-lsb
Requires: rpmdevtools
Requires: sip-devel
Requires: subversion 
Requires: gmock-devel
Requires: gtest-devel
Requires: git

BuildArch: noarch

%description
A virtual package which requires all the dependencies and tools that are 
required for Mantid development.

%prep

%build

%install

%clean

%post
# Remove myself once I have installed all the required packages.
#rpm -e %{name}

%files

%changelog
* Wed Feb 22 2012 Stuart Campbell <campbellsi@ornl.gov>
- Added git as a dependency

* Mon Feb 20 2012 Stuart Campbell <campbellsi@ornl.gov>
- Added dependency on NeXus development after nexus rpm split.
- Updated CMake dependency to 2.8.5 following 'the virus'!
- Added Google Mock and GTest.
 
* Fri Jun  3 2011 Stuart Campbell <campbellsi@ornl.gov>
- Added rpmdevtools and lsb dependencies

* Fri Jun  3 2011 Stuart Campbell <campbellsi@ornl.gov>
- Added versions for some packages

* Fri Jun  3 2011 Stuart Campbell <campbellsi@ornl.gov>
- Initial release
