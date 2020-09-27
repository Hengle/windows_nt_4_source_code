/*
 * MkClnMkf - generates the makefile to cleanse the VC++ v4.0 CRTL sources
 *	in preparation for a Microsoft-internal build of the CRTL.  Those
 *	cleansed sources will also be used for the publicly-released CRTL
 *	source as a part of the VC++ 4.0 retail product.
 *
 * Programmed by Steve Salisbury, 1994-12-07 Wed
 *
 * This program takes 7 arguments:
 *	prefixSrc - path to the raw VC++ 4.0 CRTL sources
 *	prefixDst - path to the partly cleansed VC++ 4.0 CRTL sources
 *	prefixDst2 - path to the final cleansed VC++ 4.0 CRTL sources
 *	inputRaw - a list of raw input files
 *	inputCln - a list of the input files that must be cleansed
 *	output - name of the first output file (may not already exist)
 *	outputRel - name of the second output file (may not already exist)
 *
 * Modified by Steve Salisbury, 1994-12-15 Thu
 *	Restore the crtw32/ and fpw32/ subdirectories
 *	Automate the generation of the directory list
 *
 * Modified 1995-01-16 Mon - take directory list from an input file, too.
 *
 * Modified 1995-01-17 Tue - many changes to make things work when removing
 *	crtw32/ and fpw32/ directories from the target filename paths.
 *
 * Modified 1995-01-18 Wed - The 3 files cleansed with bldmkf.sed should
 *	have that file as an explicit dependency.
 *
 * Modified 1995-01-23 Mon - Add a second output file
 */


/*-
 * Build Version Options
-*/

/* define KEEPDIRS */


/*-
 * Include Header Files
-*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>


/*-
 * Define Constants
-*/

#define MAXARGLEN	128

#define	CRTW32	"crtw32\\"

#define	FPW32	"fpw32\\"


/*-
 * Define VERBOSE to get verbose output makefiles
-*/

#ifdef VERBOSE
#define	AT	""
#else
#define	AT	"@"
#endif


#if	defined(_M_IX86)
#define TOOLDIR "..\\libw32\\tools\\i386"
#define CPUDIR	"intel"
#elif	defined(_M_ALPHA)
#define TOOLDIR "..\\libw32\\tools\\alpha"
#define CPUDIR	"alpha"
#elif	defined(_M_MRX000)
#define TOOLDIR "..\\libw32\\tools\\mips"
#define CPUDIR	"mips"
#elif	defined(_M_PPC)
#define TOOLDIR "..\\libw32\\tools\\ppc"
#define CPUDIR	"ppc"
#endif

#define MPPC_DIR	"mppc"

/*-
 * Define Global Variables (Constants)
-*/

#ifndef KEEPDIRS
char fmtRaw2 [ ] =
	"%s%s: %s%s %ssrcrel\\bldmkf.sed\n"
	"\t"	AT "echo Copying %s from %s to %s. (using SED.EXE)\n"
	"\t"	AT "if exist %s%s attrib -r %s%s\n"
	"\t"	AT "sed -f %ssrcrel\\bldmkf.sed %s%s > %s%s\n"
	"\t"	AT "attrib +r %s%s\n"
	"\n" ;
#endif /* KEEPDIRS */

char fmtRaw [ ] =
	"%s%s: %s%s\n"
	"\t"	AT "echo Copying %s from %s to %s.\n"
	"\t"	AT "if exist %s%s attrib -r %s%s\n"
	"\t"	AT "copy /b %s%s %s%s >nul\n"
	"\t"	AT "attrib +r %s%s\n"
	"\n" ;

char fmtCln2 [ ] =
	"%s%s: %s%s %ssrcrel\\msvc40.if\n"
	"\t"	AT "echo Cleansing %s%s\n"
	"\t"	AT "if exist %s%s attrib -r %s%s\n"
	"\t"	AT TOOLDIR "\\detab < %s%s | " TOOLDIR "\\trailing > %s%s\n"
	"\t"	AT "rem *\n"
	"\t"	AT "rem * IFSTRIP is NOT used on .INC assembler include files!\n"
	"\t"	AT "rem *\n"
	"\t"	AT "rem " TOOLDIR "\\ifstrip -z -c -t -x.TMP -f %ssrcrel\\msvc40.if %s%s\n"
	"\t"	AT "rem del %s%s\n"
	"\t"	AT "rem ren %s%s.TMP *%s\n"
	"\t"	AT TOOLDIR "\\striphdr -r %s%s\n"
	"\t"	AT "del %s%s\n"
	"\t"	AT "ren %s%s.NEW *%s\n"
	"\t"	AT "attrib +r %s%s\n"
	"\n" ;

