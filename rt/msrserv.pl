#!/usr/bin/perl -w

# Multithreaded Server 
# according to the example from "Programming Perl"
#
# works with read/write on a device-file  
#
# $Revision: 1.1 $
# $Date: 2002/07/09 10:10:59 $
# $RCSfile: msrserv.pl,v $
#

require 5.002;
use strict;
BEGIN { $ENV{PATH} = '/usr/bin:/bin' }
use Socket;
use Carp;
use FileHandle;
use Getopt::Std; 

use Sys::Syslog qw(:DEFAULT setlogsock); 

use vars qw (
	     $self $pid $dolog $port $dev %opts $selfbase
	     $len $offset $stream $written $read $log $blksize
	     $authfile %authhosts
	     );


# Do logging to local syslogd by unix-domain socket instead of inetd
setlogsock("unix");  

# Prototypes and some little Tools
sub spawn;
sub logmsg { 
  my ($level, @text) = @_;
  syslog("daemon|$level", @text) if $dolog;
#  print STDERR "daemon|$level", @text, "\n" if $dolog;
}
sub out {
  my $waitpid = wait; 
  logmsg("notice", "$waitpid exited");
  unlink "$selfbase.pid";
  exit 0;
}

sub help {
  print "\n  usage: $0 [-l og] [-h elp] [-p port] [-d device]\n"; 
  exit; 
}

# Process Options
%opts = (
	 "l" => 1,
	 "h" => 0,
	 "p" => 2345,
	 "d" => "/dev/msr"
	 );
  
getopts("lhp:d:", \%opts);

help if $opts{"h"};

( $self =  $0 ) =~ s+.*/++ ;
( $selfbase = $self ) =~ s/\..*//;
$log = "$selfbase.log";
$dolog = $opts{"l"};
$port = $opts{"p"};
$dev = $opts{"d"};
$blksize = 1024; # try to write as much bytes
$authfile = "/opt/kbw/etc/hosts.auth"; 

# Start logging
openlog($self, 'pid');

# Flush Output, dont buffer
$| = 1;

# first fork and run in background
if ($pid = fork) {
#  open LOG, ">$log" if $dolog;
#  close LOG;
  logmsg("notice", "forked process: $pid\n");
  exit 0;
}

# Server tells about startup success
open (PID, ">$selfbase.pid");
print PID "$$\n";
close PID;

# Cleanup on exit (due to kill -TERM signal)
$SIG{TERM} = \&out;

# We use streams
my $proto = getprotobyname('tcp');

# Open Server socket
socket(Server, PF_INET, SOCK_STREAM, $proto) or die "socket: $!";
setsockopt(Server, SOL_SOCKET, SO_REUSEADDR, pack("l", 1))
  or die "setsocketopt: $!";
bind (Server, sockaddr_in($port, INADDR_ANY))
  or die "bind: $!";
listen (Server, SOMAXCONN) 
  or die "listen: $!";

%authhosts = ();
# get authorized hosts
open (AUTH, $authfile) 
  or logmsg ("notice", "Could not read allowed hosts file: $authfile");
while (<AUTH>) {
    chomp;
    my $host = lc $_;
    logmsg ("notice", "Authorized host: $host");
    $authhosts{$_} = 1 if $host =~ /^[\d\w]/;
}
close (AUTH);

# tell about open server socket
logmsg ("notice", "Server started at port $port");

my $waitpid = 0;
my $paddr;

# wait for children to return, thus avoiding zombies
sub REAPER {
  $waitpid = wait;
  $SIG{CHLD} = \&REAPER; 
  logmsg ("notice", "reaped $waitpid", ($? ? " with exit $?" : ""));
}

# also all sub-processes should wait for their children
$SIG{CHLD} = \&REAPER;

# start a new server for every incoming request
for ( ; $paddr = accept(Client, Server); close (Client)) {
  my ($port, $iaddr) = sockaddr_in($paddr);
  my $name = lc gethostbyaddr($iaddr, AF_INET);
  my $ipaddr = inet_ntoa($iaddr);
  my $n = 0;

# tell about the requesting client
  logmsg ("info", "Connection from $ipaddr ($name) at port $port");

  spawn sub {
    my ($head, $hlen, $pos, $pegel, $typ, $siz, $nch, $nrec, $dat, $i, $j, $n, $llen); 
    my ($watchpegel, $shmpegel);
    my ($rin, $rout, $in, $line, $data_requested, $oversample);
    my (@channels);
    
#   to use stdio on writing to Client
    Client->autoflush();

#   Open Device 
    sysopen (DEV, "$dev", O_RDWR | O_NONBLOCK, 0666) or die("can't open $dev");

#   Bitmask to check for input on stdin
    $rin = "";
    vec($rin, fileno(Client), 1) = 1; 

#   check for authorized hosts
    my $access = 'allow';
    $access = 'allow' if $authhosts{$ipaddr};
    $line = "<remote_host host=\"$ipaddr\" access=\"$access\">\n";
    $len = length $line;
    $offset = 0;
    while ($len) {
	$written = syswrite (DEV, $line, $len, $offset);
	$len -= $written;
	$offset += $written;
    }

    while ( 1 ) {
      $in = select ($rout=$rin, undef, undef, 0.0); # poll client
#     look for any Input from Client
      if ($in) {
#       exit on EOF
	$len = sysread (Client, $line, $blksize) or exit;
	logmsg("info", "got $len bytes: \"$line\""); 
	$offset = 0;
#       copy request to device
	while ($len) {
	  $written = syswrite (DEV, $line, $len, $offset);
	  $len -= $written;
	  $offset += $written;
	}
      }
#     look for some output from device
      if ($len = sysread DEV, $stream, $blksize) {
	print Client $stream;
      } else {
	select undef, undef, undef, 0.1; # calm down if nothing on device
      }
    }
  }
}

sub spawn {
  my $coderef = shift;

  unless (@_ == 0 && $coderef && ref($coderef) eq 'CODE') {
    confess "usage: spawn CODEREF";
  }
  my $pid; 
  if (!defined($pid = fork)) {
    logmsg ("notice", "fork failed: $!");
    return;
  } elsif ($pid) {
    logmsg ("notice", "Request $pid");
    return; # Parent
  }

# do not use fdup as in the original example
# open (STDIN, "<&Client") or die "Can't dup client to stdin";
# open (STDOUT, ">&Client") or die "Can't dup client to stdout";
# STDOUT->autoflush();
  exit &$coderef();
}












