Name:	Mantid
Summary:	Data Analysis For ISIS
Version:	1.0
Release:	1%{?dist}
URL:		http://www.mantidproject.org/
License:	GPLv2+
Group:	Applications/Publishing
Source0:  %{name}-%{version}.tar.gz
Prefix: /tmp

BuildRequires:  python >= 2.3
BuildRequires:  scons >= 0.97
BuildRequires:  boost >= 1.34.1
BuildRequires:  glibc-devel
BuildRequires:  gsl-devel
#required for mantidplot
BuildRequires:	qt4-devel >= 4.2 
BuildRequires:	qwt-devel
BuildRequires:	qwtplot3d-qt4-devel
BuildRequires:	muParser-devel
BuildRequires:	PyQt4-devel

%description
Mantid

%prep
%setup -q
rm -f Mantid/src/Build/Tests/*
cd Mantid/src
sed -i "s!rpmBuild = False!rpmBuild = True!" SConstruct
sed -i "s!USRLOCALINCLUDE = '/usr/local/include'!USRLOCALINCLUDE = '/usr/local/include'!" SConstruct
sed -i "s!USRINCLUDE = '/usr/include'!USRINCLUDE = '%{_includedir}'!" SConstruct
sed -i "s!USRLIB = '/usr/lib'!USRLIB = '%{_libdir}'!" SConstruct

%build
cd Mantid/src/
scons skiptest=1
sed -i "s!rpmBuild = True!rpmBuild = False!" SConstruct
cd ../MantidPlot/
python release_date.py
cd qtiplot/
sed -i "s!../../../Images/images.qrc!../../images/images.qrc!" qtiplot.pro
qmake-qt4
make
sed -i "s!../../images/images.qrc!../../../Images/images.qrc!" qtiplot.pro

%install
python Mantid/src/Build/LinuxBuildScripts/cleanup.py
cp -rf Mantid/ /tmp/.

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(-, root, root)
/tmp/Mantid/*


