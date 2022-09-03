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

#include <stdio.h>

#include <kernel.h>
#include <swis.h>

#include <riscos/swi/OS.h>

#include "files.h"

int get_file_type_and_length(const char *s, int *ftp, size_t *st)
{
  int t, type, len;

  _swi(OS_File, _INR(0,1)|_OUT(0)|_OUT(2)|_OUT(4),
       17, s, &t, &type, &len);
  if (t == 1) {
    *st = len;
    if ((type & 0xfff00000) == 0xfff00000)
      *ftp = (unsigned) type >> 8 & 0xfff;
    else
      *ftp = -1;
  }
  return t;
}

int set_file_type(const char *s, int ft)
{
  _swi(OS_File, _INR(0,2), 18, s, ft);
  return 0;
}

#if 0
int get_file_length(const char *s, size_t *st)
{
  int t;
  int len;
  _swi(OS_File, _INR(0,1)|_OUT(0)|_OUT(4), 17, s, &t, &len);
  if (t != 1)
    return -1;
  *st = len;
  return 0;
}
#endif

int load_file(const char *s, void *b)
{
  _swi(OS_File, _INR(0,3), 16, s, b, 0);
  return 0;
}
