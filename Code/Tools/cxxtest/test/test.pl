#!/usr/bin/perl -w

#
# This script runs various "black-box" tests of CxxTest.  You don't
# really need it, but you can use it to see if CxxTest works as
# expected in your environment.  It is not currently well documented
# or readable, but it'll get there...
#
# If it doesn't work for you, you can try:
#  - Running it with --no-python
#  - Running it with --compiler="YOUR_COMPILER"
#
# If that still doesn't work, take a look at the generated `build.log'.
# If you find any hints in there, please let me know.
#

use strict;
use Getopt::Long;
use English;

# Automatic flush of STDOUT
$OUTPUT_AUTOFLUSH = 1;

#
# Some command line options
#
my $noPython = 0;
my $noPerl = 1;
my $compiler;
my $compilee;
my $exe_option = '-o ';
my $include_option = '-I';
my $no_eh_option;
my $qtFlags = "-Ifake";
my $x11Flags = "-Ifake";
my $w32Flags = "-Ifake";
my $p = '';
my ($quiet, $dots);
GetOptions( 'no-python' => \$noPython,
	    'no-perl' => \$noPerl,
	    'compiler=s' => \$compiler,
	    'exe-option=s' => \$exe_option,
	    'include-option=s' => \$include_option,
	    'no-eh-option=s' => \$no_eh_option,
	    'quiet' => \$quiet,
	    'dots' => \$dots,
	    'prefix=s' => \$p,
	    'qt=s' =>\$qtFlags,
	    'win32=s' =>\$w32Flags,
	    'x11=s' =>\$x11Flags,
	    'bcc32' => sub { $compiler = 'bcc32 -w-'; $exe_option = '-e'; $no_eh_option = '-x-' },
	    'gcc' => sub { $compiler = 'g++ -g -Wall -W -Wshadow -Woverloaded-virtual -Wnon-virtual-dtor -Wreorder -Wsign-promo'; $no_eh_option = '-fno-exceptions' }
	  );

#
# --no-python --no-perl is meaningless
#
if ( $noPerl && $noPython ) {
  die "You can specify at most one of --no-perl and --no-python\n";
}

#
# Make sure we have Perl and/or Python
#
if ( !$noPerl ) {
  system( "perl -v > ${p}pl.out 2>&1" ) == 0 or die "Perl not found!\n";
}

if ( !$noPython ) {
  system( "python -c exit > ${p}py.out 2>&1" ) == 0 or die "Python not found (try --no-python)\n";
}

my ($plCpp, $pyCpp) = ("${p}pl.cpp", "${p}py.cpp");
my $rootCpp = "${p}root.cpp";
$compilee = $noPerl ? $pyCpp : $plCpp;

#
# Simplistic compiler selection
#
my $target;

if ( "$^O" eq 'MSWin32' ) {
  if ( !defined($compiler) ) {
    # Default to MSVC which should be in the path
    $compiler = 'cl -nologo -GX -W4 -WX';
    $no_eh_option = '-GX-';
  }
  $target = "${p}px.exe";
}
else {
  defined($compiler) or $compiler = 'c++ -Wall -W -Werror -g';
  $target = "./${p}px";
}

sub compile($) {
  my ($flags) = @_;
  if ( !$flags ) {
    $flags = "";
  }
  system( "${compiler} ${exe_option}${target} ${include_option}. ${include_option}../ $flags ${compilee} > ${p}build.log 2>&1" );
}

sub compileWithMain($) {
  my ($flags) = @_;
  if ( !$flags ) {
    $flags = "";
  }
  system( "${compiler} ${exe_option}${target} ${include_option}. ${include_option}../ $flags main.cpp ${compilee} > ${p}build.log 2>&1" );
}

