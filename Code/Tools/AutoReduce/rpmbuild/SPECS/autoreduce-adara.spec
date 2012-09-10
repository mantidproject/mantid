Summary: autoreduce-adara
Name: autoreduce-adara
Version: 1.0
Release: 1 
Group: Applications/Engineering
prefix: /usr
BuildRoot: %{_tmppath}/%{name}
License: Unknown
Source: autoreduce-adara.tgz
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
install -m 664	../autoreduce-adara/etc/autoreduce/icat4.cfg	 %{buildroot}%{_sysconfdir}/autoreduce/icat4.cfg
install -m 664	../autoreduce-adara/etc/autoreduce/icatclient.properties	 %{buildroot}%{_sysconfdir}/autoreduce/icatclient.properties
install -m 755 -d 	 ../autoreduce-adara/usr	 %{buildroot}/usr
mkdir -p %{buildroot}%{_bindir}
install -m 755	 ../autoreduce-adara/usr/bin/ingestNexus_adara	 %{buildroot}%{_bindir}/ingestNexus_adara
install -m 755	 ../autoreduce-adara/usr/bin/ingestReduced_adara	 %{buildroot}%{_bindir}/ingestReduced_adara
install -m 755	 ../autoreduce-adara/usr/bin/process_run_adara.sh	 %{buildroot}%{_bindir}/process_run_adara.sh

%post
chgrp snswheel %{_sysconfdir}/autoreduce/icat4.cfg
chgrp snswheel %{_sysconfdir}/autoreduce/icatclient.properties

%files
%config %{_sysconfdir}/autoreduce/icat4.cfg
%config %{_sysconfdir}/autoreduce/icatclient.properties
%attr(755, -, -) %{_bindir}/ingestNexus_adara
%attr(755, -, -) %{_bindir}/ingestReduced_adara
%attr(755, -, -) %{_bindir}/process_run_adara.sh
