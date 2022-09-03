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

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <math.h>
#include <string.h>

#include <kernel.h>
#include <swis.h>

#include <riscos/swi/OS.h>
#include <riscos/swi/Draw.h>
#include <riscos/swi/Font.h>
#include <riscos/swi/ColourTrans.h>

#include "version.h"
#include "context.h"
#include "files.h"
#include "units.h"

const char *join_str[] = { "miter", "round", "bevel", "inherit" };
const char *cap_str[] = { "butt", "round", "square", "inherit" };
const char *wind_str[] = { "nonzero", "evenodd" };

void convert(struct ws *, const int *);

void convert_tagged(struct ws *ws, const int *d)
{
  convert(ws, d + 6);
}

void plot_path(struct ws *ws, const int *d, const int *e);

void convert_text_path(struct ws *ws, const int *d)
{
  _kernel_oserror *err = NULL;
  int off = d[0] == 12 ? 7 : 0;
  int fh;
  size_t len;
  unsigned flags = 1 << 8;
  int ftr;
  int *pos, *end;
  int or0, or1;
  int rule;
  int fn;
  struct {
    const char *name;
    double xscale, yscale;
  } altfont[] = {
    { NULL, 1.0, 1.0 },
    { "System.Fixed", 1.0, 1.0 },
    { "Corpus.Medium", 1.6, 1.5 }
  };

  if (d[0] == 12) {
    off = 7;
    flags |= (1 << 6) | ((d[12] & 3) << 9);
  } else {
    off = 0;
  }

  altfont[0].name = ws->font[d[8 + off] & 0xff];
  for (fn = !strcmp(altfont[0].name, altfont[1].name);
       fn >= 0 && (size_t) fn < sizeof altfont / sizeof altfont[0] &&
       (err = _swix(Font_FindFont, _INR(1,5)|_OUT(0), altfont[fn].name,
                    (int) (d[9 + off] * altfont[fn].xscale / 40),
                    (int) (d[10 + off] * altfont[fn].yscale / 40),
                    0, 0, &fh)) != NULL;
       fn++) {
    fprintf(stderr, "Font \"%s\" conversion: %s\n",
            (char *) (d + 13 + off), err->errmess);
  }
  if (err)
    return;

  _swi(ColourTrans_SetFontColours, _INR(0,3),
       fh, 0xffffff00, 0x00000000, 14);
  _swi(Font_SwitchOutputToBuffer, _INR(0,1)|_OUTR(0,1), 1, 8, &or0, &or1);
  err = _swix(Font_Paint, _INR(0,4)|_IN(6), fh, (d + 13 + off), flags,
              d[11 + off] * 25 / 16, d[12 + off] * 25 / 16, d + 6);
  _swi(Font_SwitchOutputToBuffer, _INR(0,1)|_OUT(1), 0, -1, &len);
  if (err) {
    _swi(Font_SwitchOutputToBuffer, _INR(0,1), 0, 0);
    _swi(Font_LoseFont, _IN(0), fh);
    fprintf(stderr, "Font \"%s\" conversion: %s\n",
            (char *) (d + 13 + off), err->errmess);
    return;
  }

  ws->buf = realloc(ws->buf, len);
  if (!ws->buf) {
    printf("Out of memory.\n");
    _swi(Font_SwitchOutputToBuffer, _INR(0,1), 0, 0);
    _swi(Font_LoseFont, _IN(0), fh);
    exit(EXIT_FAILURE);
  }
  ((int *) ws->buf)[0] = 0;
  ((int *) ws->buf)[1] = len - 8;
  _swi(Font_SwitchOutputToBuffer, _INR(0,1), 0, ws->buf);
  err = _swix(Font_Paint, _INR(0,4)|_IN(6), fh, (d + 13 + off), flags,
              d[11 + off] * 25 / 16, d[12 + off] * 25 / 16, d + 6);
  _swi(Font_SwitchOutputToBuffer, _INR(0,1), 0, 0);
  _swi(Font_LoseFont, _IN(0), fh);

  if (err && err->errnum != 0x1e4) {
    fprintf(stderr, "Font \"%s\" conversion: %x %s\n",
            (char *) (d + 13 + off), err->errnum, err->errmess);
    return;
  }

  end = (int *) ws->buf + (len >> 2);
  {
    int count[2] = { 0, 0 };
    pos = ws->buf;
    while (pos < end && pos[0]) {
      count[pos[9] >> 6 & 1]++;
      pos += pos[1] >> 2;
    }
    rule = count[1] > count[0];
  }
#if false
  output(ws, false, "<!-- this is text-sourced -->\n");
#endif
  output(ws, false, "<g style='");
  ws->indent += 10;

  output(ws, false, "stroke: none;\n");

  ftr = ~d[6 + off] & 0xff;

  if (ftr == 0) {
    output(ws, false, "fill: none;\n");
  } else {
    if (ftr != 255) {
      output(ws, false, "fill-opacity: %g;\n", ftr/255.0);
    }
    if (rule != 1) {
      output(ws, false, "fill-rule: %s;\n", wind_str[rule]);
    }
    output(ws, false, "fill: #%02X%02X%02X;'>\n",
            (d[6 + off]>>8)&0xff,
            (d[6 + off]>>16)&0xff,
            (d[6 + off]>>24)&0xff);
  }

  ws->indent -= 8;

  pos = ws->buf;
#if false
  end = (int *) ws->buf + (len >> 2);
#endif
  while (pos < end && pos[0]) {
    int *next = pos + (pos[1] >> 2);
    int r = pos[9] >> 6 & 1;

    output(ws, false, "<path ");
    ws->indent += 6;

    if (r != rule) {
      output(ws, false, "style='fill-rule: %s;'\n", wind_str[r]);
    }

    output(ws, false, "d='");
    ws->indent += 3;

#if false
    printf("Path style: %s\n",
           (pos[9] & (1<<6)) ? "non-zero" : "even-odd");
#endif

    pos += 10;
    if (pos[-1] & 0x80)
      pos += pos[1] + 2;
    plot_path(ws, pos, next - 1);

    output(ws, false, "' />\n");
    ws->indent -= 9;

    pos = next;
  }
  ws->indent -= 2;
  output(ws, false, "</g>\n");
}

