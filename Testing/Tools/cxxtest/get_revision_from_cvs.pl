#!/usr/bin/perl -w

#
# This file is a complete mess.
#

use strict;
use File::Find;
use Getopt::Long;
use POSIX;
use English;

sub usage() {
  print STDERR "Usage: $0 [OPTIONS]\n";
  print STDERR "\n";
  print STDERR "Get current or specified version of CxxTest from the CVS server.\n";
  print STDERR "Creates `.tar.gz', `.zip', `.pdf' and `.noarch.rpm' files.\n";
  print STDERR "\n";
  print STDERR "  --cvsroot=CVSROOT    Use CVSROOT\n";
  print STDERR "  --version=X.Y.Z      Get version X.Y.Z\n";
  print STDERR "  --rpmbuild=CMD       Use CMD to build RPM (default = rpm)\n";
  print STDERR "  --no-test            Do not run the automated tests.\n";
  print STDERR "  --no-tgz             Do not create `.tar.gz' archive\n";
  print STDERR "  --no-zip             Do not create `.zip' archive\n";
  print STDERR "  --no-rpm             Do not create RPM\n";
  print STDERR "  --no-pdf             Do not create PDF\n";
  print STDERR "\n";
  exit -1;
}

$OUTPUT_AUTOFLUSH = 1;

my $outputDir = POSIX::getcwd();
my $workDir = POSIX::tmpnam();
my $sourceDir = "$workDir/cxxtest";
my $cvsroot = ':ext:erez_v@cvs.sourceforge.net:/cvsroot/cxxtest';
my $tag = 'HEAD';
my ($second, $minute, $hour, $day, $month, $year, $weekday, $yearday) = gmtime(time);
my $version = sprintf('%04d%02d%02d_%02d%02d%02d', $year + 1900, $month + 1, $day, $hour, $minute, $second);
my $archive = "cxxtest-$version";
my $rpmbuild = "rpm";
my ($noTest, $noTgz, $noZip, $noRpm, $noPdf);

sub setVersion($$) {
  $version = $_[1];
  if ( $version =~ m/(\d+)\.(\d+)\.(\d+)/ ) {
    $tag = "cxxtest-$1_$2_$3";
    $archive = "cxxtest-$1.$2.$3";
  }
  else {
    print STDERR "Warning: Invalid version `$version' ignored\n";
  }
}

GetOptions( 'cvsroot=s' => \$cvsroot,
            'version=s' => \&setVersion,
            'rpmbuild=s' => \$rpmbuild,
	    'no-test' => \$noTest,
            'no-tgz' => \$noTgz,
            'no-zip' => \$noZip,
	    'no-rpm' => \$noRpm,
	    'no-pdf' => \$noPdf
	  ) or usage();

if ( !$noRpm && $noTgz ) {
  print STDERR "Warning: Cannot create RPM when --no-tgz specified\n";
  $noRpm = 1;
}

sub removeDir($) {
  my ($dir) = @_;
  finddepth( { 'no_chdir' => 1, 'wanted' => sub { (-d $_) ? rmdir $_: unlink $_; } }, $dir );
  rmdir $dir;
  die "Cannot remove `$dir/'\n" unless ( ! -e $dir );
}

sub removeSourceDir() {
  removeDir( "$sourceDir" );
}

sub removeWorkDir() {
  removeDir( "$workDir" );
}

sub fail($) {
  my ($message) = @_;
  removeWorkDir();
  die $message;
}

print "Preparing work directory...";
if ( -e $workDir ) {
  removeWorkDir();
}

sub md($) {
  my ($dir) = @_;
  mkdir( $dir, 0777 ) or fail "Cannot create `$dir/'\n";
}

md( $workDir );
print "OK.\n";

sub cd($) {
  chdir $_[0] or fail "Cannot cd to `$_[0]'\n";
}

print "Getting $archive from CVS...";
cd( $workDir );
$ENV{CVS_RSH} = "ssh";
system("cvs -z9 -Q -d $cvsroot export -r $tag cxxtest") == 0
  or fail "Cannot get sources from CVS\n";
system("tar cf cxxtest-cvs.tar cxxtest") == 0
  or fail "Cannot create cxxtest-cvs.tar\n";
print "OK.\n";

