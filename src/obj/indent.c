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

#include <stdarg.h>
#include <stdio.h>
#include <ctype.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "context.h"

int findent(FILE *fp, int hm)
{
  char sp[LINE_WIDTH + 1], *ptr = sp;
  while (hm > 0 && ptr < sp + (sizeof sp - 1)) {
    if (hm >= 8)
      *ptr++ = '\t', hm -= 8;
    else
      *ptr++ = ' ', hm--;
  }
  *ptr = '\0';
  return fprintf(fp, "%s", sp);
}

void indent(struct ws *ws)
{
  int hm = ws->indent;

  while (hm > 0) {
    if (hm >= 8)
      fputc('\t', ws->out), hm -= 8;
    else
      fputc(' ', ws->out), hm--;
  }
}

void cindent(struct ws *ws, int *spp, int req)
{
  if (*spp < req) {
    if (ws->indent >= 36) {
      *spp = LINE_WIDTH - 2;
      fprintf(ws->out, "\n  ");
    } else {
      *spp = LINE_WIDTH - ws->indent;
      fputc('\n', ws->out);
      indent(ws);
    }
  }
}

int output(struct ws *ws, int pretty, const char *fmt, ...)
{
  va_list ap;
  int rc = 0;
  const char *ptr;
  static char line[200];

  va_start(ap, fmt);
  vsprintf(line, fmt, ap);
  va_end(ap);

  ptr = line;
  while (*ptr) {
    int npos, opos, mpos;
    const char *start, *nosp;

    if (ws->cpos < 0) {
      rc += findent(ws->out, ws->indent);
      ws->cpos = ws->indent;
    }

    opos = ws->cpos;
    for (start = ptr;
         *ptr && (*ptr == ' ' || *ptr == '\t');
         ptr++)
      ws->cpos += (*ptr == '\t' ? 8 - ws->cpos % 8 : 1);
    if (*ptr == '\n') {
      putc('\n', ws->out);
      rc++;
      ws->cpos = -1;
      ptr++;
      continue;
    }
    mpos = ws->cpos;

    for (nosp = ptr; *ptr && !isspace(*ptr); ptr++)
      ws->cpos++;

    if (pretty && ws->cpos > LINE_WIDTH) {
      if (opos > 0) putc('\n', ws->out), rc++;
      if (ws->cpos - mpos > LINE_WIDTH) {
        npos = LINE_WIDTH - (ws->cpos - mpos);
        if (npos < 0) npos = 0;
      } else {
        npos = ws->indent;
      }
      start = nosp;
      rc += findent(ws->out, npos);
      ws->cpos = npos + (ptr - start);
    }
    rc += fprintf(ws->out, "%.*s", (int) (ptr - start), start);
  }
  return rc;
}

int output_esc(struct ws *ws, unsigned flags, const char *fmt, ...)
{
  static size_t buflen = 0;
  static char *buf = NULL;

  va_list ap;
  int rc;
  char special[10] = "";
  int pretty = !!(flags & OUT_PRETTY);

  {
    char *p = special;
    if (flags & OUT_ESCAMP)
      *p++ = '&';
    if (flags & OUT_ESCLT)
      *p++ = '<';
    if (flags & OUT_ESCGT)
    *p++ = '>';
    if (flags & OUT_ESCQUOT)
      *p++ = '"';
    if (flags & OUT_ESCAPOS)
      *p++ = '\'';
    *p = '\0';
  }

  va_start(ap, fmt);
  rc = vsnprintf(buf, buflen, fmt, ap);
  va_end(ap);

  if (rc < 0)
    return rc;

  if (!buf || (unsigned) rc >= buflen) {
    /* There was insufficient space in the current.  Re-allocate it to
       be big enough. */
    size_t nbl = rc + 1 + buflen / 2;
    void *nb = realloc(buf, nbl);
    if (!nb) return -1;
    buflen = nbl;
    buf = nb;

    va_start(ap, fmt);
    rc = vsnprintf(buf, buflen, fmt, ap);
    va_end(ap);

    if (rc < 0)
      return -1;

    assert((unsigned) rc < buflen);
  }

  rc = 0;
  for (const char *pos = buf; *pos; pos++) {
    /* Print out the largest prefix that doesn't need escaping, and
       discard it. */
    size_t n = strcspn(pos, special);
    rc += output(ws, pretty, "%.*s", (int) n, pos);
    pos += n;
    if (!*pos) break;

    /* Escape the current character. */
    switch (*pos) {
    case '&':
      rc += output(ws, pretty, "&amp;");
      break;
    case '<':
      rc += output(ws, pretty, "&lt;");
      break;
    case '>':
      rc += output(ws, pretty, "&gt;");
      break;
    case '"':
      rc += output(ws, pretty, "&quot;");
      break;
    case '\'':
      rc += output(ws, pretty, "&apos;");
      break;
    default:
      assert(false);
      break;
    }
  }

  return rc;
}
