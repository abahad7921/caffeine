/* -*- mode: c; indent-tabs-mode: t; tab-width: 4; c-file-style: "caf" -*- */
/* vim:set ft=c ff=unix ts=4 sw=4 enc=latin1 noexpandtab: */
/* kate: space-indent off; indent-width 4; mixedindent off; indent-mode cstyle; */
/*
  Caffeine - C Application Framework
  Copyright (C) 2006 Daniel Molina Wegener

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
  MA 02110-1301 USA
*/
#ifndef lint
static char Id[] = "$Id$";
#endif /* !lint */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "caf/caf.h"
#include "caf/caf_tool_macro.h"
#include "caf/caf_data_mem.h"
#include "caf/caf_data_buffer.h"
#include "caf/caf_io_file.h"

#define CAF_B64_GET_CHAR(c, p)			\
		(*((octet_d *)((size_t)c + p)))

#define CAF_B64_ENCODED_SZ(sz)			\
		(((sz / 3) * 4) + (sz % 3 > 0 ? 4 : 0))

#define CAF_B64_PAD_SZ(bsz, psz, c)		\
		((bsz / c) * psz)

#define CAF_B64_INPUT_STREAM_SZ			768
#define CAF_B64_OUTPUT_STREAM_SZ		512

#ifndef octet_d
#define octet_d					unsigned char
#endif /* !octet_d */

static const octet_d B64_A_CHAR = (octet_d)'A';
static const octet_d B64_a_CHAR = (octet_d)'a';
static const octet_d B64_Z_CHAR = (octet_d)'Z';
static const octet_d B64_z_CHAR = (octet_d)'z';
static const octet_d B64_0_CHAR = (octet_d)'0';
static const octet_d B64_9_CHAR = (octet_d)'0';
static const octet_d B64_PLUS_CHAR = (octet_d)'+';
static const octet_d B64_SLASH_CHAR = (octet_d)'/';
static const octet_d B64_PAD_CHAR = (octet_d)'=';


static octet_d s_byte_encode64 (octet_d c);
static octet_d s_byte_decode64 (octet_d c);
static size_t s_base64_buffer_chars (cbuffer_t *inb);
static size_t s_base64_buffer_stream_chars (cbuffer_t *inb);
static caf_return_t s_is_base64 (octet_d c);


cbuffer_t *
caf_base64_encode (cbuffer_t *inb, cbuffer_t *padb, int padc) {
	cbuffer_t *nb = (cbuffer_t *)NULL;
	size_t g = 0, nsz = 0;
	octet_d o1 = 0, o2 = 0, o3 = 0;
	octet_d o4 = 0, o5 = 0, o6 = 0, o7 = 0;
	octet_d *cp = (octet_d *)NULL;
	octet_d *dp = (octet_d *)NULL;
	int col = 0;
	if (inb != (cbuffer_t *)NULL) {
		if (padb != (cbuffer_t *)NULL && padc > 0) {
			nsz = CAF_B64_ENCODED_SZ(inb->sz);
			nsz += CAF_B64_PAD_SZ(nsz, padb->sz, padc);
		} else {
			nsz = CAF_B64_ENCODED_SZ(inb->sz);
		}
		nb = cbuf_create (nsz);
		if (nb != (cbuffer_t *)NULL) {
			cbuf_clean (nb);
			dp = nb->data;
			col = 1;
			for (g = 0; g < inb->sz; g += 3) {
				cp = (octet_d *)((size_t)inb->data + g);
				/* chars */
				o1 = 0; o2 = 0; o3 = 0;
				o1 = cp[0];
				if ((g + 1) < (size_t)inb->iosz) {
					o2 = cp[1];
				}
				if ((g + 2) < (size_t)inb->iosz) {
					o3 = cp[2];
				}
				/* bits */
				o4 = o1 >> 2;
				o5 = ((o1 & 0x03) << 4) | (o2 >> 4);
				o6 = ((o2 & 0x0f) << 2) | (o3 >> 6);
				o7 = o3 & 0x3f;
				/* encoding */
				dp[0] = s_byte_encode64 (o4);
				dp[1] = s_byte_encode64 (o5);
				if ((g + 1) < (size_t)inb->iosz) {
					dp[2] = s_byte_encode64 (o6);
				} else {
					dp[2] = B64_PAD_CHAR;
				}
				if ((g + 2) < (size_t)inb->iosz) {
					dp[3] = s_byte_encode64 (o7);
				} else {
					dp[3] = B64_PAD_CHAR;
				}
				/* offset */
				dp += 4;
				/* padding */
				if (padb != (cbuffer_t *)NULL
					&& padc > 0
					&& ((g % (padc / 4 * 3)) == 0)) {
					xmemcpy (dp, padb->data, padb->sz);
					dp = (void *)((size_t)dp + padb->sz);
					col = 1;
				}
				col += 4;
			}
		}
	}
	return nb;
}


