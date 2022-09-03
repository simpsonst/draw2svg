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
*/

#include <string.h>
#include <stdio.h>

#include "units.h"

#define FACTOR_in (180.0 * 256.0)
#define FACTOR_pt 640.0
#define FACTOR_mm (FACTOR_in * 0.0394)
#define FACTOR_cm (FACTOR_mm * 10.0)

static double draw2in(double x) { return x / FACTOR_in; }
static double draw2pt(double x) { return x / FACTOR_pt; }
static double draw2mm(double x) { return x / FACTOR_mm; }
static double draw2cm(double x) { return x / FACTOR_cm; }
static double draw2du(double x) { return x; }

static double in2draw(double x) { return x * FACTOR_in; }
static double pt2draw(double x) { return x * FACTOR_pt; }
static double mm2draw(double x) { return x * FACTOR_mm; }
static double cm2draw(double x) { return x * FACTOR_cm; }
static double du2draw(double x) { return x; }

static const struct unit units[] = {
  { "in", &draw2in, &in2draw },
  { "mm", &draw2mm, &mm2draw },
  { "cm", &draw2cm, &cm2draw },
  { "pt", &draw2pt, &pt2draw },
  { "", &draw2du, &du2draw }
};

const struct unit *choose_units(const char *u)
{
  size_t i;

  for (i = 0; i < sizeof units / sizeof units[0]; i++)
    if (!strcmp(u, units[i].t))
      return &units[i];
  return NULL;
}

int measure(const char *data, double *valp)
{
  double val;
  const struct unit *up;
  char unit[5];

  switch (sscanf(data, "%lf%4s", &val, unit)) {
  default:
    return -1;
  case 1:
    *valp = val;
    return 0;
  case 2:
    if (!(up = choose_units(unit)))
      return -1;
    *valp = (*up->u2d)(val);
    return 0;
  }
}