char fmtCln [ ] =
	"%s%s: %s%s %ssrcrel\\msvc40.if\n"
	"\t"	AT "echo Cleansing %s%s\n"
	"\t"	AT "if exist %s%s attrib -r %s%s\n"
	"\t"	AT TOOLDIR "\\detab < %s%s | " TOOLDIR "\\trailing > %s%s\n"
	"\t"	AT TOOLDIR "\\ifstrip -z -c -t -x.TMP -f %ssrcrel\\msvc40.if %s%s\n"
	"\t"	AT "del %s%s\n"
	"\t"	AT "ren %s%s.TMP *%s\n"
	"\t"	AT TOOLDIR "\\striphdr -r %s%s\n"
	"\t"	AT "del %s%s\n"
	"\t"	AT "ren %s%s.NEW *%s\n"
	"\t"	AT "attrib +r %s%s\n"
	"\n" ;

const char prefixSrc [ ] = "$(SRC)\\" ;

const char prefixDst [ ] = "$(DST)\\" ;

char * * DirList ;


/*-
 * Function Declarations (Prototypes)
-*/

int main ( int argc , char * * argv ) ;

void Usage ( void ) ;

char * fgetsNL ( char * lineBuf , int lineSize , FILE * fileInput ) ;

void Progress ( int prefix , char * string , int suffix ) ;

char * SkipFirstDir ( char * string ) ;


/*-
 * Function Definitions (Implementations)
-*/