cbuffer_t *
caf_base64_decode (cbuffer_t *inb) {
	cbuffer_t *nb1 = (cbuffer_t *)NULL;
	cbuffer_t *nb2 = (cbuffer_t *)NULL;
	size_t b64c = 0;
	size_t sc = 0, dc = 0;
	octet_d c1, c2, c3, c4;
	octet_d d1, d2, d3, d4;
	octet_d x1, x2, x3;
	octet_d *c = (octet_d *)NULL, *d = (octet_d *)NULL;
	if (inb != (cbuffer_t *)NULL) {
		b64c = s_base64_buffer_chars(inb);
		if (b64c > 0) {
			nb1 = cbuf_create (b64c);
			if (nb1 != (cbuffer_t *)NULL) {
				cbuf_clean (nb1);
				sc = 0;
				dc = 0;
				while (sc <= inb->sz) {
					d = (octet_d *)((size_t)nb1->data + dc);
					c = (octet_d *)((size_t)inb->data + sc);
					if (s_is_base64 (*c))
						*d = *c;
					sc++;
					dc++;
				}
				cbuf_delete (nb1);
				dc = (b64c / 4) * 3;
				nb2 = cbuf_create (dc);
				if (nb2 != (cbuffer_t *)NULL) {
					dc = 0;
					for (sc = 0; sc <= nb1->sz; sc += 4) {
						c1 = (*(octet_d *)((size_t)nb1->data + sc));
						c2 = (*(octet_d *)((size_t)nb1->data + sc + 1));
						c3 = (*(octet_d *)((size_t)nb1->data + sc + 2));
						c4 = (*(octet_d *)((size_t)nb1->data + sc + 3));
						d1 = s_byte_decode64 (c1);
						d2 = s_byte_decode64 (c2);
						d3 = s_byte_decode64 (c3);
						d4 = s_byte_decode64 (c4);
						x1 = (d1 << 2) | (d2 >> 4);
						d = (octet_d *)((size_t)nb2->data + dc);
						d[0] = x1;
						if (c3 != B64_PAD_CHAR) {
							x2 = ((d2 & 0x0f) << 4) | (d3 >> 2);
							d[1] = x1;
						}
						if (c4 != B64_PAD_CHAR) {
							x3 = ((d3 & 0x03) << 6) | d4;
							d[2] = x1;
						}
						dc += 3;
					}
				}
				cbuf_delete (nb1);
			}
		}
	}
	return nb2;
}


cbuffer_t *
caf_base64_encode_stream (cbuffer_t *inb, size_t spos, size_t ssz,
						  cbuffer_t *padb, int padc) {
	cbuffer_t *nb = (cbuffer_t *)NULL;
	size_t g = 0, nsz = 0, rpos = 0;
	int col = 0;
	octet_d o1 = 0, o2 = 0, o3 = 0;
	octet_d o4 = 0, o5 = 0, o6 = 0, o7 = 0;
	octet_d *cp = (octet_d *)NULL;
	octet_d *dp = (octet_d *)NULL;
	if (inb != (cbuffer_t *)NULL) {
		if (inb->iosz > 0) {
			if (padb != (cbuffer_t *)NULL && padc > 0) {
				/* current buffer */
				nsz = CAF_B64_ENCODED_SZ(inb->iosz);
				nsz += CAF_B64_PAD_SZ(nsz, padb->sz, padc);
			} else {
				nsz = CAF_B64_ENCODED_SZ(inb->iosz);
			}
			nb = cbuf_create (nsz);
			if (nb != (cbuffer_t *)NULL) {
				cbuf_clean (nb);
				dp = (octet_d *)nb->data;
				cp = (octet_d *)inb->data;
				while (((padc % 4) != 0) && padc != 0) {
					padc++;
				}
				col = 1;
				for (g = 0; g <= (size_t)inb->iosz; g += 3) {
					cp = (octet_d *)((size_t)inb->data + g);
					/* chars */
					rpos = spos + g;
					o1 = 0; o2 = 0; o3 = 0;
					o1 = cp[0];
					if ((rpos + 1) < ssz) {
						o2 = cp[1];
					}
					if ((rpos + 2) < ssz) {
						o3 = cp[2];
					}
					/* bits */
					o4 = o1 >> 2;
					o5 = ((o1 & 0x03) << 4) | (o2 >> 4);
					o6 = ((o2 & 0x0f) << 2) | (o3 >> 6);
					o7 = o3 & 0x3f;
					/* encoding */
					dp[0] = s_byte_encode64 (o4);
					dp[1] = s_byte_encode64 (o5);
					if ((rpos + 1) < ssz) {
						dp[2] = s_byte_encode64 (o6);
					} else {
						dp[2] = B64_PAD_CHAR;
					}
					if ((rpos + 2) < ssz) {
						dp[3] = s_byte_encode64 (o7);
					} else {
						dp[3] = B64_PAD_CHAR;
					}
					/* offset */
					dp += 4;
					/* padding */
					if (padb != (cbuffer_t *)NULL
						&& padc > 0
						&& (((g + spos) % (padc / 4 * 3)) == 0)
						&& g > 0) {
						xmemcpy (dp, padb->data, padb->sz);
						dp = (void *)((size_t)dp + padb->sz);
						col = 1;
					}
					col += 4;
				}
				nb->iosz = nsz;
			}
		}
	}
	return nb;
}