void convert_text(struct ws *ws, const int *d)
{
  int off = d[0] == 12 ? 7 : 0;
  int ftr;

  if (d[0] == 12) {
    off = 7;
  } else {
    off = 0;
  }

  output(ws, false, "<text x='%i' y='%i'\n", d[11 + off], -d[12 + off]);
  ws->indent += 10;

  if (d[0] == 12) {
    double ma = d[6] / 65536.0;
    double mb = d[7] / 65536.0;
    double mc = d[8] / 65536.0;
    double md = d[9] / 65536.0;
    double det = ma * md - mb * mc;
    output(ws, false, "transform='translate(%i %i) "
            "matrix(%g %g %g %g %g %g) "
            "translate(%i %i)'\n",
            d[11 + off],
            -d[12 + off],
            md / det,
            -mb / det,
            -mc / det,
            ma / det,
            -d[10] / 65536.0,
            -d[11] / 65536.0,
            -d[11 + off],
            d[12 + off]);
  }

  output(ws, false, "style='");
  output(ws, false, "font-family: \"%s\";\n", ws->font[d[8 + off] & 0xff]);
  output(ws, false, "font-size: %i;\n", d[9 + off]);

  ftr = ~d[6 + off] & 0xff;
  if (ftr == 0) {
    output(ws, false, "fill: none;\n");
  } else {
    if (ftr != 255) {
      output(ws, false, "fill-opacity: %g;\n", ftr/255.0);
    }
    output(ws, false, "fill: #%02X%02X%02X;'>",
            (d[6 + off]>>8)&0xff,
            (d[6 + off]>>16)&0xff,
            (d[6 + off]>>24)&0xff);
  }

  output_esc(ws, OUT_ESCCDATA, "%s", d + 13 + off);

  ws->indent -= 10;
  output(ws, false, "</text>\n");
}

void convert_list(struct ws *ws, const int *p, const int *e)
{
  for (; p < e; p += (p[1] >> 2))
    convert(ws, p);
}

void convert_group(struct ws *ws, const int *d)
{
  const int *e;
  int preserve = ws->ct->groups;

  e = (const int *) (d + (d[1] >> 2));

  if (preserve) {
    output(ws, false, "<g>\n");
    ws->indent += 2;
  }
#if false
  output(ws, false, "<desc>%12.12s</desc>\n", (char *) (d + 6));
#endif
  convert_list(ws, d + 9, e);
  if (preserve) {
    ws->indent -= 2;
    output(ws, false, "</g>\n");
  }
}

void plot_circ(struct ws *ws, int *spp, int fx, int fy,
               int tx, int ty, int h)
{
  int dx = tx - fx, dy = ty - fy;
  double r = h / 2.0 / sqrt((double) dx * dx + (double) dy * dy);

  spp = spp;

  output(ws, true, "M%g %g", tx - r * dy, -(ty + r * dx));
  output(ws, true, "A%g %g", h / 2.0, h / 2.0);
  output(ws, true, " 0 0 1 %g %gz", tx + r * dy, -(ty - r * dx));
}