if ( !$noTest ) {
  print "Running automated tests...";
  cd( "$sourceDir/test" );
  system( 'perl -w test.pl --dots' ) == 0 or fail "Error in tests, not creating distribution\n";
  print "OK.\n";
}

print "Preparing to archive...";
finddepth( { 'no_chdir' => 1, 'wanted' => sub { unlink $_ if m/.cvsignore/ } }, $workDir );
unlink "$sourceDir/get_revision_from_cvs.pl";
unlink "$sourceDir/docs/Makefile";
unlink "$sourceDir/docs/guide.texi";
unlink "$sourceDir/docs/texinfo.tex";

my $fix = "$workDir/fix.ex";
open FIX, ">$fix" or fail "Cannot create file `$fix'\n";
print FIX ":%s/INSERT_VERSION_HERE/$version/g\n";
print FIX ":wq\n";
close FIX;
system( "ex -s $sourceDir/python/scripts/cxxtestgen < $fix" ) == 0 or fail "Error fixing version in Python script\n";
system( "ex -s $sourceDir/cxxtest.spec < $fix" ) == 0  or fail "Error fixing version in spec file\n";

cd( $workDir );
system("tar cf cxxtest.tar cxxtest") == 0
  or fail "Cannot create cxxtest.tar\n";
print "OK.\n";

sub makeArchive($$) {
  my ($archive, $command) = @_;
  my $fullPath = "$outputDir/$archive";

  print "Creating `$archive'...";
  cd( $workDir );
  unlink $fullPath or fail "Cannot remove $fullPath\n" unless ( ! -f $fullPath );
  $command =~ s/ARCHIVE/$fullPath/;
  system( $command ) == 0 or fail "Cannot create `$archive'\n";
  print "OK.\n";
}

sub makeArchives($$) {
  my ($prefix, $dir) = @_;
  makeArchive( "$prefix$version.tar.gz", "tar cf - $dir |gzip -9 > ARCHIVE" ) unless $noTgz;
  makeArchive( "$prefix$version.zip", "zip -qr9 ARCHIVE $dir" ) unless $noZip;
}

sub restoreSources($) {
  my ($file) = @_;
  print "Opening archive...";
  cd( $workDir );
  removeDir( $sourceDir );
  system( "tar xf $file" ) == 0
    or fail "Cannot open cxxtest.tar\n";
  print "OK.\n";
}

restoreSources( "cxxtest.tar" );
removeDir( "$sourceDir/test" );
makeArchives( "cxxtest-", "cxxtest" );

restoreSources( "cxxtest.tar" );
makeArchives( "cxxtest-selftest-", "cxxtest/test" );

if ( ! $noRpm ) {
  print "Creating `$archive-1.noarch.rpm'...";

  my $rpmDir = "$workDir/rpm";
  md( $rpmDir );
  cd( $rpmDir );

  my $rpmMacros = "$rpmDir/.rpmmacros";
  open RPM_MACROS, ">$rpmMacros" or fail "Cannot create file `$rpmMacros'\n";
  print RPM_MACROS "%_topdir\t$rpmDir\n";
  print RPM_MACROS "%_rpmdir\t$outputDir\n";
  print RPM_MACROS "%_rpmfilename\t%%{NAME}-%%{VERSION}-%%{RELEASE}.%%{ARCH}.rpm\n";
  print RPM_MACROS "%packager\tErez Volk\n";
  foreach my $item ('_builddir', '_sourcedir', '_specdir', '_srcrpmdir', '_tmppath') {
    print RPM_MACROS "%$item\t%{_topdir}\n";
  }
  close RPM_MACROS;

  system( "HOME=$rpmDir $rpmbuild -tb --quiet $outputDir/$archive.tar.gz > /dev/null" ) == 0 or fail "Error creating RPM\n";
  print "OK.\n";
}

if ( !$noPdf ) {
  restoreSources( "cxxtest-cvs.tar" );
  makeArchive( "cxxtest-guide-$version.pdf", "make -sC $sourceDir/docs guide.pdf > /dev/null && cp $sourceDir/docs/guide.pdf ARCHIVE" );
}

print "Cleaning up...";
removeWorkDir();
print "OK.\n";

#
# Local Variables:
# compile-command: "perl get_revision_from_cvs.pl"
# End:
#