#
# Compare two files line by line
#
sub filesEqual($$) {
  my ($file1, $file2) = @_;
  my $equal;

  open FILE1, "<$file1" or die "Cannot open $file1\n";
  open FILE2, "<$file2" or die "Cannot open $file2\n";
  while ( !defined($equal) ) {
    my $line1 = <FILE1>;
    my $line2 = <FILE2>;

    if ( defined($line1) xor defined($line2) ) {
      $equal = 0;
    } elsif ( !defined($line1) ) {
      $equal = 1;
    } else {
      chomp $line1;
      chomp $line2;
      if ( $line1 ne $line2 ) {
	$equal = 0;
      }
    }
  }

  close FILE2;
  close FILE1;

  return $equal;
}

#
# Print something (unless we were told to keep quiet)
#
sub say(@$) {
  if ( $quiet ) {
  }
  elsif ( $dots ) {
    print ".";
  }
  else {
    print @_;
  }
}

#
# Append a line to a file that reports the error level
#
sub appendErrorLevel() {
  my $errorLevel = $? >> 8;
  open FILE, ">>${p}px.pre";
  print FILE "Error level = $errorLevel\n";
  close FILE;
}

#
# Create the "root" .cpp file
#
sub makeRoot($) {
  my $test = $_[0];
  my ($name, $output, $args) = ($test->{'name'}, $test->{'output'}, $test->{'args'});

  generate( $test );
  comparePlPy();

  my $input = $noPerl ? $pyCpp : $plCpp;
  open INPUT, "<$input" or die "Missing file $input\n";
  open OUTPUT, ">$rootCpp" or die "Cannot create $rootCpp\n";
  while (<INPUT>) {
    print OUTPUT;
  }
  close OUTPUT;
  close INPUT;
}

#
# Test the root/part thing
#  - Generate source files
#  - Compile
#  - Compare output to reference file
#
sub testRoot($) {
  my $test = $_[0];
  my ($name) = ($test->{'name'});

  clean();
  say( "$name: " );

  my $files = "$rootCpp";
  makeRoot { 'args' => "--have-eh --abort-on-fail --root --error-printer" };

  foreach my $i (1, 2) {
    my $args = "--have-eh --abort-on-fail --part Part${i}.h";

    if ( !$noPython ) {
      say "python...";
      system( "python ../python/scripts/cxxtestgen -o ${p}py${i}.cpp $args > ${p}py.out 2>&1" );
    }

    if ( !$noPython && !$noPerl ) {
      say "compare...";
      if ( !filesEqual( "${p}pl${i}.cpp", "${p}py${i}.cpp" ) ) {
	die "Different source files generated (${p}pl${i}.cpp <> ${p}py${i}.cpp)\n";
      }
    }

    $files .= $noPerl ? " ${p}py${i}.cpp" : " ${p}pl${i}.cpp";
  }

  say "compile...";
  system( "${compiler} ${exe_option}${target} ${include_option}. ${include_option}.. ${files} > ${p}build.log 2>&1" );
  die "Error building runner, see ${p}build.log\n" unless -x $target;

  runAndCheck( $test );

  say( "OK\n" );
  clean();
}

#
# Run one test case:
#  - Generate source file(s)
#  - Optionally compile
#  - Compare outputs to reference file
#
sub testCase($) {
  my $test = $_[0];
  my ($name, $compile, $output, $args) = ($test->{'name'}, $test->{'compile'}, $test->{'output'}, $test->{'args'});
  clean();
  say("$name: ");

  generate( $test );

  if ( $compile && "$compile" ne "" ) {
    comparePlPy();
    compileRunner( $test );
  }
  elsif ( $output && "$output" ne "" ) {
    comparePlPy();
    compileRunner( $test );
    runAndCheck( $test );
  }
  else {
    checkNoSources();
  }

  say "OK\n";
  clean();
}

#
# Generate sources
#
sub generate($) {
  my $test = $_[0];
  my ($name, $output, $args) = ($test->{'name'}, $test->{'output'}, $test->{'args'});

  if ( !$noPython ) {
    say "python...";
    system( "python ../python/scripts/cxxtestgen -o ${pyCpp} $args > ${p}py.out 2>&1" );
  }
}

