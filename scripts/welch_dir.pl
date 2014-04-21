#!/usr/bin/perl
use strict;
use warnings;

my $N_args = $#ARGV + 1;

if( $N_args != 2 ) {
	print "Usage: welch_dir.pl directory_name max_freq\n";
	exit;
}

my $dir = $ARGV[0];
my $max_freq = $ARGV[1];

opendir DIR, $dir or die "Cannot open directory $dir: $!";
my @files = readdir DIR;
closedir DIR;

for my $file (@files) {
	if($file =~ m/.dat$/) {
		my $infile = my $outfile = $file;
		$outfile =~ s/.dat$/.pdf/;
		my $syscall = "Rscript welch.R $dir$infile $dir$outfile $max_freq";
		print $syscall."\n";
		my $status = system($syscall);
		if(($status >>= 8) != 0) {
			die "Failed to run: $syscall\n";
		}
	}
}




