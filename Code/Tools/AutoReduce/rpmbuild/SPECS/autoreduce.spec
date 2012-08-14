Summary: autoreduce
Name: autoreduce
Version: 1.2
Release: 1 
Group: Applications/Engineering
prefix: /usr
BuildRoot: %{_tmppath}/%{name}
License: Unknown
Source: autoreduce.tgz
Requires: libNeXus.so.0()(64bit) libc.so.6()(64bit) libc.so.6(GLIBC_2.2.5)(64bit)
Requires: mantid 
Requires: mantidunstable 
Requires: mantidnightly
Requires: python-suds 
%define debug_package %{nil}


%description
Autoreduce program to automatically reduce neutron data after a run

%prep
%setup -q -n %{name}

%build

%install
rm -rf %{buildroot}
mkdir -p %{buildroot}%{_sysconfdir}/autoreduce
install -m 664	../autoreduce/etc/autoreduce/icat4.cfg	 %{buildroot}%{_sysconfdir}/autoreduce/icat4.cfg
install -m 664	../autoreduce/etc/autoreduce/icatclient.properties	 %{buildroot}%{_sysconfdir}/autoreduce/icatclient.properties
install -m 755 -d 	 ../autoreduce/usr	 %{buildroot}/usr
mkdir -p %{buildroot}%{_bindir}
install -m 755	 ../autoreduce/usr/bin/ingestNexus	 %{buildroot}%{_bindir}/ingestNexus
install -m 755	 ../autoreduce/usr/bin/ingestReduced	 %{buildroot}%{_bindir}/ingestReduced
install -m 755	 ../autoreduce/usr/bin/process_run.sh	 %{buildroot}%{_bindir}/process_run.sh
mkdir -p %{buildroot}%{_libdir}
install -m 755 -d 	 ../autoreduce/usr/lib/autoreduce	 %{buildroot}%{_libdir}/autoreduce

%post
chgrp snswheel %{_sysconfdir}/autoreduce/icat4.cfg
chgrp snswheel %{_sysconfdir}/autoreduce/icatclient.properties

%files
%config %{_sysconfdir}/autoreduce/icat4.cfg
%config %{_sysconfdir}/autoreduce/icatclient.properties
%attr(755, -, -) %{_bindir}/ingestNexus
%attr(755, -, -) %{_bindir}/ingestReduced
%attr(755, -, -) %{_bindir}/process_run.sh
%attr(755, -, -) %{_libdir}/autoreduce
