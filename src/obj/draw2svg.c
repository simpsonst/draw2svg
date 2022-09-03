// -*- c-basic-offset: 2; indent-tabs-mode: nil -*-

/*
   draw2svg: converts RISC OS drawfiles to SVG
   Copyright (C) 2000-1,2005-6 Steven Simpson

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA


   Author contact: <https://github.com/simpsonst>

   Some parts by James Bursa.
*/

#if !defined __riscos && !defined __riscos__
#error "Target system must be RISC OS"
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#ifdef __TARGET_UNIXLIB__
#define __UNIXLIB_INTERNALS 1
#include <unixlib/local.h>
int __riscosify_control = __RISCOSIFY_NO_PROCESS;
#endif

#include "version.h"
#include "units.h"
#include "files.h"
#include "context.h"

int main(int argc, const char *const *argv)
{
  struct context ct;
  int arg;
  int dashargs = false;

  ct.iname = ct.oname = ct.bgcol = NULL;
  ct.u = choose_units("in");
  ct.thin = 1;
  ct.topxy = false;
  ct.groups = true;
  ct.itype = ct.otype = true;
  ct.abssized = SIZE_PERCENT;

  ct.partype = PAR_MEET;
  ct.parx = ct.pary = PAR_MID;
  ct.scale.factor.x = ct.scale.factor.y = 1.0;
  ct.scaletype = SCALE_FACTOR;

  ct.margin.width = ct.margin.height = 0.0;

  ct.text_to_path = false;

  for (arg = 1; arg < argc; arg++) {
    if (dashargs || (argv[arg][0] != '-' && argv[arg][0] != '+')) {
      if (!ct.iname) {
        ct.iname = (const char *) argv[arg];
      } else if (!ct.oname) {
        ct.oname = (const char *) argv[arg];
      } else {
        fprintf(stderr, "%s: superfluous argument\n", argv[arg]);
        break;
      }
    } else if (!strcmp(argv[arg], "--help") || !strcmp(argv[arg], "-h")) {
      break;
    } else if (!strcmp(argv[arg], "-xy")) {
      ct.topxy = true;
    } else if (!strcmp(argv[arg], "-it")) {
      ct.itype = true;
    } else if (!strcmp(argv[arg], "-ot")) {
      ct.otype = true;
    } else if (!strcmp(argv[arg], "+it")) {
      ct.itype = false;
    } else if (!strcmp(argv[arg], "+ot")) {
      ct.otype = false;
    } else if (!strcmp(argv[arg], "+z")) {
      ct.abssized = SIZE_NONE;
    } else if (!strcmp(argv[arg], "-z")) {
      ct.abssized = SIZE_ABS;
    } else if (!strcmp(argv[arg], "--pcsize")) {
      ct.abssized = SIZE_PERCENT;
    } else if (!strcmp(argv[arg], "-g")) {
      ct.groups = true;
    } else if (!strcmp(argv[arg], "--xmin")) {
      ct.parx = PAR_MIN;
    } else if (!strcmp(argv[arg], "--xmid")) {
      ct.parx = PAR_MID;
    } else if (!strcmp(argv[arg], "--xmax")) {
      ct.parx = PAR_MAX;
    } else if (!strcmp(argv[arg], "--ymin")) {
      ct.pary = PAR_MIN;
    } else if (!strcmp(argv[arg], "--ymid")) {
      ct.pary = PAR_MID;
    } else if (!strcmp(argv[arg], "--ymax")) {
      ct.pary = PAR_MAX;
    } else if (!strcmp(argv[arg], "--meet")) {
      ct.partype = PAR_MEET;
    } else if (!strcmp(argv[arg], "--slice")) {
      ct.partype = PAR_SLICE;
    } else if (!strcmp(argv[arg], "--noar")) {
      ct.partype = PAR_NONE;
    } else if (!strcmp(argv[arg], "--thin")) {
      if (arg + 2 > argc) {
        fprintf(stderr, "%s: needs argument\n", argv[arg]);
        break;
      }
      if (measure(argv[arg + 1], &ct.thin) < 0) {
        fprintf(stderr, "%s: invalid\n", argv[arg + 1]);
        break;
      }
      arg++;
    } else if (!strcmp(argv[arg], "--units") ||
               !strcmp(argv[arg], "-u")) {
      const struct unit *up;
      if (arg + 2 > argc) {
        fprintf(stderr, "%s: needs argument\n", argv[arg]);
        break;
      }
      if (!(up = choose_units(argv[arg + 1]))) {
        fprintf(stderr, "%s: invalid\n", argv[arg + 1]);
        break;
      }
      ct.u = up;
      arg++;
    } else if (!strcmp(argv[arg], "-bg")) {
      if (arg + 2 > argc) {
        fprintf(stderr, "%s: needs colour argument\n", argv[arg]);
        break;
      }
      ct.bgcol = argv[++arg];
    } else if (!strcmp(argv[arg], "--scale")) {
      if (arg + 2 > argc) {
        fprintf(stderr, "%s: needs x-scale[,y-scale]\n", argv[arg]);
        break;
      }
      switch (sscanf(argv[arg + 1], "%lf,%lf",
              &ct.scale.factor.x, &ct.scale.factor.y)) {
      case 2:
        ct.scaletype = SCALE_FACTOR;
        break;
      case 1:
        ct.scaletype = SCALE_FACTOR;
        ct.scale.factor.y = ct.scale.factor.x;
        break;
      default:
        fprintf(stderr, "%s: scale factors not parsed\n", argv[arg + 1]);
        arg = argc;
        break;
      }
      ++arg;
    } else if (!strcmp(argv[arg], "--margin")) {
      char wstr[20], hstr[20];

      if (arg + 2 > argc) {
        fprintf(stderr, "%s: needs width[,height]\n", argv[arg]);
        break;
      }
      switch (sscanf(argv[arg + 1], "%19[^,],%19s", wstr, hstr)) {
      case 2:
        if (measure(wstr, &ct.margin.width) ||
            measure(hstr, &ct.margin.height)) {
          fprintf(stderr, "%s: margin not parsed\n", argv[arg + 1]);
          arg = argc;
          break;
        }
        break;
      case 1:
        if (measure(wstr, &ct.margin.width)) {
          fprintf(stderr, "%s: margin not parsed\n", argv[arg + 1]);
          arg = argc;
          break;
        }
        ct.margin.height = ct.margin.width;
        break;
      default:
        fprintf(stderr, "%s: margin not parsed\n", argv[arg + 1]);
        arg = argc;
        break;
      }
      ++arg;
    } else if (!strcmp(argv[arg], "--fit")) {
      char wstr[20], hstr[20];

      if (arg + 2 > argc) {
        fprintf(stderr, "%s: needs [*|width],[*|height]\n", argv[arg]);
        break;
      }
      if (sscanf(argv[arg + 1], "%19[^,],%19s", wstr, hstr) != 2) {
        fprintf(stderr, "%s: needs [*|width],[*|height]\n", argv[arg]);
        arg = argc;
        break;
      }
      if (!strcmp(wstr, "*")) {
        if (!strcmp(hstr, "*")) {
          /* Reset scale. */
          ct.scaletype = SCALE_FACTOR;
          ct.scale.factor.y = ct.scale.factor.x = 1.0;
        } else {
          /* Scale to fit height. */
          if (measure(hstr, &ct.scale.fit.height))
            goto margin_failed;
          ct.scaletype = SCALE_HEIGHT;
        }
      } else if (!strcmp(hstr, "*")) {
        /* Scale to fit width. */
        if (measure(wstr, &ct.scale.fit.width))
          goto margin_failed;
        ct.scaletype = SCALE_WIDTH;
      } else {
        /* Scale to fit rectangle. */
        if (measure(wstr, &ct.scale.fit.width) ||
            measure(hstr, &ct.scale.fit.height))
          goto margin_failed;
        ct.scaletype = SCALE_FIT;
      }
      ++arg;
      continue;

     margin_failed:
      fprintf(stderr, "%s: needs [*|width],[*|height]\n", argv[arg]);
      break;
    } else if (!strcmp(argv[arg], "+bg")) {
      ct.bgcol = NULL;
    } else if (!strcmp(argv[arg], "--no-margin")) {
      ct.margin.width = ct.margin.height = 0.0;
    } else if (!strcmp(argv[arg], "--no-scale") ||
               !strcmp(argv[arg], "--no-fit")) {
      ct.scale.factor.x = ct.scale.factor.y = 1.0;
      ct.scaletype = SCALE_FACTOR;
    } else if (!strcmp(argv[arg], "--")) {
      dashargs = true;
    } else if (!strcmp(argv[arg], "+xy")) {
      ct.topxy = false;
    } else if (!strcmp(argv[arg], "+g")) {
      ct.groups = false;
    } else if (!strcmp(argv[arg], "--text-to-path")) {
      ct.text_to_path = true;
    } else {
      fprintf(stderr, "%s: unrecognised option\n", argv[arg]);
      break;
    }
  }

  if (arg < argc || !ct.iname || !ct.oname) {
    fprintf(stderr, "Draw-to-SVG converter (back end) %s (%s)\n",
            linkversion, linkdate);
    fprintf(stderr, "usage: %s [options] [--] infile outfile\n", argv[0]);
    fprintf(stderr, "\t--help  display this text\n");
    fprintf(stderr, "\t--thin number[unit]\n\t\twidth of thin lines\n");
    fprintf(stderr, "\t--units/-u unit\n\t\tselect units\n");
    fprintf(stderr, "\t-/+xy    set x and y attrs on top-level <svg>\n");
    fprintf(stderr, "\t-/+g     preserve groups as <g>...</g>\n");
    fprintf(stderr, "\t-/+it    check input file type\n");
    fprintf(stderr, "\t-/+ot    set output file type\n");
    fprintf(stderr, "\t-z       set absolute image size\n");
    fprintf(stderr, "\t+z       do not set absolute image size\n");
    fprintf(stderr, "\t--pcsize set image size as percentages\n");
    fprintf(stderr, "\t-bg col  set background colour\n");
    fprintf(stderr, "\t+bg      clear background colour\n");
    fprintf(stderr, "\t--text-to-path\n"
            "\t\tconvert text to paths (else assume Latin-1)\n");
    fprintf(stderr, "\t--scale x[,y]\n\t\tset scale factors\n");
    fprintf(stderr, "\t--no-scale\n\t--no-fit\n\t\tremove scale/fit\n");
    fprintf(stderr, "\t--margin width[,height]\n\t\tset margin\n");
    fprintf(stderr, "\t--no-margin\n\t\tremove margin\n");
    fprintf(stderr, "Aspect-ratio options:\n");
    fprintf(stderr, "\t--meet   show all of image and preserve AR (default)\n");
    fprintf(stderr, "\t--slice  fill viewbox and preserve AR\n");
    fprintf(stderr, "\t--noar   don't preserve AR\n");
    fprintf(stderr, "\t--xmin/xmid/xmax\n\t\tX alignment (default: mid)\n");
    fprintf(stderr, "\t--ymin/ymid/ymax\n\t\tY alignment (default: mid)\n");
    return EXIT_FAILURE;
  }

  if (process(&ct)) {
    fprintf(stderr, "%s: %s\n", argv[0], strerror(errno));
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
