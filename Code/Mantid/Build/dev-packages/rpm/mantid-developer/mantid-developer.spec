Name:           mantid-developer
Version:        1.10
Release:        1%{?dist}
Summary:        Meta Package to install dependencies for Mantid Development

Group:          Development/Tools
License:        GPL

BuildRoot:      %{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)

%{?fc20:Requires: rpmfusion-nonfree-release}
Requires: cmake-gui >= 2.8.12
Requires: boost-devel
%{?el6:Requires: epel-release}
Requires: gperftools-devel
Requires: gperftools-libs
Requires: gcc-c++
Requires: git-all
Requires: gsl-devel
Requires: hdf-devel
Requires: hdf5-devel
Requires: jsoncpp-devel
Requires: muParser-devel
Requires: mxml-devel
Requires: nexus >= 4.2
Requires: nexus-devel >= 4.2
Requires: numpy
Requires: OCE-devel
Requires: poco-devel
Requires: PyQt4-devel
Requires: python-devel
Requires: python-ipython >= 1.1
%{?el6:Conflicts: python-ipython >= 2.0}
Requires: python-pip
Requires: python-sphinx
Requires: qscintilla-devel
Requires: qt-devel >= 4.6
%if 0%{?el6}
Requires: qwt-devel
%else
Requires: qwt5-qt4-devel
%endif
Requires: qwtplot3d-qt4-devel
Requires: redhat-lsb
Requires: rpmdevtools
Requires: scipy
Requires: sip-devel
Requires: git
Requires: openssl-devel
Requires: texlive-latex
%if 0%{?el6}
# do nothing
%else
Requires: texlive-latex-bin
Requires: texlive-was
%endif
Requires: tex-preview
Requires: dvipng
%if 0%{?el6}
Requires: mantidlibs-qt-devel
Requires: mantidlibs-qtwebkit-devel
Requires: scl-utils
%else
Requires: qt-devel
Requires: qtwebkit-devel
%endif
Requires: graphviz

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