void plot_square(struct ws *ws, int *spp, int fx, int fy,
                 int tx, int ty, int h)
{
  int dx = tx - fx, dy = ty - fy;
  double r = h / 2.0 / sqrt((double) dx * dx + (double) dy * dy);

  spp = spp;

  output(ws, true, "M%g %g", tx - r * dy, -(ty + r * dx));
  output(ws, true, "l%g %g", r * dx, -r * dy);
  output(ws, true, "l%g %g", 2 * r * dy, 2 * r * dx);
  output(ws, true, "l%g %gz", -r * dx, -r * dy);
}

void plot_tri(struct ws *ws, int *spp, int fx, int fy,
              int tx, int ty, double w, double h)
{
  int dx = tx - fx, dy = ty - fy;
  double r = sqrt((double) dx * dx + (double) dy * dy);

  spp = spp;

  w /= r;
  h /= r;

  output(ws, true, "M%g %g", tx - w * dy, -(ty + w * dx));
  output(ws, true, "l%g %g", w * dy + h * dx, w * dx - h * dy);
  output(ws, true, "l%g %gz", w * dy - h * dx, h * dy + w * dx);
}

void record(const int *p[], int n, const int *nw)
{
  int i;

  for (i = 0; i < n - 1; i++)
    p[i] = p[i + 1];
  p[i] = nw;
}

void plot_caps(struct ws *ws, const int *d, const int *e,
               int scap, int ecap, int wid, int triwf, int trihf)
{
  double triw = 0.0, trih = 0.0;
  const int *p[2] = { NULL };
  int start = 1;
  int sp = LINE_WIDTH - ws->indent;

  if (scap == 3 || ecap == 3) {
    triw = (double) wid * triwf / 16.0;
    trih = (double) wid * trihf / 16.0;
  }

  while (d < e) {
#if false
    printf("Code: %d\n", (int) d[0]);
#endif
    switch (d[0]) {
    case 0:
    case 2:
    case 3:
    case 7:
      if (!start) {
        switch (ecap) {
        case 1:
          plot_circ(ws, &sp, p[0][0], p[0][1], p[1][0], p[1][1], wid);
          break;
        case 2:
          plot_square(ws, &sp, p[0][0], p[0][1], p[1][0], p[1][1], wid);
          break;
        case 3:
          plot_tri(ws, &sp, p[0][0], p[0][1], p[1][0], p[1][1], triw, trih);
          break;
        }
        start = 1;
      }
      if (d[0] == 0)
        return;
      record(p, 2, d + 1);
      d += 3;
      break;
    case 4:
      start = 1;
    case 5:
      d += 1;
      break;
    case 6:
      fprintf(stderr, "Bezier in thickened path?\n");
      d += 7;
      break;
    case 8:
      if (start) {
        switch (scap) {
        case 1:
          plot_circ(ws, &sp, d[1], d[2], p[1][0], p[1][1], wid);
          break;
        case 2:
          plot_square(ws, &sp, d[1], d[2], p[1][0], p[1][1], wid);
          break;
        case 3:
          plot_tri(ws, &sp, d[1], d[2], p[1][0], p[1][1], triw, trih);
          break;
        }
        start = 0;
      }
      record(p, 2, d + 1);
      d += 3;
      break;
    default:
      fprintf(stderr, "Cap path aborted: element is %d\n", d[0]);
      return;
    }
  }
}

void plot_path(struct ws *ws, const int *d, const int *e)
{
#if false
  struct context *ctp = ws->ct;
#endif

#ifdef RELCOORDS
  int lx = 0, ly = 0;
#endif

  while (d < e) {
#if false
    printf("Code: %d\n", (int) d[0]);
#endif
    switch (d[0]) {
    case 0:
      return;
    case 2:
      output(ws, true, "M%d %d", d[1], -d[2]);
#ifdef RELCOORDS
      lx = d[1], ly = d[2];
#endif
      d += 3;
      break;
    case 5:
      output(ws, true, "z");
      d += 1;
      break;
    case 6:
#ifdef RELCOORDS
      output(ws, true, "c%d %d %d %d %d %d",
                    d[1] - lx, -(d[2] - ly),
                    d[3] - lx, -(d[4] - ly),
                    d[5] - lx, -(d[6] - ly));
      lx = d[5], ly = d[6];
#else
      output(ws, true, "C%d %d %d %d %d %d",
                    d[1], -d[2], d[3], -d[4], d[5], -d[6]);
#endif
      d += 7;
      break;
    case 8:
#ifdef RELCOORDS
      if (d[1] == lx) {
        output(ws, true, "v%d", -(d[2] - ly));
      } else if (d[2] == ly) {
        output(ws, true, "h%d", d[1] - lx);
      } else {
        output(ws, true, "l%d %d", d[1] - lx, -(d[2] - ly));
      }
      lx = d[1], ly = d[2];
#else
      output(ws, true, "L%d %d", d[1], -d[2]);
#endif
      d += 3;
      break;
    default:
      fprintf(stderr, "Path aborted: element is %d\n", d[0]);
      return;
    }
  }
}