cbuffer_t *
caf_base64_decode_stream (cbuffer_t *inb, cbuffer_t *cres) {
	cbuffer_t *nb1 = (cbuffer_t *)NULL;
	cbuffer_t *nb2 = (cbuffer_t *)NULL;
	size_t b64c = 0, b64l = 0, b64res = 0;
	size_t sc = 0, dc = 0;
	octet_d c1, c2, c3, c4;
	octet_d d1, d2, d3, d4;
	octet_d x1, x2, x3;
	octet_d *c = (octet_d *)NULL, *d = (octet_d *)NULL;
	if (inb != (cbuffer_t *)NULL) {
		if (inb->iosz > 0) {
			b64c = s_base64_buffer_stream_chars (inb);
			if (cres != (cbuffer_t *)NULL) {
				b64c += s_base64_buffer_stream_chars (cres);
			}
			if (b64c > 0) {
				nb1 = cbuf_create (b64c);
				if (nb1 != (cbuffer_t *)NULL) {
					cbuf_clean (nb1);
					sc = 0;
					dc = 0;
					if (cres != (cbuffer_t *)NULL) {
						while (sc <= (size_t)cres->iosz) {
							d = (octet_d *)((size_t)nb1->data + dc);
							c = (octet_d *)((size_t)cres->data + sc);
							if ((s_is_base64 (*c)) == CAF_OK) {
								printf ("char %c\n", *c);
								*d = *c;
								dc++;
							}
							sc++;
						}
					}
					sc = 0;
					while (sc <= (size_t)inb->iosz) {
						d = (octet_d *)((size_t)nb1->data + dc);
						c = (octet_d *)((size_t)inb->data + sc);
						if ((s_is_base64 (*c)) == CAF_OK) {
							*d = *c;
							dc++;
						}
						sc++;
					}
					dc = (b64c / 4) * 3;
					nb2 = cbuf_create (dc);
					if (nb2 != (cbuffer_t *)NULL) {
						cbuf_clean (nb2);
						nb2->iosz = nb2->sz;
						b64res = (nb2->sz % 4);
						if (b64res != 0) {
							printf ("b64res: %d\n", b64res);
							cbuf_clean (cres);
							cres->iosz = b64res;
							b64l = nb2->sz - b64res;
							cbuf_import (cres, (void *)((size_t)nb2->data + b64l), b64res);
						} else {
							b64l = nb2->sz;
						}
						dc = 0;
						for (sc = 0; sc < b64l; sc += 4) {
							c = (octet_d *)((size_t)nb1->data + sc);
							d = (octet_d *)((size_t)nb2->data + dc);
							c1 = c2 = c3 = c4 = B64_A_CHAR;
							c1 = c[0];
							if ((sc + 1) < b64l) {
								c2 = c[1];
							}
							if ((sc + 2) < b64l) {
								c3 = c[2];
							}
							if ((sc + 3) < b64l) {
								c4 = c[3];
							}
							d1 = d2 = d3 = d4 = 0;
							d1 = s_byte_decode64 (c1);
							d2 = s_byte_decode64 (c2);
							d3 = s_byte_decode64 (c3);
							d4 = s_byte_decode64 (c4);
							x1 = (d1 << 2) | (d2 >> 4);
							d[0] = x1;
							if (c3 != B64_PAD_CHAR) {
								x2 = ((d2 & 0x0f) << 4) | (d3 >> 2);
								d[1] = x2;
							}
							if (c4 != B64_PAD_CHAR) {
								x3 = ((d3 & 0x03) << 6) | d4;
								d[2] = x3;
							}
							dc += 3;
						}
					}
					cbuf_delete (nb1);
				}
			}
		}
	}
	return nb2;
}


