#!/usr/bin/perl
#
# build.pl
# Copyright 2016, Cypress Semiconductor Corp. All rights reserved.
#
# This script performs a cross-platform build for the cyelftool
# Test can be added with the [-t] option
# 
# Developer note: a common problem is when the path to this script or test
#                 has spaces somewhere in it. Always quote pathways in the script
#################################################################################

use strict; 
use warnings;
use Term::ANSIColor qw(:constants); # for colored output
use File::Copy;
use File::Path 'rmtree';
use Archive::Tar;
use Cwd;
use sigtrap qw/handler signal_handler normal-signals/;

# Globals
my $tar        = Archive::Tar->new;
my $dt         = localtime;
my $success    = 1;
my $os         = $^O;   # MSWin32, linux, or darwin
my $base_dir   = cwd();
my $elftool_ver= "1.0.0.$dt";
my $libelf_ver = '0.8.13';
my $libelf_dir;
my $libelf_tgz;
my $libelf_tar;
my $os_arch;
my $build_dir;
my $build_log;
my $perform_tests;
my $LOGGER;

# when CTRL-C or other INT is trapped
sub signal_handler
{
  rmtree("libelf-$libelf_ver");
  die "\nbuild process killed\n";
}

#Run main, and catch any exceptions thrown (via die) in the code.
eval
{
    main(@ARGV);
};

#Report the exceptions message.
warn $@ if $@;

#Return 0 if there were no errors (like die) and the build succeeded
if (!$@ && $success)
{
    exit 0;
}
exit 1;

# accepts err msg as argument, prints err and help
sub usage
{
  print @_;
  print <<"END";
usage:
  [-h]              :  display this message
  [-o <output_dir>] :  specify an output directory (defaults to "output_<unix/win>")
  [-t]              :  perform test for as well as build for cyelftool (and cymcuelftool when it is stable)
  [-v]              :  set explicit version string
END
}


sub main
{
#####################################################################
# parse args

    while( my $arg = shift )
    {
      if( $arg eq "-h" ) { usage(); exit 0; }
      elsif( $arg eq "-o" ) {$build_dir = shift or (usage("-o expects one argument, none given\n") and exit 1);}
      elsif( $arg eq "-v" ) {$elftool_ver = shift or (usage("-v expects one argument, none given\n") and exit 1);}
      elsif( $arg eq "-t" ) {$perform_tests = 1;}
      else { usage("invalid argument: $arg\n"); exit 1; }
    }
    (defined $build_dir) or $build_dir = "output_$os";


#####################################################################
# set up build dir and shared variables

    (-d $build_dir) or mkdir($build_dir) or die "could not create build directory: $?\n";
    chdir( $build_dir );
    chomp( $build_dir = cwd() ); # make path absolute
    chdir( $base_dir );
    ( $build_dir ne $base_dir ) or die "Cannot build cyelftool here! You must specify an out-of-source build directory\n";

    $build_log = "$build_dir/build.log";
    unlink $build_log;
    open $LOGGER, ">>$build_log" or die;

    runCommand("echo cyelftool $os build started $dt", $LOGGER);
    runCommand("echo ---------------------------------------------", $LOGGER);

####################################################################
# build Windows
    my @args = @_;
    $libelf_dir = "$build_dir/libelf_win32/lib";
    $libelf_tar = 'libelf-0.8.13_WIN32_x86.tar';

    #      ________________
    #_____/ extract libelf \____________________________#
    info("\nextracting libelf package...\n", $LOGGER);
    $tar->read($libelf_tar) or die "could not find libelf tar file\n";

    if ( ! -d "$libelf_dir")
    {
      my @files = $tar->list_files();
      $tar->extract(@files);

      # confirm extraction was successful
      foreach my $file (@files)
      {
        (-e $file) or die "could not extract $file\n";
      }
      runCommand("mv libelf_win32 \"$build_dir\"", $LOGGER);
    }
    info("done\n", $LOGGER);

    #      _______
    #_____/ Build \_____________________________________#
    info("\nrunning CMake...\n", $LOGGER);
    chdir $build_dir;
    runCommand("cmake -DELFTOOL_VER=\"${elftool_ver}\" -G\"Visual Studio 12\" \"$base_dir\"", $LOGGER);

    runCommand("call \"%VS120COMNTOOLS%\\vsvars32.bat\"", $LOGGER);
    runCommand("\"C:\\Program Files (x86)\\MSBuild\\12.0\\Bin\\MSBuild\" CyElftool.sln /p:Configuration=Release", $LOGGER);

    #      ___________________
    #_____/ test if requested \___________________#
    if( defined $perform_tests )
    {
      # require commandline tools
      runCommand("which openssl", $LOGGER);

      chdir( "$base_dir\\test\\cyelftool" );
      runCommand("perl ..\\tests.pl -c cyelftool -d \"$build_dir\\cyelftool\\Release\"", $LOGGER);
    }
    chdir $base_dir;
  }


#Run a Windows shell command, printing the output to both standard output and a log file
sub runCommand
{
  my ($command, $LOG) = @_;

  open my $CMD, "$command |" or die;
  
  while (<$CMD>)
  {
    info($_,$LOG);
  }
  
  close $CMD; 
  my $exitcode = $? >> 8;
  if ($exitcode != 0)
  {
    error("command \"$command\" failed with exit code $exitcode\n",$LOG);
    $success = 0;
    close $LOG;
    die;
  }
}

sub info
{
    my ($message, $FILE) = @_;

    print "[INFO] ".$message;
    print $FILE "[INFO] ".$message;
}

sub warn
{
    my ($message, $FILE) = @_;

    print "[WARNING] ".$message;
    print $FILE "[WARNING] ".$message;
}

sub error
{
    my ($message, $FILE) = @_;

    print "[ERROR] ".$message;
    print $FILE "[ERROR] ".$message;

    die;
}
