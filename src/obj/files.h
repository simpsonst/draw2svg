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

#ifndef FILES_H
#define FILES_H

#include <stddef.h>

int get_file_type_and_length(const char *s, int *ftp, size_t *st);
int set_file_type(const char *s, int ft);
int load_file(const char *s, void *b);

#endif
