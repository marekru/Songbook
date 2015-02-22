/* Copyright (C) 2013 Kevin W. Hamlen
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA  02110-1301, USA.
 *
 * The latest version of this program can be obtained from
 * http://songs.sourceforge.net.
 *
 *
 * This program generates an index.sbx file from an index.sxd file. Index.sxd
 * files are generated by the song book LaTeX files while you compile a song
 * book. They contain lists of all the song titles, song numbers, and
 * scripture references. Once the .sxd files exist, run this program to
 * convert them in to .sbx files. This program will sort the song titles
 * alphabetically, group all the scripture entries into the appropriate
 * chunks, etc. The TeX files that it creates are then used by the LaTeX
 * files next time you compile the song book.
 *
 * The syntax for running the program is:
 *    songidx input.sxd output.sbx
 * or, if you're compiling a scripture index:
 *    songidx -b bible.can input.sxd output.sbx
 *
 * The program should compile on unix with
 *
 *   cc *.c -o songidx
 *
 * On Windows, you'll need to find a C compiler in order to compile it,
 * or go to http://songs.sourceforge.net to obtain a Windows self-installer
 * with pre-compiled binaries.
 */

#if HAVE_CONFIG_H
#  include "config.h"
#else
#  include "vsconfig.h"
#endif

#include "chars.h"
#include "songidx.h"
#include "fileio.h"

#if HAVE_STRING_H
#  include <string.h>
#elif HAVE_STRINGS_H
#  include <strings.h>
#endif

#if HAVE_SETLOCALE
#if HAVE_LOCALE_H
#  include <locale.h>
#endif
#endif

extern int gentitleindex(FSTATE *fs, const char *outname);
extern int genscriptureindex(FSTATE *fs, const char *outname,
                             const char *biblename);
extern int genauthorindex(FSTATE *fs, const char *outname);

#define BIBLEDEFAULT "bible.can"

#if HAVE_STRRCHR
#  define STRRCHR(x,y) strrchr((x),(y))
#else
#  define STRRCHR(x,y) local_strrchr((x),(y))
char *
local_strrchr(s,c)
  const char *s;
  int c;
{
  const char *t;
  if (!s) return NULL;
  for (t=s; *t; ++s) ;
  for (; t>=s; --s)
    if (*t == c) return t;
  return NULL;
}
#endif