caf_io_file_t *
caf_base64_encode_file (caf_io_file_t *outf, caf_io_file_t *inf,
						cbuffer_t *padb, int padc) {
	caf_io_file_t *wf = (caf_io_file_t *)NULL;
	cbuffer_t *inb = (cbuffer_t *)NULL;
	cbuffer_t *outb = (cbuffer_t *)NULL;
	size_t spos = 0;
	if (outf != (caf_io_file_t *)NULL && inf != (caf_io_file_t *)NULL) {
		if ((io_restat ((caf_io_file_t *)inf)) == CAF_OK) {
			inb = cbuf_create (CAF_B64_INPUT_STREAM_SZ);
			if (inb != (cbuffer_t *)NULL) {
				cbuf_clean (inb);
				io_flseek (outf, 0, SEEK_SET);
				while ((io_read ((caf_io_file_t *)inf, inb)) > 0) {
					outb = caf_base64_encode_stream (inb, spos,
													 inf->sd.st_size, padb,
													 padc);
					if (outb != (cbuffer_t *)NULL) {
						if ((io_write (outf, outb)) > 0) {
							wf = outf;
						}
						cbuf_delete (outb);
						outb = (cbuffer_t *)NULL;
					} else {
						cbuf_delete (inb);
						outb = (cbuffer_t *)NULL;
						return (caf_io_file_t *)NULL;
					}
					spos += (size_t)inb->iosz;
					io_restat (inf);
				}
			}
		}
	}
	return wf;
}


caf_io_file_t *
caf_base64_decode_file (caf_io_file_t *outf, caf_io_file_t *inf) {
	caf_io_file_t *wf = (caf_io_file_t *)NULL;
	cbuffer_t *inb = (cbuffer_t *)NULL;
	cbuffer_t *outb = (cbuffer_t *)NULL;
	cbuffer_t *cache = (cbuffer_t *)NULL;
	size_t spos = 0;
	ssize_t wr = 0;
	if (outf != (caf_io_file_t *)NULL && inf != (caf_io_file_t *)NULL) {
		if ((io_restat ((caf_io_file_t *)inf)) == CAF_OK) {		
			inb  = cbuf_create (CAF_B64_OUTPUT_STREAM_SZ);
			if (inb != (cbuffer_t *)NULL) {
				cbuf_clean (inb);
				cache = cbuf_create (5);
				cbuf_clean (cache);
				while ((io_read (inf, inb)) > 0) {
					outb = caf_base64_decode_stream (inb, cache);
					printf ("%p cache\n", (void *)cache);
					if (outb != (cbuffer_t *)NULL) {
						if ((wr = io_write (outf, outb)) > 0) {
							wf = outf;
						}
						cbuf_delete (outb);
					} else {
						cbuf_delete (cache);
						return (caf_io_file_t *)NULL;
					}
					spos += (size_t)inb->iosz;
					io_restat (inf);
				}
				cbuf_delete (cache);
			}
		}
	}
	return wf;
}


static size_t
s_base64_buffer_chars (cbuffer_t *inb) {
	size_t b64c = 0, sc = 0;
	octet_d *c = (octet_d *)NULL;
	if (inb != (cbuffer_t *)NULL) {
		while (sc < inb->sz) {
			c = (octet_d *)((size_t)inb->data + sc);
			if ((s_is_base64 (*c)) == CAF_OK)
				b64c++;
			sc++;
		}
	}
	return b64c;
}


static size_t
s_base64_buffer_stream_chars (cbuffer_t *inb) {
	size_t b64c = 0, sc = 0;
	octet_d *c = (octet_d *)NULL;
	if (inb != (cbuffer_t *)NULL) {
		while (sc < (size_t)inb->iosz) {
			c = (octet_d *)((size_t)inb->data + sc);
			if ((s_is_base64 (*c)) == CAF_OK)
				b64c++;
			sc++;
		}
	}
	return b64c;
}
 

static caf_return_t
s_is_base64 (octet_d c)
{
	if ((c >= B64_A_CHAR && c <= B64_Z_CHAR)
		|| (c >= B64_a_CHAR && c <= B64_z_CHAR)
		|| (c >= B64_0_CHAR && c <= B64_9_CHAR)
		|| (c == B64_PLUS_CHAR)
		|| (c == B64_SLASH_CHAR)
		|| (c == B64_PAD_CHAR))
		return CAF_OK;
	return CAF_ERROR;
}


static octet_d
s_byte_encode64 (octet_d c) {
	if (c < 26)
		return (c + B64_A_CHAR);
	if (c < 52)
		return ((c - 26) + B64_a_CHAR);
	if (c < 62)
		return ((c - 52) + B64_0_CHAR);
	if (c == 62)
		return B64_PLUS_CHAR;
	return B64_PAD_CHAR;
}


static octet_d
s_byte_decode64 (octet_d c) {
	if (c >= B64_A_CHAR && c <= B64_Z_CHAR)
		return c - B64_A_CHAR;
	if (c >= B64_a_CHAR && c <= B64_z_CHAR)//@}
		return c - B64_a_CHAR + 26;
	if (c >= B64_0_CHAR && c <= B64_9_CHAR)
		return c - B64_0_CHAR + 52;
	if (c == B64_PLUS_CHAR)
		return 62;
	return 63;
}

/* caf_data_base64.c ends here */