#
# Compare outputs
#
sub comparePlPy() {
  if ( !$noPython && !$noPerl ) {
    say "compare...";
    if ( !filesEqual( $plCpp, $pyCpp ) ) {
      die "Different source files generated (${plCpp} <> ${pyCpp})\n";
    }
  }
}

#
# Compile generated source file
#
sub compileRunner($) {
  my ($test) = @_;

  say "compile...";
  if ( $test->{'main'} ) {
    compileWithMain( $test->{'compile'} );
  } else {
    compile( $test->{'compile'} );
  }

  die "Error building runner, see ${p}build.log\n" unless -x $target;
}

#
# Run runner and verify the output
#
sub runAndCheck($) {
  my ($test) = @_;

  say "run...";
  if (my $run = $test->{'run'}) {
    system( "$run" );
  } else {
    system( "$target > ${p}px.pre 2>&1" );
  }
  appendErrorLevel();

  say "check...";
  my $output = $test->{'output'};
  fixFile( "${p}px.pre", "${p}px.out", );
  if ( !filesEqual( $output, "${p}px.out" ) ) {
    die "Generated source gives incorrect output ($output <> ${p}px.out)\n";
  }
}

#
# File file names in output file
#
sub fixFile($$) {
  my ($in, $out) = @_;
  open IN, "<$in" or die "Cannot open $in\n";
  open OUT, ">$out" or die "Cannot create $out\n";

  while ( my $line = <IN> ) {
    $line =~ s/^.*[\/\\]([A-Za-z_]*\.[hcpp]*.[0-9])/$1/;
    print OUT $line;
  }

  close OUT;
  close IN;
}

#
# Verify that no source files have been generated
#
sub checkNoSources() {
  if ( !$noPerl && -e $plCpp ) {
    die "Output file ${plCpp} should not have been generated\n";
  }
  if( !$noPython && -e $pyCpp ) {
    die "Output file ${pyCpp} should not have been generated\n";
  }
}

#
# Check if compiler can compile something
#
sub supported($) {
  my ($source) = @_;
  system( "${compiler} ${exe_option}${target} ${include_option}.. $source > ${p}build.log 2>&1" );
  return ( -x $target );
}

#
# A test case that verifies something compiles
#
sub testComp($) {
  my $test = $_[0];
  my ($name, $source) = ($test->{'name'}, $test->{'source'});
  clean();
  say("$name: ");

  if ( !supported( $source ) ) {
    die "Error (see ${p}build.log)\n";
  }

  say "OK\n";
  clean();
}


#
# Clean up the mess
#
sub clean() {
  unlink <${p}p[ly]*.cpp>, "$rootCpp", "${p}pl.out", "${p}py.out", "$target", "${p}px.pre", "${p}px.out", "${p}build.log", <*~>;
  checkNoSources();
}