int main ( int argc , char * * argv )
{
	FILE * inputRaw ;
	FILE * inputCln ;
	FILE * inputDir ;
	FILE * output ;
	FILE * output2 ;
	char lineRaw [ MAXARGLEN ] ;
	char lineCln [ MAXARGLEN ] ;
	char * rvRaw ;
	char * rvCln ;
	char * defaultSrc ;
	char * defaultDst ;
	char * defaultDst2 ;
	char * lineSkip ;
	char * * pDirs ;
	time_t timestamp ;
	struct tm localt ;
	char * * nextArg ;

	nextArg = ++ argv ;

	if ( -- argc != 8 )
	{
		Usage ( ) ;
		exit ( 1 ) ;
	}

	defaultSrc = * nextArg ;
	++ nextArg ;
	defaultDst = * nextArg ;
	++ nextArg ;
	defaultDst2 = * nextArg ;
	++ nextArg ;
	
	if ( ( inputRaw = fopen ( * nextArg , "rt" ) ) == NULL )
	{
		fprintf ( stderr , "mkclnmkf: cannot open input file \"%s\"\n" ,
			* nextArg ) ;
		exit ( 1 ) ;
	}
	++ nextArg ;

	if ( ( inputCln = fopen ( * nextArg , "rt" ) ) == NULL )
	{
		fprintf ( stderr , "mkclnmkf: cannot open input file \"%s\"\n" ,
			* nextArg ) ;
		exit ( 1 ) ;
	}
	++ nextArg ;

	if ( ( inputDir = fopen ( * nextArg , "rt" ) ) == NULL )
	{
		fprintf ( stderr , "mkclnmkf: cannot open input file \"%s\"\n" ,
			* nextArg ) ;
		exit ( 1 ) ;
	}
	++ nextArg ;

	if ( ( output = fopen ( * nextArg , "rb" ) ) != NULL )
	{
		fprintf ( stderr , "mkclnmkf: first output file already exists: \"%s\"\n" ,
			* nextArg ) ;
		exit ( 1 ) ;
	}

	if ( ( output = fopen ( * nextArg , "wt" ) ) == NULL )
	{
		fprintf ( stderr , "mkclnmkf: cannot open first output file: \"%s\"\n" ,
			* nextArg ) ;
		exit ( 1 ) ;
	}
	++ nextArg ;

	if ( ( output2 = fopen ( * nextArg , "rb" ) ) != NULL )
	{
		fprintf ( stderr , "mkclnmkf: second output file already exists: \"%s\"\n" ,
			* nextArg ) ;
		exit ( 1 ) ;
	}

	if ( ( output2 = fopen ( * nextArg , "wt" ) ) == NULL )
	{
		fprintf ( stderr , "mkclnmkf: cannot open second output file: \"%s\"\n" ,
			* nextArg ) ;
		exit ( 1 ) ;
	}
	++ nextArg ;

	/*-
	 * Read in the list of directories
	-*/

	{
		int numDirs = 0 ;
		int maxNumDirs = 0 ;
		char nextDir [ MAXARGLEN ] ;
		char * pDir ;

		pDir = fgetsNL ( nextDir , sizeof ( nextDir ) , inputDir ) ;

		while ( pDir )
		{
			if ( numDirs + 1 > maxNumDirs )
			{
				maxNumDirs += 16 ;
				if ( ! ( DirList = ( char * * ) realloc ( DirList ,
					sizeof ( char * ) * maxNumDirs ) ) )
				{
					fprintf ( stderr ,
						"mkclnmkf: realloc failed (maxNumDirs=%d)\n" ,
							maxNumDirs ) ;

					exit ( 1 ) ;
				}
			}

			DirList [ numDirs ] = _strdup ( nextDir ) ;

			if ( ! DirList [ numDirs ] )
			{
				fprintf ( stderr ,
					"mkclnmkf: strdup failed (nextDir=\"%s\")\n" ,
						nextDir ) ;

				exit ( 1 ) ;
			}

			++ numDirs ;

			pDir = fgetsNL ( nextDir , sizeof ( nextDir ) , inputDir ) ;
		}

		DirList [ numDirs ] = NULL ;

		{
			int i ;

			fprintf ( stderr , "Reading in directory list...\n" ) ;

			for ( i = 0 ; DirList [ i ] ; ++ i )
				Progress ( '(' , DirList [ i ] , ')' ) ;

			Progress ( '(' , NULL , ')' ) ;
		}
	}

	if ( fclose ( inputDir ) )
	{
		fprintf ( stderr ,
			"mkclnmkf: error closing input file \"%s\"\n" ,
			* nextArg ) ;
	}

	/*
	 * Generate MAKEFILE header comment
	 */

	time ( & timestamp ) ;

	localt = * localtime ( & timestamp ) ;

	fprintf ( output ,
	"########\n"
	"#\n"
	"# This is a program-generated Makefile, the purpose of which is to\n"
	"# copy Visual C++ v4.0 C Run-Time Library Source files from the raw\n"
	"# source tree to a directory structure suitable for end user usage\n"
	"# in debugging and modification.  Not all of the VC++ 4.0 CRTL srcs\n"
	"# are provided to end users, but the ones that are 'cleansed' using\n"
	"# an automated process.  Another makefile will copy these cleansed\n"
	"# files to their final directory lay-out.\n"
	"#\n"
	"# This Makefile was generated on\n"
	"#\t\t%.3s %2d %4d at %02d:%02d:%02d\n"
	"#\n"
	"# The program that generated this Makefile was compiled on\n"
	"#\t\t" __DATE__ " at " __TIME__ "\n"
	"\n"
	"!if \"$(SRC)\"==\"\"\n"
	"SRC=%s\n"
	"!endif\n"
	"\n"
	"!if \"$(DST)\"==\"\"\n"
	"DST=%s\n"
	"!endif\n"
	"\n"
	"all:\tdirs files\n"
	"\n"
	"files: \\\n" ,
		localt . tm_mon * 3 + "JanFebMarAprMayJunJulAugSepOctNovDec" ,
		localt . tm_mday , localt . tm_year + 1900 ,
		localt . tm_hour , localt . tm_min , localt . tm_sec ,
		defaultSrc , defaultDst ) ;


	fprintf ( output2 ,
	"########\n"
	"#\n"
	"# This is a program-generated Makefile, the purpose of which is to\n"
	"# copy the cleansed Visual C++ v4.0 C Run-Time Library Source files\n"
	"# to their directory lay-out which will be received by end users.\n"
	"#\n"
	"# This Makefile was generated on\n"
	"#\t\t%.3s %2d %4d at %02d:%02d:%02d\n"
	"#\n"
	"# The program that generated this Makefile was compiled on\n"
	"#\t\t" __DATE__ " at " __TIME__ "\n"
	"\n"
	"!if \"$(SRC)\"==\"\"\n"
	"SRC=%s\n"
	"!endif\n"
	"\n"
	"!if \"$(DST)\"==\"\"\n"
	"DST=%s\n"
	"!endif\n"
	"\n"
	"all:\tdirs files\n"
	"\n"
	"dirs:\n"
	"\t" AT "if not exist %s" CPUDIR "\\NUL mkdire %s" CPUDIR "\n"
#if	defined(_M_IX86)
	"\t" AT "if not exist %s" CPUDIR "\\win32s\\NUL mkdire %s" CPUDIR "\\win32s\n"
	"\t" AT "if not exist %s" MPPC_DIR "\\NUL mkdire %s" MPPC_DIR "\n"
#endif
	"\t" AT "if not exist %ssys\\NUL mkdire %ssys\n"
	"\t" AT "if not exist %sbuild\\NUL mkdire %sbuild\n"
	"\t" AT "if not exist %sbuild\\" CPUDIR "\\NUL mkdire %sbuild\\" CPUDIR "\n"
	"\t" AT "if not exist %sbuild\\" CPUDIR "\\puma\\NUL mkdire %sbuild\\" CPUDIR "\\puma\n"
#if	defined(_M_IX86)
	"\t" AT "if not exist %sbuild\\" CPUDIR "\\win32s\\NUL mkdire %sbuild\\" CPUDIR "\\win32s\n"
	"\t" AT "if not exist %sbuild\\" MPPC_DIR "\\NUL mkdire %sbuild\\" MPPC_DIR "\n"
	"\t" AT "if not exist %sbuild\\" MPPC_DIR "\\puma\\NUL mkdire %sbuild\\" MPPC_DIR "\\puma\n"
#endif
	"\n"
	"files: \\\n"
	"\t%smsvcrt40.rc \\\n"
	"\t%smsvcrt.src \\\n"
	"\t%smakefile \\\n"
	"\t%smakefile.inc \\\n"
	"\t%smakefile.sub \\\n" ,
		localt . tm_mon * 3 + "JanFebMarAprMayJunJulAugSepOctNovDec" ,
		localt . tm_mday , localt . tm_year + 1900 ,
		localt . tm_hour , localt . tm_min , localt . tm_sec ,
		defaultDst , defaultDst2 ,
		/* if not exist ... CPUDIR */
		prefixDst , prefixDst ,
#if	defined(_M_IX86)
		/* if not exist ... CPUDIR\win32s */
		prefixDst , prefixDst ,
		/* if not exist mppc */
		prefixDst , prefixDst ,
#endif
		/* if not exist sys */
		prefixDst , prefixDst ,
		/* if not exist build ... */
		prefixDst , prefixDst ,
		prefixDst , prefixDst ,
		prefixDst , prefixDst ,
#if	defined(_M_IX86)
		/* if not exist ... build\CPUDIR\win32s */
		prefixDst , prefixDst ,
		/* if not exist ... build\mppc */
		prefixDst , prefixDst ,
		prefixDst , prefixDst ,
#endif
		/* msvcrt40.rc makefile.* */
		prefixDst , prefixDst ,
		prefixDst , prefixDst , prefixDst ) ;
		/* msvc*.def */

	/*
	 * Generate default ("all") dependecy
	 */

	/*-
	 * First, files that are just copied
	-*/

	rvRaw = fgetsNL ( lineRaw , sizeof ( lineRaw ) , inputRaw ) ;

	while ( rvRaw )
	{
		lineSkip = lineRaw ;

#ifndef KEEPDIRS
		if ( ! strncmp ( "crtw32\\" , lineSkip , 7 ) )
			lineSkip += 7 ;
		else if ( ! strncmp ( "fpw32\\" , lineSkip , 6 ) )
			lineSkip += 6 ;
#endif /* KEEPDIRS */

		fprintf ( output , "\t%s%s \\\n" ,
			prefixDst , lineSkip ) ;

		rvRaw = fgetsNL ( lineRaw , sizeof ( lineRaw ) , inputRaw ) ;
	}
	
	/*-
	 * Second, files that have be cleansed
	-*/

	rvCln = fgetsNL ( lineCln , sizeof ( lineCln ) , inputCln ) ;

	while ( rvCln )
	{
		lineSkip = lineCln ;

#ifndef KEEPDIRS
		if ( ! strncmp ( "crtw32\\" , lineSkip , 7 ) )
			lineSkip += 7 ;
		else if ( ! strncmp ( "fpw32\\" , lineSkip , 6 ) )
			lineSkip += 6 ;
#endif /* KEEPDIRS */

		fprintf ( output , "\t%s%s \\\n" ,
			prefixDst , lineSkip ) ;

		fprintf ( output2 , "\t%s%s \\\n" ,
			prefixDst , SkipFirstDir ( lineSkip ) ) ;

		rvCln = fgetsNL ( lineCln , sizeof ( lineCln ) , inputCln ) ;
	}

	fprintf ( output ,
		"\n\n# Directory Dependencies:\n\ndirs:\n"
		"\t" AT "if not exist %sbuild\\NUL mkdire %sbuild\n" ,
			prefixDst , prefixDst ) ;

#ifdef BLDROOTDIR
	/*
	 * Create Root Directory Component by Component
	 *	NOTE: this code assumes prefixDst resembles:
	 *		\msdev\crt\src\
	 */

	{
		char temp [ MAXARGLEN ] ;
		char * prefixSlash ;
		char * prefixNext ;
		int len ;

		strcpy ( temp , prefixDst ) ;
		len = strlen ( temp ) ;

		if ( temp [ len - 1 ] == '\\' )
			temp [ len - 1 ] == '\0' ;

		prefixNext = prefixDst + 1 ;

		while ( prefixSlash = strchr ( prefixNext , '\\' ) )
		{
			temp [ prefixSlash - prefixDst ] = '\0' ;

			fprintf ( output ,
	"\t" AT "if not exist %s\\NUL mkdire %s\n" ,
				temp , temp ) ;

			prefixNext = prefixSlash + 1 ;
			temp [ prefixSlash - prefixDst ] = '\\' ;
		}
	}
#endif /* BLDROOTDIR */

	/*
	 * Create Directories
	 */

	for ( pDirs = DirList ; * pDirs ; ++ pDirs )
	{
		char * targetDir ;

		targetDir = * pDirs ;

#ifndef KEEPDIRS
		if ( ! strcmp ( "crtw32" , targetDir )
		  || ! strcmp ( "fpw32" , targetDir ) )
			continue ;

		if ( ! strncmp ( "crtw32\\" , targetDir , 7 ) )
			targetDir += 7 ;
		else if ( ! strncmp ( "fpw32\\" , targetDir , 6 ) )
			targetDir += 6 ;
#endif /* KEEPDIRS */

		fprintf ( output ,
	"\t" AT "if not exist %s%s\\NUL mkdire %s%s\n" ,
			prefixDst , targetDir ,
			prefixDst , targetDir ,
			prefixDst , targetDir ) ;
	}

	fprintf ( output , "\n\n#\n# Individual Dependencies:\n#\n\n" ) ;

	/*
	 * Second output makefile has some special targets
	 */

	fprintf ( output2 , "\n\n#\n# Individual Dependencies:\n#\n\n" ) ;
	
	{
		static char * listMkf [ ] =
		{
			"makefile" , "makefile.inc" , "makefile.sub" , NULL
		} ;
/*---
		static char * listDef [ ] =
		{
			"msvcrt40.def" , "msvcr40d.def" ,
			"win32s\\msvcrt40.def" ,  NULL
		} ;
---*/

		const char * rcfile = "msvcrt40.rc" ;
		const char * srcfile = "msvcrt.src" ;
		int i ;

		fprintf ( output2 ,
			"%s%s: %slibw32\\%s\n"
			"\t" AT "echo Copying %s from %s to %s.\n"
			"\t" AT "if exist %s%s attrib -r %s%s\n"
			"\t" AT "copy /b $** $@ >NUL\n"
			"\t" AT "attrib +r %s%s\n"
			"\n" ,
			/* %s%s: %slibw32\\%s */
			prefixDst , rcfile ,
			prefixSrc , rcfile ,
			/* echo Copying %s from %s to %s. */
			rcfile , prefixDst , prefixSrc ,
			/* if exist %s%s attrib -r %s%s */
			prefixDst , rcfile ,
			prefixDst , rcfile ,
			/* copy /b $** $@ >NUL */
			/* attrib +r %s%s */
			prefixDst , rcfile ) ;

		fprintf ( output2 ,
			"%s%s: %slibw32\\%s\n"
			"\t" AT "echo Copying %s from %s to %s.\n"
			"\t" AT "if exist %s%s attrib -r %s%s\n"
			"\t" AT "copy /b $** $@ >NUL\n"
			"\t" AT "attrib +r %s%s\n"
			"\n" ,
			/* %s%s: %slibw32\\%s */
			prefixDst , srcfile ,
			prefixSrc , srcfile ,
			/* echo Copying %s from %s to %s. */
			srcfile , prefixDst , prefixSrc ,
			/* if exist %s%s attrib -r %s%s */
			prefixDst , srcfile ,
			prefixDst , srcfile ,
			/* copy /b $** $@ >NUL */
			/* attrib +r %s%s */
			prefixDst , srcfile ) ;

		for ( i = 0 ; listMkf [ i ] ; ++ i )
			fprintf ( output2 ,
				"%s%s: %s%s relmkf.sed\n"
				"\t" AT "echo Copying %s from %s to %s. (using SED)\n"
				"\t" AT "if exist %s%s attrib -r %s%s\n"
				"\t" AT "sed -f relmkf.sed %s%s > $@\n"
				"\t" AT "attrib +r %s%s\n"
				"\n" ,
				/* %s%s: %slibw32\\%s */
				prefixDst , listMkf [ i ] ,
				prefixSrc , listMkf [ i ] ,
				/* echo Copying %s from %s to %s. */
				listMkf [ i ] , prefixSrc , prefixDst ,
				/* if exist %s%s attrib -r %s%s */
				prefixDst , listMkf [ i ] ,
				prefixDst , listMkf [ i ] ,
				/* sed -f relmkf.sed %s%s > $@ */
				prefixSrc , listMkf [ i ] ,
				/* attrib +r %s%s */
				prefixDst , listMkf [ i ] ) ;

/*---
		for ( i = 0 ; listDef [ i ] ; ++ i )
			fprintf ( output2 ,
				"%si386\\%s: %slibw32\\lib\\i386\\%s\n"
				"\t" AT "echo Copying %s from %s to %s.\n"
				"\t" AT "if exist %s%s attrib -r %s%s\n"
				"\t" AT "copy /b $** $@ >NUL\n"
				"\t" AT "attrib +r %s%s\n"
				"\n" ,
				/-* %s%s: %slibw32\\%s *-/
				prefixDst , listDef [ i ] ,
				prefixSrc , listDef [ i ] ,
				/-* echo Copying %s from %s to %s. *-/
				listDef [ i ] , prefixSrc , prefixDst ,
				/-* if exist %s%s attrib -r %s%s *-/
				prefixDst , listDef [ i ] ,
				prefixDst , listDef [ i ] ,
				/-* copy /b $** $@ >NUL *-/
				/-* attrib +r %s%s *-/
				prefixDst , listDef [ i ] ) ;
---*/
	}

	/*
	 * Reset input files for another pass: individual dependencies
	 */

	if ( fseek ( inputRaw , 0L , SEEK_SET )
	  || fseek ( inputCln , 0L , SEEK_SET ) )
	{
		fprintf ( stderr , "mkclnmkf: cannot reset input file(s)\n" ) ;
		exit ( 1 ) ;
	}


	/*
	 * Generate individual dependecies for files that are copied as-is
	 */

	fprintf ( stderr , "Producing copied file dependencies...\n" ) ;

	fprintf ( output , "# Copied File Dependencies:\n\n" ) ;

	rvRaw = fgetsNL ( lineRaw , sizeof ( lineRaw ) , inputRaw ) ;

	while ( rvRaw )
	{
		lineSkip = lineRaw ;

#ifndef KEEPDIRS
		/*
		 * strip crtw32\ and fpw32\ from the input string
		 */

		if ( ! strncmp ( "crtw32\\" , lineSkip , 7 ) )
			lineSkip += 7 ;
		else if ( ! strncmp ( "fpw32\\" , lineSkip , 6 ) )
			lineSkip += 6 ;

		if ( ( lineRaw [ 0 ] != 'f'
		  || strcmp ( lineSkip + strlen ( lineSkip ) - 9 , "\\lsources" ) )
		  && strcmp ( "makefile" , lineRaw ) )
#endif /* KEEPDIRS */

			fprintf ( output , fmtRaw ,
				/* %s%s: %s%s */
				prefixDst , lineSkip , prefixSrc , lineRaw ,
				/* echo Copying %s from %s to %s */
				lineSkip , prefixSrc , prefixDst ,
				/* if exist %s%s attrib -r %s%s */
				prefixDst , lineSkip , prefixDst , lineSkip ,
				/* copy /b %s%s %s%s */
				prefixSrc , lineRaw , prefixDst , lineSkip ,
				/* attrib +r %s%s */
				prefixDst , lineSkip ) ;
#ifndef KEEPDIRS
		else
			fprintf ( output , fmtRaw2 ,
				/* %s%s: %s%s %ssrcrel\\bldmkf.sed */
				prefixDst , lineSkip ,
				prefixSrc , lineRaw , prefixSrc ,
				/* echo Copying %s from %s to %s */
				lineSkip , prefixSrc , prefixDst ,
				/* if exist %s%s attrib -r %s%s */
				prefixDst , lineSkip , prefixDst , lineSkip ,
				/* sed -f %ssrcrel\\bldmkf.sed %s%s > %s%s */
				prefixSrc ,
				prefixSrc , lineRaw , prefixDst , lineSkip ,
				/* attrib +r %s%s */
				prefixDst , lineSkip ) ;
#endif /* KEEPDIRS */

		Progress ( '[' , lineRaw , ']' ) ;

		rvRaw = fgetsNL ( lineRaw , sizeof ( lineRaw ) , inputRaw ) ;
	}

	Progress ( '[' , NULL , ']' ) ;

	/*
	 * Generate individual dependecies for files that are cleansed
	 */

	fprintf ( stderr , "Producing cleansed file dependencies...\n" ) ;

	fprintf ( output , "# Cleansed File Dependencies:\n\n" ) ;

	rvCln = fgetsNL ( lineCln , sizeof ( lineCln ) , inputCln ) ;

	while ( rvCln )
	{
		char * pExt ;
		char pNoExt [ MAXARGLEN ] ;

		lineSkip = lineCln ;

#ifndef KEEPDIRS
		/*
		 * strip crtw32\ and fpw32\ from the input string
		 */

		if ( ! strncmp ( "crtw32\\" , lineSkip , 7 ) )
			lineSkip += 7 ;
		else if ( ! strncmp ( "fpw32\\" , lineSkip , 6 ) )
			lineSkip += 6 ;
#endif /* KEEPDIRS */

		pExt = strrchr ( lineSkip , '.' ) ;

		if ( ! pExt )
		{
			fprintf ( stderr ,
				"mkclnmkf: malformed filename (no '.'): %s\n" ,
				lineCln ) ;

			exit ( 1 ) ;
		}

		strcpy ( pNoExt , lineSkip ) ;
		pNoExt [ pExt - lineSkip ] = '\0' ;

		/*
		 * .INC files do not get ifstripped - use fmtCln2 for them
		 */

		fprintf ( output ,
			strcmp ( ".inc" , pExt ) ? fmtCln : fmtCln2 ,
			/* %s%s: %scrtw32\%s %ssrcrel\msvc40.if */
			prefixDst , lineSkip , prefixSrc , lineCln , prefixSrc ,
			/* echo Cleansing %scrtw32\%s */
			prefixDst , lineSkip ,
			/* if exist %s%s attrib -r %s%s */
			prefixDst , lineSkip , prefixDst , lineSkip ,
			/* detab < %scrtw32\%s | trailing > ... */
			prefixSrc , lineCln , prefixDst , lineSkip ,
			/* ifstrip %s\srcrel\msvc40.if %s%s */
			prefixSrc , prefixDst , lineSkip ,
			/* del %s%s */
			prefixDst , lineSkip ,
			/* ren %s%s.TMP *%s */
			prefixDst , pNoExt , pExt ,
			/* striphdr -r %s%s */
			prefixDst , lineSkip ,
			/* del %s%s */
			prefixDst , lineSkip ,
			/* ren %s%s.NEW *%s */
			prefixDst , pNoExt , pExt ,
			/* attrib +r %s%s */
			prefixDst , lineSkip ) ;

		/*
		 * Secondary makefile
		 */

		if ( memcmp ( "libw32" , lineCln , 6 ) )
		{
			char * lineSkipFirst = SkipFirstDir ( lineSkip ) ;

			if ( ! strcmp ( ".asm" , pExt ) &&
			   (  ! memcmp ( "mbstring" , lineSkip , 8 )
			   || ! strcmp ( "memmove.asm" , pExt - 7 )
			   || ! strcmp ( "strcspn.asm" , pExt - 7 )
			   || ! strcmp ( "strpbrk.asm" , pExt - 7 ) )
			  || ! strcmp ( ".c" , pExt ) &&
			   (  ! strcmp ( "mbscat.c" , pExt - 6 )
			   || ! strcmp ( "mbsdup.c" , pExt - 6 ) ) )
				fprintf ( output2 ,
					"%s%s: %s%s asmfile.sed\n"
					"\t" AT "echo Copying %s from %s to %s. (using SED.EXE)\n"
					"\t" AT "if exist %s%s attrib -r %s%s\n"
					"\t" AT "sed -f asmfile.sed %s%s > $@\n"
					"\t" AT "attrib +r %s%s\n"
					"\n" ,
					/* %s%s: %s%s asmfile.sed */
					prefixDst , lineSkipFirst ,
					prefixSrc , lineSkip ,
					/* echo Copying %s from %s to %s... */
					lineSkip , prefixSrc , prefixDst ,
					/* if exist %s%s attrib -r %s%s */
					prefixDst , lineSkipFirst ,
					prefixDst , lineSkipFirst ,
					/* sed -f asmfile.sed %s%s > $@ */
					prefixSrc , lineSkip ,
					/* attrib +r %s%s */
					prefixDst , lineSkipFirst ) ;
			else
				fprintf ( output2 ,
					"%s%s: %s%s\n"
					"\t" AT "echo Copying %s from %s to %s.\n"
					"\t" AT "if exist %s%s attrib -r %s%s\n"
					"\t" AT "copy /b $** $@ >NUL\n"
					"\t" AT "attrib +r %s%s\n"
					"\n" ,
					/* %s%s: %s%s */
					prefixDst , lineSkipFirst ,
					prefixSrc , lineSkip ,
					/* Copying %s from %s to %s */
					lineSkip , prefixSrc , prefixDst ,
					/* if exist %s%s attrib -r %s%s */
					prefixDst , lineSkipFirst ,
					prefixDst , lineSkipFirst ,
					/* copy /b $** $@ >NUL */
					/* attrib +r %s%s */
					prefixDst , lineSkipFirst ) ;
		}
	
		/*
		 * Show Progress 
		 */

		Progress ( '{' , lineSkip , '}' ) ;

		rvCln = fgetsNL ( lineCln , sizeof ( lineCln ) , inputCln ) ;
	}

	Progress ( '{' , NULL , '}' ) ;

	return 0 ;
}