void xappend(double *var, const double *arg)
{
  double temp[6];
  int i;

  temp[0] = var[0] * arg[0] + var[1] * arg[2];
  temp[1] = var[0] * arg[1] + var[1] * arg[3];
  temp[2] = var[2] * arg[0] + var[3] * arg[2];
  temp[3] = var[2] * arg[1] + var[3] * arg[3];
  temp[4] = var[4] * arg[0] + var[5] * arg[2] + arg[4];
  temp[5] = var[4] * arg[1] + var[5] * arg[3] + arg[5];

  for (i = 0; i < 6; i++)
    var[i] = temp[i];
}

double fnmax(int q, double fact, double max, ...)
{
  double test;
  va_list ap;

  va_start(ap, max);
  while (--q > 0) {
    test = va_arg(ap, double);
    if (test * fact > max * fact)
      max = test;
  }
  va_end(ap);
  return max;
}

void xapply(double *var, const double *arg)
{
  double temp = arg[0] * var[0] + arg[2] * var[1] + arg[4];
  var[1] = arg[1] * var[0] + arg[3] * var[1] + arg[5];
  var[0] = temp;
}

/* Get a sprite's dimensions in OS units. */
void get_sprite_size(int *xp, int *yp, const void *ptr)
{
  unsigned int mode;
  _kernel_oserror *r;

  r = _swix(XOS_SpriteOp, _INR(0,2)|_OUTR(3,4)|_OUT(6),
            40 + 512, 0x100, ptr,
            xp, yp, &mode);
  if (r) {
    fprintf(stderr, "Error getting sprite size:\n%s\n", r->errmess);
    exit(1);
  }
  if ((mode >> 27) == 0) {
    /* Read mode variables for resolution. */
    int factor;
    _swi(OS_ReadModeVariable, _INR(0,1)|_OUT(2), mode & 0x7f, 4, &factor);
    *xp <<= factor;
    _swi(OS_ReadModeVariable, _INR(0,1)|_OUT(2), mode & 0x7f, 5, &factor);
    *yp <<= factor;
  } else {
    /* Resolution is encoded in mode. */
    *xp = *xp * 180 / ((mode >> 1) & 0x1fff);
    *yp = *yp * 180 / ((mode >> 14) & 0x1fff);
  }
}

