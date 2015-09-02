#! e:/program files/perl/bin/perl.exe
#  version info can be found in 'freetype.h'

require "../local-paths.lib";

$major = 2;
$minor = 6;
$micro = 0;
$binary_age = 600;
$interface_age = 0;
$current_minus_age = 2;

sub process_file
{
        my $outfilename = shift;
	my $infilename = $outfilename . ".in";
	
	open (INPUT, "< $infilename") || exit 1;
	open (OUTPUT, "> $outfilename") || exit 1;
	
	while (<INPUT>) {
	    s/\@FREETYPE_MAJOR\@/$major/g;
	    s/\@FREETYPE_MINOR\@/$minor/g;
	    s/\@FREETYPE_PATCH\@/$micro/g;
	    s/\@FREETYPE_INTERFACE_AGE\@/$interface_age/g;
	    s/\@FREETYPE_BINARY_AGE\@/$binary_age/g;
	    s/\@LT_CURRENT_MINUS_AGE@/$current_minus_age/g;
	    s/\@PERL_PATH@/$perl_path/g;
	    s/\@GlibBuildRootFolder@/$glib_build_root_folder/g;
	    s/\@GenericIncludeFolder@/$generic_include_folder/g;
	    s/\@GenericLibraryFolder@/$generic_library_folder/g;
	    s/\@GenericWin32LibraryFolder@/$generic_win32_library_folder/g;
	    s/\@GenericWin32BinaryFolder@/$generic_win32_binary_folder/g;
	    s/\@Debug32TestSuiteFolder@/$debug32_testsuite_folder/g;
	    s/\@Release32TestSuiteFolder@/$release32_testsuite_folder/g;
	    s/\@Debug32TargetFolder@/$debug32_target_folder/g;
	    s/\@Release32TargetFolder@/$release32_target_folder/g;
	    s/\@TargetSxSFolder@/$target_sxs_folder/g;
	    print OUTPUT;
	}
}

my $command=join(' ',@ARGV);
if ($command eq -buildall) {
	process_file ("builds/win32/msvc/freetype.vsprops");
}