/*
 * Usage - print message explaining the arguments to this program
 */

void Usage ( void )
{
	fprintf ( stderr ,

	"Usage: mkclnmkf prefIn prefOut prefOut2 fRaw fCln dirList mkfOut mkfOut2\n"
	"where\tprefIn is the input pref (such as \"\\crt\")\n"
	"\tprefOut is the primary output pref (such as \"\\msdev\\crt\\prebuild\")\n"
	"\tprefOut2 is the secondary output pref (such as \"\\msdev\\crt\\src\")\n"
	"\tfRaw is a list of f to be copied as-is\n"
	"\tfCln is a list of f to be cleansed\n"
	"\tdirList is a list of the directories to be created\n"
	"\tmkfOut is the primary output file (it may not already exist)\n"
	"\tmkfOut is the secondary output file (it may not already exist)\n" ) ;
}


/*
 * fgetsNL - same as fgets except final newline character is deleted
 *	Upon EOF or error, make sure the buffer is set to the empty string.
 */

char * fgetsNL ( char * lineBuf , int lineSize , FILE * fileInput )
{
	char * retval = fgets ( lineBuf , lineSize , fileInput ) ;

	if ( retval )
	{
		int len = strlen ( lineBuf ) ;

		if ( len < 1 && lineBuf [ len - 1 ] != '\n' )
		{
			fprintf ( stderr , "mkclnmkf: malformed input line:\n\t%s\n" , lineBuf ) ;
		}
		else
			lineBuf [ len - 1 ] = '\0' ;
	}
	else
		lineBuf [ 0 ] = '\0' ;

	return retval ;
}

void Progress ( int prefix , char * string , int suffix )
{
static	int prevlen = 0 ;
static	int counter = 0 ;

	if ( string == NULL )
	{
		fprintf ( stderr , "%*s\n" , prevlen + 3 , " " )  ;
		prevlen = 0 ;
		counter = 0 ;
	}
	else if ( ++ counter % 100 == 1 )
	{
		int length = strlen ( string ) ;

		fprintf ( stderr , "%c%s%c%*s\r" ,
			prefix , string , suffix ,
			prevlen > length ? prevlen - length : 1 ,
			" " ) ;

		prevlen = length ;
	}
}

char * SkipFirstDir ( char * string )
{
	char * pFile = string ;

	while ( * pFile ++ != '\\' )
		if ( ! * pFile )
		{
			fprintf ( stderr ,
				"mkclnmkf: no \\ in \"%s\"\n" , string ) ;
			exit ( 1 ) ;
		}

	return pFile ;
}