#
# Make sure we scan the sample headers in the correct order
#
my $samples = join( " ", sort( <../sample/*.h> ) );
my $guiInputs = "../sample/gui/GreenYellowRed.h";

#
# Make sure we have a compiler
#
die "The compiler (\"$compiler\") doesn't seem to work (PATH problem?)\n"
  unless supported( 'anything.cpp' );

print "Your compiler supports CXXTEST_PARTIAL_TEMPLATE_SPECIALIZATION.\n"
  if supported( 'tpltpl.cpp' );

#
# Run all the test cases
#

#
# Tests for cxxtestgen
#

testRoot { 'name' => "Root/Part       ", 'output' => "parts.out" };
testCase { 'name' => "Root + Part     ", 'args' => "--error-printer --root --part $samples", 'output' => "error.out" };
testCase { 'name' => "Wildcard input  ", 'args' => '../sample/*.h', 'main' => 1, 'output' => "wildcard.out" };
testCase { 'name' => "Stdio printer   ", 'args' => "--runner=StdioPrinter $samples", 'output' => "error.out" };
testCase { 'name' => "Paren printer   ", 'args' => "--runner=ParenPrinter $samples", 'output' => "paren.out" };
testCase { 'name' => "Yes/No runner   ", 'args' => "--runner=YesNoRunner $samples", 'output' => "runner.out" };
testCase { 'name' => "No static init  ", 'args' => "--error-printer --no-static-init $samples", 'output' => "error.out" };
testCase { 'name' => "Have Std        ", 'args' => "--runner=StdioPrinter --have-std HaveStd.h", 'output' => "std.out" };
testCase { 'name' => "Comments        ", 'args' => "--error-printer Comments.h", 'output' => "comments.out" };
testCase { 'name' => "Long long       ", 'args' => "--error-printer --longlong= LongLong.h", 'output' => "longlong.out" }
  if supported('longlong.cpp');
testCase { 'name' => "Int64           ", 'args' => "--error-printer --longlong=__int64 Int64.h", 'output' => "int64.out" }
  if supported('int64.cpp');
testCase { 'name' => "Include         ",
	     'args' => "--include=VoidTraits.h --include=LongTraits.h --error-printer IncludeTest.h",
	       'output' => "include.out" };

#
# Template file tests
#
testCase { 'name' => "Preamble        ", 'args' => "--template=preamble.tpl $samples", 'output' => "preamble.out" };
testCase { 'name' => "Activate all    ", 'args' => "--template=activate.tpl $samples", 'output' => "error.out" };
testCase { 'name' => "Only Suite      ", 'args' => "--template=../sample/only.tpl $samples",
	     'run' => "$target SimpleTest > ${p}px.pre 2>&1", 'output' => "suite.out" };
testCase { 'name' => "Only Test       ", 'args' => "--template=../sample/only.tpl $samples",
	     'run' => "$target SimpleTest testAddition > ${p}px.pre 2>&1", 'output' => "suite_test.out" };

#
# Test cases which do not require exception handling
#
testCase { 'name' => "No errors       ", 'args' => "--error-printer GoodSuite.h", 'output' => "good.out" };
testCase { 'name' => "Max dump size   ", 'args' => "--error-printer --include=MaxDump.h DynamicMax.h SameData.h",
	     'output' => 'max.out' };
testCase { 'name' => "Wide char       ", 'args' => "--error-printer WideCharTest.h", 'output' => "wchar.out" }
  if supported( 'wchar.cpp' );
testCase { 'name' => "Factor          ", 'args' => "--error-printer --factor Factor.h", 'output' => "factor.out" };
testCase { 'name' => "User traits     ", 'args' => "--template=UserTraits.tpl UserTraits.h",
	     'output' => 'user.out' };

my $normals = "LessThanEquals.h Relation.h DefaultTraits.h DoubleCall.h SameData.h Tsm.h TraitsTest.h MockTest.h SameZero.h";
testCase { 'name' => "Normal Behavior ", 'args' => "--error-printer $normals", 'output' => "normal.out" };
testCase { 'name' => "Normal + Abort  ", 'args' => "--error-printer --have-eh --abort-on-fail $normals", 'output' => "abort.out" };
testCase { 'name' => "STL Traits      ", 'args' => "--error-printer StlTraits.h", 'output' => "stl.out" }
  if supported( 'stpltpl.cpp' );

#
# Test cases which do require exception handling
#
testCase { 'name' => "Throw w/o Std   ", 'args' => "--template=ThrowNoStd.tpl ThrowNoStd.h",
	     'output' => 'throw.out' };

my $ehNormals = "Exceptions.h DynamicAbort.h DeepAbort.h ThrowsAssert.h";
testCase { 'name' => "Exceptions      ", 'args' => "--error-printer --abort-on-fail --have-eh $ehNormals",
	     'output' => "eh_normals.out" };
testCase { 'name' => "Default abort   ", 'args' => "--error-printer --include=DefaultAbort.h $ehNormals",
	     'output' => "default_abort.out" };
testCase { 'name' => "Default no abort", 'args' => "--error-printer $ehNormals",
	     'output' => "default_abort.out" };

#
# Global Fixtures
#
testCase { 'name' => "Global fixtures ", 'args' => "--error-printer GlobalFixtures.h WorldFixtures.h",
	     'output' => "gfxs.out" };
testCase { 'name' => "GF:SUW fails    ", 'args' => "--error-printer SetUpWorldFails.h",
	     'output' => "suwf.out" };
testCase { 'name' => "GF:SUW throws   ", 'args' => "--error-printer SetUpWorldThrows.h",
	     'output' => "suwt.out" };
testCase { 'name' => "GF:SU fails     ", 'args' => "--error-printer GfSetUpFails.h",
	     'output' => "gfsuf.out" };
testCase { 'name' => "GF:SU throws    ", 'args' => "--error-printer GfSetUpThrows.h",
	     'output' => "gfsut.out" };
testCase { 'name' => "GF:TD fails     ", 'args' => "--error-printer GfTearDownFails.h",
	     'output' => "gftdf.out" };
testCase { 'name' => "GF:TD throws    ", 'args' => "--error-printer GfTearDownThrows.h",
	     'output' => "gftdt.out" };
testCase { 'name' => "GF:TDW fails    ", 'args' => "--error-printer TearDownWorldFails.h",
	     'output' => "tdwf.out" };
testCase { 'name' => "GF:TDW throws   ", 'args' => "--error-printer TearDownWorldThrows.h",
	     'output' => "tdwt.out" };

#
# GUI
#
testCase { 'name' => "GUI             ", 'args' => "--gui=DummyGui $guiInputs",
	     'output' => "gui.out" };
testCase { 'name' => "GUI + runner    ", 'args' => "--gui=DummyGui --runner=ParenPrinter $guiInputs", 
	     'output' => "gui_paren.out" };
testCase { 'name' => "QT GUI          ", 'args' => "--gui=QtGui GoodSuite.h", 'compile' => "$qtFlags" };
testCase { 'name' => "Win32 GUI       ", 'args' => "--gui=Win32Gui GoodSuite.h", 'compile' => "$w32Flags" };
testCase { 'name' => "Win32 Unicode   ", 'args' => "--gui=Win32Gui GoodSuite.h",
	     'compile' => "$w32Flags -DUNICODE" }
  if supported( 'wchar.cpp' );
testCase { 'name' => "X11 GUI         ", 'args' => "--gui=X11Gui GoodSuite.h", 'compile' => "$x11Flags" };

#
# Tests for when the compiler doesn't support exceptions
#
if ( defined($no_eh_option) ) {
  testCase { 'name' => "No exceptions   ", 'args' => "--runner=StdioPrinter NoEh.h", 'output' => "no_eh.out",
	       'compile' => "$no_eh_option" };
  testCase { 'name' => "Force no EH     ", 'args' => "--runner=StdioPrinter --no-eh ForceNoEh.h", 'output' => "no_eh.out",
	       'compile' => "$no_eh_option" };
}

#
# Invalid input to cxxtestgen
#
testCase { 'name' => "Part + No Static", 'args' => "--part --no-static  $samples" };
testCase { 'name' => "Missing destroy ", 'args' => "BadTest.h" };
testCase { 'name' => "No tests        ", 'args' => "EmptySuite.h" };
testCase { 'name' => "Missing input   ", 'args' => "NoSuchFile.h" };
testCase { 'name' => "Missing template", 'args' => "--template=NoSuchFile.h $samples" };

#
# Local Variables:
# compile-command: "perl test.pl"
# End:
#