void convert_sprite(struct ws *ws, const int *d)
{
  const int *sd;
  int osx, osy;
  double drx, dry;
  double xf[6] = { 1.0, 0.0, 0.0, 1.0, 0.0, 0.0 };
  double arg[6];
  double bl[2], tr[2], br[2], tl[2];
  double bbox[4];

  if (d[0] == 13) {
    sd = d + 12;
    xf[0] = d[6] / 65536.0;
    xf[1] = d[7] / 65536.0;
    xf[2] = d[8] / 65536.0;
    xf[3] = d[9] / 65536.0;
    xf[4] = d[10];
    xf[5] = d[11];
#if false
    output(ws, false, "<!-- transformed sprite -->\n");
#endif
  } else {
    sd = d + 6;
#if false
    output(ws, false, "<!-- sprite -->\n");
#endif
  }

  output(ws, false, "<rect style='fill: #777'\n");
  ws->indent += 6;

  /* Get the width and height of the sprite in OS units. */
  get_sprite_size(&osx, &osy, sd);
#if false
  printf("Sprite size: %d x %d OS units\n", osx, osy);
#endif

  /* Get the width and height of the sprite in draw units. */
  drx = osx * (256.0 / 180.0);
  dry = osy * (256.0 / 180.0);
  output(ws, false, "width='%g' height='%g'\n", drx, dry);

  /* Specify the top-left corner before transformation. */
  output(ws, false, "x='0' y='%g'\n", -dry);

  /* Set the initial bounding box. */
  bl[0] = bl[1] = br[1] = tl[0] = 0.0;
  br[0] = tr[0] = drx;
  tl[1] = tr[1] = dry;

  /* Transform the box using the supplied (or identity) matrix. */
  xapply(bl, xf);
  xapply(tl, xf);
  xapply(br, xf);
  xapply(tr, xf);

  /* Get the transformed bounding box. */
  bbox[0] = fnmax(4, -1.0, bl[0], tr[0], br[0], tl[0]); /* min x */
  bbox[2] = fnmax(4, +1.0, bl[0], tr[0], br[0], tl[0]); /* max x */
  bbox[1] = fnmax(4, -1.0, bl[1], tr[1], br[1], tl[1]); /* min y */
  bbox[3] = fnmax(4, +1.0, bl[1], tr[1], br[1], tl[1]); /* may y */

  /* Pre-multiply the transformation by the y-inverter. */
  xf[2] = -xf[2];
  xf[3] = -xf[3];

  /* Append an initial shift to matrix. */
  xf[4] -= bbox[0];
  xf[5] -= bbox[1];

  /* Append scaling + translation to fit final b-box. */
  arg[0] = (d[4] - d[2]) / (bbox[2] - bbox[0]); /* x scale */
  arg[3] = (d[5] - d[3]) / (bbox[3] - bbox[1]); /* y scale */
  arg[1] = arg[2] = 0.0; /* no rotation */
  arg[4] = d[2]; /* final b-box x */
  arg[5] = d[3]; /* final b-box y */
  xappend(xf, arg);

  /* Post-multiply the transformation by the y-inverter. */
  xf[1] = -xf[1];
  xf[3] = -xf[3];
  xf[5] = -xf[5];

  output(ws, false, "transform='matrix(%g,%g,%g,%g,%g,%g)' />\n",
          xf[0], xf[1], xf[2], xf[3], xf[4], xf[5]);
  ws->indent -= 6;

#if false
  output(ws, false, "<!-- scale(1,-1)");
  output(ws, false, "translate(%d,%d)", d[2], d[3]);
  output(ws, false, "scale(%g,%g)", arg[0], arg[3]);
  output(ws, false, "translate(%g,%g)", -bbox[0], -bbox[1]);
  if (d[0] == 13)
    output(ws, false, "matrix(%g,%g,%g,%g,%d,%d)",
            d[6] / 65536.0, d[7] / 65536.0, d[8] / 65536.0,
            d[9] / 65536.0, d[10], d[11]);
  output(ws, false, "scale(1,-1) -->\n");
#endif
}

