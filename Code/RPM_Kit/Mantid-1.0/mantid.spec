Name:	Mantid
Summary:	Data Analysis For ISIS
Version:	1.0
Release:	1%{?dist}
URL:		http://www.mantidproject.org/
License:	GPLv2+
Group:	Applications/Publishing
Source0:  %{name}-%{version}.tar.gz

BuildRequires:  python >= 2.5
BuildRequires:  scons >= 0.98.0
BuildRequires:  boost >= 1.34.1
BuildRequires:  poco >= 1.3.0
BuildRequires:  poco-devel >= 1.3.0
BuildRequires:  glibc-devel
BuildRequires:  gsl-devel
BuildRequires:  gts-devel

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

%install
python Mantid/src/Build/LinuxBuildScripts/cleanup.py
cp -rf Mantid/ /usr/local/.

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(-, root, root)
/usr/local/Mantid/*


