Summary: autoreduce
Name: autoreduce
Version: 1.1
Release: 2 
Group: Applications/Engineering
prefix: /usr
BuildRoot: %{_tmppath}/%{name}
License: Unknown
Source: autoreduce.tgz
Requires: libNeXus.so.0()(64bit) libc.so.6()(64bit) libc.so.6(GLIBC_2.2.5)(64bit)
Requires: mantid 
Requires: mantidunstable 
Requires: mantidnightly
%define debug_package %{nil}


%description
Autoreduce program to automatically reduce neutron data after a run

%prep
%setup -q -n %{name}

%build

%install
rm -rf %{buildroot}
mkdir -p %{buildroot}%{_sysconfdir}/autoreduce
install -m 664	../autoreduce/etc/autoreduce/mapping.xml	 %{buildroot}%{_sysconfdir}/autoreduce/mapping.xml
install -m 664	../autoreduce/etc/autoreduce/auto_reduce.conf	 %{buildroot}%{_sysconfdir}/autoreduce/auto_reduce.conf
install -m 664	../autoreduce/etc/autoreduce/icatclient.properties 	 %{buildroot}%{_sysconfdir}/autoreduce/icatclient.properties 
install -m 664	../autoreduce/etc/autoreduce/cacerts.jks	 %{buildroot}%{_sysconfdir}/autoreduce/cacerts.jks
install -m 755 -d 	 ../autoreduce/usr	 %{buildroot}/usr
mkdir -p %{buildroot}%{_bindir}
install -m 755	 ../autoreduce/usr/bin/nxingest	 %{buildroot}%{_bindir}/nxingest-autoreduce
install -m 644	 ../autoreduce/usr/bin/create_reduced_metadata.py	 %{buildroot}%{_bindir}/create_reduced_metadata.py
install -m 755	 ../autoreduce/usr/bin/process_run.sh	 %{buildroot}%{_bindir}/process_run.sh
mkdir -p %{buildroot}%{_libdir}
install -m 755 -d 	 ../autoreduce/usr/lib/autoreduce	 %{buildroot}%{_libdir}/autoreduce
install -m 644	 ../autoreduce/usr/lib/autoreduce/icat3-xmlingest-client-1.0.0-SNAPSHOT.jar	 %{buildroot}%{_libdir}/autoreduce/icat3-xmlingest-client-1.0.0-SNAPSHOT.jar
install -m 644	 ../autoreduce/usr/lib/autoreduce/cacerts.jks	 %{buildroot}%{_libdir}/autoreduce/cacerts.jks

%post
chgrp snswheel %{_sysconfdir}/autoreduce/mapping.xml
chgrp snswheel %{_sysconfdir}/autoreduce/auto_reduce.conf
chgrp snswheel %{_sysconfdir}/autoreduce/icatclient.properties 
chgrp snswheel %{_sysconfdir}/autoreduce/cacerts.jks

%files
%config %{_sysconfdir}/autoreduce/mapping.xml
%config %{_sysconfdir}/autoreduce/auto_reduce.conf
%config %{_sysconfdir}/autoreduce/icatclient.properties 
%config %{_sysconfdir}/autoreduce/cacerts.jks 
%attr(755, -, -) %{_bindir}/nxingest-autoreduce
%attr(755, -, -) %{_bindir}/create_reduced_metadata.py
%attr(755, -, -) %{_bindir}/process_run.sh
%attr(755, -, -) %{_libdir}/autoreduce
%attr(644, -, -) %{_libdir}/autoreduce/icat3-xmlingest-client-1.0.0-SNAPSHOT.jar
%attr(644, -, -) %{_libdir}/autoreduce/cacerts.jks