void convert_path(struct ws *ws, const int *d)
{
#if false
  struct context *ctp = ws->ct;
#endif

  int join = d[9]&3;
  int scap = (d[9]>>4)&3;
  int ecap = (d[9]>>2)&3;
  int wind = (d[9]>>6)&1;
  int dash = (d[9]>>7)&1;
  unsigned otr = ~d[7] & 0xff;
  unsigned ftr = ~d[6] & 0xff;

  const int *path = (const int *) (d + 10 + (dash ? 2 + d[11] : 0));

  int divide = 0;

  /*
    Decide whether the path can be represented by a single SVG <path>:

      * the start and end caps must be the same
      * they must not use triangles

    but it doesn't matter if the outline is transparent or thin.
  */
  divide = otr != 0 && d[8] != 0 &&
    (scap != ecap || scap == 3 || ecap == 3);

  if (otr != 0 || ftr != 0) {
    if (divide) {
      output(ws, false, "<g>\n");
      ws->indent += 2;

#if false
      output(ws, false, "<!-- RISC OS Drawfile path without caps -->\n");
#endif
    }

    output(ws, false, "<path style='");
    ws->indent += 13;


    if (otr == 0) {
      output(ws, false, "stroke: none;\n");
    } else {
      output(ws, false, "stroke: #%02X%02X%02X;\n",
              (d[7]>>8)&0xff,
              (d[7]>>16)&0xff,
              (d[7]>>24)&0xff);
      if (otr != 255) {
        output(ws, false, "stroke-opacity: %g;\n", otr/255.0);
      }
      if ((divide ? 0 : scap) != 0) {
        output(ws, false, "stroke-linecap: %s;\n",
                cap_str[divide ? 0 : scap]);
      }
      if (join != 0) {
        output(ws, false, "stroke-linejoin: %s;\n", join_str[join]);
      }

      if (dash) {
        int i;
        output(ws, false, "stroke-dasharray:");
        for (i = 0; i < d[11]; i++)
          fprintf(ws->out, "%s %d", i ? "," : "", d[12 + i]);
        output(ws, false, ";\n");
        output(ws, false, "stroke-dashoffset: %d;\n", d[10]);
      } else {
#if false
        output(ws, false, "stroke-dasharray: none;\n");
#endif
      }

      output(ws, false, "stroke-width: ");
      if (d[8])
        output(ws, false, "%d;\n", d[8]);
      else
        output(ws, false, "%g;\n", ws->ct->thin);
    }

    if (ftr == 0) {
      output(ws, false, "fill: none;'\n");
    } else {
      if (wind != 1) {
        output(ws, false, "fill-rule: %s;\n", wind_str[wind]);
      }

      if (ftr != 255) {
        output(ws, false, "fill-opacity: %g;\n", ftr/255.0);
      }

      output(ws, false, "fill: #%02X%02X%02X;'\n",
              (d[6]>>8)&0xff,
              (d[6]>>16)&0xff,
              (d[6]>>24)&0xff);
    }

    ws->indent -= 7;

    output(ws, false, "d='");
    ws->indent += 3;
    plot_path(ws, path, d + (d[1] >> 2));
    ws->indent -= 9;
    output(ws, false, "' />\n");
  }

  if (divide) {
    /* Draw the caps manually. */

    size_t wssize = 8;
    int *thick;
    unsigned int ctw = (d[9]>>16)&0xff;
    unsigned int ctl = (d[9]>>24)&0xff;
    unsigned char caj[16] = { 0 };

    caj[0] = join;
    caj[1] = 0;
    caj[2] = 0;

    caj[6] = 2;

    caj[12] = caj[8] = ctw<<4;
    caj[13] = caj[9] = ctw>>4;
    caj[14] = caj[10] = ctl<<4;
    caj[15] = caj[11] = ctl>>4;

    /* Calculate workspace required for path thickening. */
    _swi(Draw_StrokePath, _INR(0,6) | _OUT(0), (int) path, 0,
         0, 120, 0, (int) caj,
         (int) (dash ? d + 10 : 0), (int *) &wssize);

    /* Allocate workspace. */
    thick = ws->buf = realloc(ws->buf, wssize);
    if (!ws->buf) {
      printf("Out of memory\n");
      abort();
    }
    thick[0] = 0;
    thick[1] = wssize;

    /* Thicken the path into the workspace. */
    _swi(Draw_StrokePath, _INR(0,6), (int) path, (int) ws->buf,
         0, 120, 0, (int) caj,
         (int) (dash ? d + 10 : 0));

    /* Plot the thickened outline. */
#if false
    output(ws, false, "<!-- RISC OS Drawfile caps -->\n");
#endif
    output(ws, false, "<path style='");
    ws->indent += 13;

    output(ws, false, "stroke: none;\n");

    output(ws, false, "fill: #%02X%02X%02X;\n",
            (d[7]>>8)&0xff,
            (d[7]>>16)&0xff,
            (d[7]>>24)&0xff);

    if (otr != 255) {
      output(ws, false, "fill-opacity: %g;\n", otr/255.0);
    }

    output(ws, false, "fill-rule: nonzero;'\n");

    ws->indent -= 7;

    output(ws, false, "d='");
    ws->indent += 3;
    plot_caps(ws, thick, thick + (wssize >> 2),
              scap, ecap, d[8], ctw, ctl);
    ws->indent -= 9;
    output(ws, false, "' />\n");

    if (otr != 0 || ftr != 0) {
      ws->indent -= 2;
      output(ws, false, "</g>\n");
    }
  }
}

void convert(struct ws *ws, const int *d)
{
  switch (d[0]) {
  case 0: {
    const char *end = (const char *) ((const char *) d + d[1]);
    const char *pos = (char *) (d + 2);
    while (pos < end && pos[0]) {
#if false
      printf("%*sFont %d is %s\n", ws->indent, "", pos[0], pos + 1);
#endif
      ws->font[(int) pos[0]] = pos + 1;
      pos++;
      while (*(pos++))
        ;
    }
  } break;
  case 5:
  case 13:
#if false
    printf("%*sWe've got a sprite...\n", ws->indent, "");
#endif
    convert_sprite(ws, d);
    break;
  case 1:
  case 12:
#if false
    printf("%*sWe've got some text...\n", ws->indent, "");
#endif
    if (ws->ct->text_to_path) {
      convert_text_path(ws, d);
    } else {
    convert_text(ws, d);
    }
    break;
  case 2:
#if false
    printf("%*sWe've got a path...\n", ws->indent, "");
#endif
    convert_path(ws, d);
    break;
  case 6:
#if false
    printf("%*sWe've got a group...\n", ws->indent, "");
#endif
    convert_group(ws, d);
#if false
    printf("%*sGroup ended\n", ws->indent, "");
#endif
    break;
  default:
#if false
    printf("%*sWe've got type %d...\n", ws->indent, "", (int) d[0]);
#endif
    break;
  }
}

