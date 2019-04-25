%if 0%{?fedora} || 0%{?rhel} >= 8
  %global with_python3 1
%else
  %global with_python3 0
%endif

Name:           mantid-developer
Version:        1.30
Release:        1%{?dist}
Summary:        Meta Package to install dependencies for Mantid Development

Group:          Development/Tools
License:        GPL

BuildRoot:      %{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)

%{?fedora:Requires: rpmfusion-nonfree-release}
%{?rhel:Requires: epel-release}
Requires: clang
%{?fedora:Requires: cmake-gui}
%{?rhel:Requires: cmake3-gui}
Requires: boost-devel
Requires: doxygen
Requires: gperftools-devel
Requires: gperftools-libs
Requires: gcc-c++
Requires: git-all
Requires: gsl-devel
Requires: hdf-devel
Requires: hdf5-devel
Requires: h5py >= 2.3.1
Requires: jsoncpp-devel >= 0.7.0
Requires: librdkafka-devel
Requires: muParser-devel
Requires: mxml-devel
Requires: nexus >= 4.2
Requires: nexus-devel >= 4.2
Requires: ninja-build
Requires: numpy
Requires: OCE-devel
Requires: poco-devel >= 1.4.6
Requires: PyQt4-devel
Requires: python-qt5-devel
Requires: python-QtPy
Requires: python-requests
Requires: python-devel
Requires: python-setuptools
Requires: python-ipython >= 1.1
Requires: python-matplotlib
%{?fedora:Requires: python2-matplotlib-qt4}
%{?el7:Requires: python-matplotlib-qt4}
Requires: python-pip
%{?fedora:Requires: python2-qtconsole}
Requires: python-sphinx
Requires: python2-sphinx-bootstrap-theme
Requires: PyYAML
Requires: python2-mock
Requires: python2-psutil
Requires: qscintilla-devel
Requires: qt-devel >= 4.6
Requires: qwt5-qt4-devel
Requires: qwtplot3d-qt4-devel
Requires: redhat-lsb
Requires: rpmdevtools
Requires: scipy
Requires: python2-scikit-image
Requires: sip-devel
Requires: tbb
Requires: tbb-devel
Requires: git
Requires: openssl-devel
Requires: texlive-latex
Requires: texlive-latex-bin
Requires: texlive-was
Requires: tex-preview
Requires: dvipng
Requires: qt-devel
Requires: qtwebkit-devel
Requires: qt5-qtbase-devel
Requires: qt5-qtbase-gui
Requires: qt5-qtimageformats
Requires: qt5-qtsvg
Requires: qt5-qttools-devel
Requires: qt5-qttools-libs-designer
Requires: qt5-qtwebkit-devel
Requires: qt5-qtx11extras
Requires: qt5-qtx11extras-devel
Requires: qscintilla-qt5-devel
Requires: graphviz
%if %{with_python3}
Requires: python3-setuptools
Requires: python3-sip-devel
Requires: python3-PyQt4-devel
Requires: python-qt5-devel
Requires: python3-QtPy
Requires: python3-numpy
Requires: python3-scipy
Requires: python3-scikit-image
Requires: python3-sphinx
Requires: python3-sphinx-bootstrap-theme
Requires: python3-dateutil
Requires: python3-h5py
Requires: python3-ipython-gui
Requires: python3-matplotlib
%{?fedora:Requires: python3-qtconsole}
Requires: python3-PyYAML
Requires: python3-mock
%{?fedora:Requires: python3-psutil}
%{?fedora:Requires: python3-requests}
Requires: boost-python3-devel
%endif

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

* Thu Apr 25 2019 Samuel Jones <samuel.jones@stfc.ac.uk>
- Removed qtawesome

* Thu Nov 18 2018 Martyn Gigg <martyn.gigg@stfc.ac.uk>
- Added python-requests

* Wed Jul 18 2018 Martyn Gigg <martyn.gigg@stfc.ac.uk>
- Added python-psutil