int
main(argc,argv)
  int argc;
  char *argv[];
{
  FSTATE fs;
  int eof = 0;
  WCHAR buf[MAXLINELEN];
  int retval;
  const char *bible, *inname, *outname, *locale;
  int i;

  bible = inname = outname = locale = NULL;
  for (i=1; i<argc; ++i)
  {
    if (!strcmp(argv[i],"-v") || !strcmp(argv[i],"--version"))
    {
      printf("songidx " VERSION "\n"
        "Copyright (C) 2013 Kevin W. Hamlen\n"
        "License GPLv2: GNU GPL version 2 or later"
        " <http://gnu.org/licenses/gpl.html>\n"
        "This is free software: you are free to change and redistribute it.\n"
        "There is NO WARRANTY, to the extent permitted by law.\n");
      return 0;
    }
    else if (!strcmp(argv[i],"-h") || !strcmp(argv[i],"--help"))
    {
      printf("Syntax: %s [options] input.sxd [output.sbx]\n", argv[0]);
      printf(
        "Available options:\n"
        "  -b FILE              Sets the bible format when generating a\n"
        "  --bible FILE          scripture index (default: " BIBLEDEFAULT ")\n"
        "\n"
        "  -l LOCALE            Overrides the default system locale (affects\n"
        "  --locale LOCALE       (how non-English characters are parsed).\n"
        "                        See local system help for valid LOCALE's.\n"
        "\n"
        "  -h                   Displays this help file and stops.\n"
        "  --help\n"
        "\n"
        "  -o FILE              Sets the output filename.\n"
        "  --output FILE\n"
        "\n"
        "  -v                   Print version information and stop.\n"
        "  --version\n"
        "\n"
        "If omitted, [output.sbx] defaults to the input filename but with\n"
        "the file extension renamed to '.sbx'. To read or write to stdin or\n"
        "stdout, use '-' in place of input.sxd or output.sbx.\n"
        "\n"
        "See http://songs.sourceforge.net for support.\n");
      return 0;
    }
    else if (!strcmp(argv[i],"-b") || !strcmp(argv[i],"--bible"))
    {
      if (bible)
      {
        fprintf(stderr, "songidx: multiple bible files specified\n");
        return 2;
      }
      else if (++i < argc)
        bible = argv[i];
      else
      {
        fprintf(stderr, "songidx: %s option requires an argument\n", argv[i-1]);
        return 2;
      }
    }
    else if (!strcmp(argv[i],"-l") || !strcmp(argv[i],"--locale"))
    {
      if (locale)
      {
        fprintf(stderr, "songidx: multiple locales specified\n");
        return 2;
      }
      else if (++i < argc)
        locale = argv[i];
      else
      {
        fprintf(stderr, "songidx: %s option requires an argument\n", argv[i-1]);
        return 2;
      }
    }
    else if (!strcmp(argv[i],"-o") || !strcmp(argv[i],"--output"))
    {
      if (outname)
      {
        fprintf(stderr, "songidx: multiple output files specified\n");
        return 2;
      }
      else if (++i < argc)
        outname = argv[i];
      else
      {
        fprintf(stderr, "songidx: %s option requires an argument\n", argv[i-1]);
        return 2;
      }
    }
    else if ((argv[i][0] == '-') && (argv[i][1] != '\0'))
    {
      fprintf(stderr, "songidx: unknown option %s\n", argv[i]);
      return 2;
    }
    else if (!inname) inname=argv[i];
    else if (!outname) outname=argv[i];
    else
    {
      fprintf(stderr, "songidx: too many command line arguments\n");
      return 2;
    }
  }

#if HAVE_SETLOCALE
  if (!locale)
    (void) setlocale(LC_ALL, "");
  else if (!setlocale(LC_ALL, locale))
  {
    fprintf(stderr, "songidx: invalid locale: %s\n", locale);
    return 2;
  }
#endif

  if (!inname)
  {
    fprintf(stderr, "songidx: no input file specified\n");
    return 2;
  }
  if (!outname)
  {
    if (!strcmp(inname,"-"))
      outname = "-";
    else
    {
      char *b;
      const char *p1 = STRRCHR(inname,'.');
      if ((p1 < STRRCHR(inname,'/')) || (p1 < STRRCHR(inname,'\\')))
        p1 = inname+strlen(inname);
      b = (char *) malloc(p1-inname+5);
      strncpy(b,inname,p1-inname);
      strcpy(b+(p1-inname),".sbx");
      outname = b;
    }
  }
  if (!bible) bible = BIBLEDEFAULT;

  if (!fileopen(&fs, inname)) return 2;

  /* The first line of the input file is a header line that dictates which kind
   * of index the data file is for. */
  if (!filereadln(&fs, buf, &eof)) return 2;
  retval = 2;
  if (eof)
  {
    fprintf(stderr, "songidx:%s: file is empty\n", fs.filename);
    fileclose(&fs);
  }
  else if (!ws_strcmp(buf, ws_lit("TITLE INDEX DATA FILE")))
    retval = gentitleindex(&fs, outname);
  else if (!ws_strcmp(buf, ws_lit("SCRIPTURE INDEX DATA FILE")))
    retval = genscriptureindex(&fs, outname, bible);
  else if (!ws_strcmp(buf, ws_lit("AUTHOR INDEX DATA FILE")))
    retval = genauthorindex(&fs, outname);
  else
  {
    fprintf(stderr, "songidx:%s:%d: file has unrecognized format\n",
            fs.filename, fs.lineno);
    fileclose(&fs);
  }

  /* Report the outcome to the user. */
  if (retval == 0)
    fprintf(stderr, "songidx: Done!\n");
  else if (retval == 1)
    fprintf(stderr, "songidx: COMPLETED WITH ERRORS. SEE ABOVE.\n");
  else
    fprintf(stderr, "songidx: FAILED. SEE ERROR MESSAGES ABOVE.\n");

  return retval;
}