struct point {
  double x, y;
};

struct rect {
  struct point min, max;
};

static double scale(double orig, double mid, double factor)
{
  return (orig - mid) * factor + mid;
}

extern const char *const version;

int process(struct context *ctp)
{
  struct rect viewbox, natsize;
  struct ws ws;

  int *drawfile;
  size_t drawlen;
  int type;

#if false
  printf("Draw-to-SVG converter %s %s\n", __DATE__, __TIME__);
#endif

  if (get_file_type_and_length(ctp->iname, &type, &drawlen) != 1) {
    fprintf(stderr, "Not a file: %s\n", ctp->iname);
    return -1;
  }

  if (ctp->itype && type != 0xaff) {
    fprintf(stderr, "Not a drawfile: %s\n", ctp->iname);
    return -1;
  }

  drawfile = malloc((drawlen + 3) & ~3);
  if (!drawfile) {
    fprintf(stderr, "Out of memory\n");
    return -1;
  }

  if (load_file(ctp->iname, drawfile) < 0) {
    free(drawfile);
    fprintf(stderr, "Error loading %s\n", ctp->iname);
    return -1;
  }

  ws.out = fopen(ctp->oname, "w");
  if (!ws.out) {
    free(drawfile);
    fprintf(stderr, "Error opening %s\n", ctp->oname);
    return -1;
  }

  ws.ct = ctp;
  ws.buf = NULL;
  ws.indent = 0;
  ws.cpos = -1;
  ws.bbox = drawfile + 6;
  for (size_t i = 0; i < sizeof ws.font / sizeof ws.font[0]; i++)
    ws.font[i] = "System.Fixed";

  output(&ws, false, "<?xml version='1.0' encoding='iso-8859-1' "
         "standalone='no' ?>\n");
  output(&ws, false, "<!-- Generated from by Draw2SVG %s (%s) -->\n",
         linkversion, linkdate);
  output(&ws, false, "<!DOCTYPE svg PUBLIC\n");
  output(&ws, false, " '-//W3C//DTD SVG 20000303 Stylable//EN'\n");
  output(&ws, false, " 'http://www.w3.org/TR/2000/03/WD-SVG-20000303/"
                  "DTD/svg-20000303-stylable.dtd'>\n");

  /* Get the natural size of the file in draw-units. */
  viewbox.min.x = (double) drawfile[6];
  viewbox.min.y = (double) -drawfile[9];
  viewbox.max.x = (double) drawfile[8];
  viewbox.max.y = (double) -drawfile[7];

  /* Normalise scaling to SCALE_FACTOR. */
  switch (ctp->scaletype) {
  default:
    break;
  case SCALE_WIDTH:
    ctp->scale.factor.x = ctp->scale.factor.y =
      ctp->scale.fit.width / (viewbox.max.x - viewbox.min.x);
    break;
  case SCALE_HEIGHT:
    ctp->scale.factor.x = ctp->scale.factor.y =
      ctp->scale.fit.height / (viewbox.max.y - viewbox.min.y);
    break;
  case SCALE_FIT:
    ctp->scale.factor.x =
      ctp->scale.fit.width / (viewbox.max.x - viewbox.min.x);
    ctp->scale.factor.y =
      ctp->scale.fit.height / (viewbox.max.y - viewbox.min.y);
    break;
  }

  /* Now add the margin (which isn't scaled). */
  viewbox.min.x -= ctp->margin.width / ctp->scale.factor.x;
  viewbox.min.y -= ctp->margin.height / ctp->scale.factor.y;
  viewbox.max.x += ctp->margin.width / ctp->scale.factor.x;
  viewbox.max.y += ctp->margin.height / ctp->scale.factor.y;

  /* Work out the new natural size in proper units. */
  natsize.min.x = ctp->u->d2u(viewbox.min.x);
  natsize.min.y = ctp->u->d2u(viewbox.min.y);
  natsize.max.x = ctp->u->d2u(viewbox.max.x);
  natsize.max.y = ctp->u->d2u(viewbox.max.y);

  /* Now scale according to context. */
  {
    double midx = (natsize.min.x + natsize.max.x) / 2.0;
    double midy = (natsize.min.y + natsize.max.y) / 2.0;

    natsize.min.x = scale(natsize.min.x, midx, ctp->scale.factor.x);
    natsize.min.y = scale(natsize.min.y, midy, ctp->scale.factor.y);
    natsize.max.x = scale(natsize.max.x, midx, ctp->scale.factor.x);
    natsize.max.y = scale(natsize.max.y, midy, ctp->scale.factor.y);
  }

  double wpc, hpc;
  {
    double w = natsize.max.x - natsize.min.x;
    double h = natsize.max.y - natsize.min.y;
    if (w > h) {
      wpc = 100.0;
      hpc = h / w * 100.0;
    } else {
      hpc = 100.0;
      wpc = w / h * 100.0;
    }
  }

  if (ctp->abssized != SIZE_ABS)
    output(&ws, false, "<!-- width='%g%s' height='%g%s' -->\n",
           natsize.max.x - natsize.min.x, ctp->u->t,
           natsize.max.y - natsize.min.y, ctp->u->t);
  if (ctp->abssized != SIZE_PERCENT)
    output(&ws, false, "<!-- width='%g%%' height='%g%%' -->\n", wpc, hpc);

  output(&ws, false, "<svg "); 
  output(&ws, false, "xmlns='http://www.w3.org/2000/svg'\n     ");
  output(&ws, false, "xmlns:xlink='http://www.w3.org/1999/xlink'\n");

  if (ctp->topxy)
    output(&ws, false, "     x='%g%s' y='%g%s'\n",
           natsize.min.x, ctp->u->t, natsize.min.y, ctp->u->t);
  if (ctp->abssized == SIZE_ABS) {
    output(&ws, false, "     width='%g%s' height='%g%s'\n",
           natsize.max.x - natsize.min.x, ctp->u->t,
           natsize.max.y - natsize.min.y, ctp->u->t);
  } else if (ctp->abssized == SIZE_PERCENT) {
    output(&ws, false, "     width='%g%%' height='%g%%'\n", wpc, hpc);
  }
  if (ctp->partype != PAR_MEET ||
      ctp->parx != PAR_MID || ctp->pary != PAR_MID) {
    output(&ws, false, "     preserveAspectRatio='");
    if (ctp->partype == PAR_NONE) {
      output(&ws, false, "none");
    } else {
      static const char *const text[3] = { "Min", "Mid", "Max" };
      output(&ws, false, "x%sY%s", text[ctp->parx], text[ctp->pary]);
      if (ctp->partype != PAR_MEET) {
        output(&ws, false, " slice");
      }
    }
    output(&ws, false, "'\n");
  }

#ifndef STYLE_IN_GROUP
  output(&ws, false, "     style='fill-rule: evenodd;\n");
  ws.indent += 12;
  output(&ws, false, "fill-opacity: 1;\n");
  output(&ws, false, "stroke-dasharray: none;\n");
  output(&ws, false, "stroke-linejoin: miter;\n");
  output(&ws, false, "stroke-linecap: butt;\n");
  output(&ws, false, "stroke-opacity: 1;'\n");
  ws.indent -= 12;
#endif
  output(&ws, false, "     viewBox='%g %g %g %g'>\n",
         viewbox.min.x, viewbox.min.y,
         viewbox.max.x - viewbox.min.x,
         viewbox.max.y - viewbox.min.y);

  ws.indent += 2;
  if (ctp->bgcol) {
    output(&ws, false,
           "<rect style='fill: %s; stroke: none;'\n", ctp->bgcol);
    ws.indent += 6;
    output(&ws, false, "x='%g' y='%g' width='%g' height='%g' />\n",
           viewbox.min.x, viewbox.min.y,
           viewbox.max.x - viewbox.min.x,
           viewbox.max.y - viewbox.min.y);
    ws.indent -= 6;
  }

#ifdef STYLE_IN_GROUP
  output(&ws, false, "<g style='fill-rule: evenodd;\n");
  ws.indent += 10;
  output(&ws, false, "fill-opacity: 1;\n");
  output(&ws, false, "stroke-dasharray: none;\n");
  output(&ws, false, "stroke-linejoin: miter;\n");
  output(&ws, false, "stroke-linecap: butt;\n");
  output(&ws, false, "stroke-opacity: 1;'>\n");
  ws.indent -= 10;
  ws.indent += 2;
#endif

  convert_list(&ws, drawfile + 10, drawfile + (drawlen >> 2));
#ifdef STYLE_IN_GROUP
  ws.indent -= 2;
  output(&ws, false, "</g>\n");
#endif

  ws.indent -= 2;
  output(&ws, false, "</svg>\n");

  fclose(ws.out);
  free(ws.buf);

  if (ctp->otype)
    set_file_type(ctp->oname, 0xaad);

  return 0;
}
