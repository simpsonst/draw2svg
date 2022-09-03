// -*- c-basic-offset: 2; indent-tabs-mode: nil -*-

/*
   draw2svg: converts RISC OS drawfiles to SVG
   Copyright (C) 2000-1,2005-6,2012,2019  Steven Simpson

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

#ifndef CONTEXT_H
#define CONTEXT_H

#include <stdio.h>

#ifndef false
#define false 0
#endif

#ifndef true
#define true 1
#endif

#define RELCOORDS

#define LINE_WIDTH 76

enum { SCALE_FACTOR, SCALE_WIDTH, SCALE_HEIGHT, SCALE_FIT };
enum { PAR_MIN, PAR_MID, PAR_MAX };
enum { PAR_NONE, PAR_MEET, PAR_SLICE };
enum { SIZE_NONE, SIZE_PERCENT, SIZE_ABS };

struct context {
  const char *iname, *oname, *bgcol;
  const struct unit *u;
  double thin;
  union {
    struct {
      double width, height;
    } fit;
    struct {
      double x, y;
    } factor;
  } scale;
  struct {
    double width, height;
  } margin;
  unsigned topxy : 1, groups : 1, itype : 1, otype : 1, scaletype : 2,
    abssized : 2;
  unsigned parx, pary, partype;
  unsigned text_to_path;
};

struct ws {
  struct context *ct;
  FILE *out;
  int cpos;
  void *buf;
  int indent;
  int *bbox;
  const char *font[256];
};

int process(struct context *);
void indent(struct ws *ws);
void cindent(struct ws *ws, int *spp, int req);
int output(struct ws *ws, int pretty, const char *fmt, ...);

#define OUT_PRETTY   1u
#define OUT_ESCAMP   2u
#define OUT_ESCLT    4u
#define OUT_ESCGT    8u
#define OUT_ESCQUOT 16u
#define OUT_ESCAPOS 32u

#define OUT_ESCCDATA (OUT_ESCGT | OUT_ESCLT | OUT_ESCAMP)

#define OUT_ESCSQSTR \
  (OUT_ESCAPOS | OUT_ESCGT | OUT_ESCLT | OUT_ESCAMP)

#define OUT_ESCDQSTR \
  (OUT_ESCQUOT | OUT_ESCGT | OUT_ESCLT | OUT_ESCAMP)

#define OUT_ESCPCDATA \
  (OUT_ESCAPOS | OUT_ESCQUOT | OUT_ESCGT | OUT_ESCLT | OUT_ESCAMP)

int output_esc(struct ws *ws, unsigned flags, const char *fmt, ...);

#endif