* Wed Apr 25 2018 Steven Hahn <hahnse@ornl.gov>
- Add qt5-qtx11extras
- remove RHEL6-specific packages

* Mon Jan 22 2018 Martyn Gigg <martyn.gigg@stfc.ac.uk>
- Added qtawesome

* Tue Dec 05 2017 Peter Peterson <petersonpf@ornl.gov>
- Added cmake3-gui for rhel

* Wed Nov 22 2017 Martyn Gigg <martyn.gigg@stfc.ac.uk>
- Added Qt5, PyQt5 and QtPy dependencies

* Thu Jul 20 2017 Peter Peterson <petersonpf@ornl.gov>
- Added python-qtconsole for fedora

* Sat Feb 18 2017 Stuart Campbell <scampbell@bnl.gov>
- Updated to use upstream sphinx-bootstrap-theme

* Mon Jan 09 2017 Lamar Moore <lamar.moore@stfc.ac.uk>
- Require librdkafka-dev

* Wed Dec 21 2016 Martyn Gigg <martyn.gigg@stfc.ac.uk>
- Require python-mock & python3-mock on fedora

* Fri Nov 18 2016 Martyn Gigg <martyn.gigg@stfc.ac.uk>
- Require PyYAML

* Fri Sep 23 2016 Stuart Campbell <campbellsi@ornl.gov>
- Require poco >= 1.4.6

* Thu Aug 04 2016 Peter Peterson <petersonpf@ornl.gov>
- Require sphinx-bootstrap, ninja, and python3 packages on fedora

* Tue Aug 02 2016 Peter Peterson <petersonpf@ornl.gov>
- Require tbb

* Wed May 18 2016 Martyn Gigg <martyn.gigg@stfc.ac.uk>
- Require h5py >= 2.3.1

* Tue May 03 2016 Pete Peterson <petersonpf@ornl.gov>
- Require python-matplotlib-qt4 and h5py

* Mon Nov 30 2015 Steven Hahn <hahnse@ornl.gov>
- Require jsoncpp-devel >= 0.7.0

* Wed Jul 01 2015 Steven Hahn <hahnse@ornl.gov>
- Added python-matplotlib dependency

* Thu Feb 12 2015 Harry Jeffery <henry.jeffery@stfc.ac.uk>
- Added graphviz dependency

* Wed Aug 13 2014 Peter Peterson <petersonpf@ornl.gov>
- Merged all three distribution spec files into one

* Fri Apr 25 2014 Michael Reuter <reuterma@ornl.gov>
- Added texlive-latex-bin, texlive-was, tex-preview

* Thu Apr 10 2014 Peter Peterson <petersonpf@ornl.gov>
- Added qtwebkit-devel

* Tue Feb 04 2014 Stuart Campbell <campbellsi@ornl.gov>
- Added scipy and ipython >= 1.1 dependency

* Fri Dec 20 2013 Stuart Campbell <campbellsi@ornl.gov>
- Added python-sphinx

* Thu Dec 19 2013 Stuart Campbell <campbellsi@ornl.gov>
- Changed to use OCE rather than OpenCASCADE.

* Tue Aug 20 2013 Peter Peterson <petersonpf@ornl.gov>
- Removed things not necessary for fedora 19.

* Tue May 07 2013 Stuart Campbell <campbellsi@ornl.gov>
- Added dvipng and latex for qt-assistant stuff
- Added software collection dependencies

* Thu Jun  7 2012 Russell Taylor <taylorrj@ornl.gov>
- Remove gmock & gtest now that we include them in our repo
- Remove subversion dependency now that we use git

* Mon Mar 19 2012 Stuart Campbell <campbellsi@ornl.gov>
- Updated for google-perftools -> gperftools package rename.

* Wed Feb 22 2012 Stuart Campbell <campbellsi@ornl.gov>
- Added nexus as it is not required by it's devel package.

* Wed Feb 22 2012 Stuart Campbell <campbellsi@ornl.gov>
- Added git as a dependency
- Added openssl-devel dependency

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
