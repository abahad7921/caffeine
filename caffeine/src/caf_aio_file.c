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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/uio.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <aio.h>

#include "caf/caf.h"
#include "caf/caf_data_mem.h"
#include "caf/caf_data_buffer.h"
#include "caf/caf_io_file.h"
#include "caf/caf_aio_file.h"


caf_aio_file_t *
caf_aio_fopen (const char *path, const int flg, const mode_t md,
			   int fs, size_t bsz) {
	caf_aio_file_t *r = (caf_aio_file_t *)NULL;
	if (path != (char *)NULL) {
		r = (caf_aio_file_t *)xmalloc (CAF_IO_FILE_SZ);
		if (r != (caf_aio_file_t *)NULL) {
			if (md != 0) {
				r->iocb.aio_fildes = open (path, flg, md);
			} else {
				r->iocb.aio_fildes = open (path, flg);
			}
			if (r->iocb.aio_fildes >= 0) {
				if (fs == CAF_OK) {
					r->flags = flg;
					r->mode = md;
					r->path = strdup (path);
					r->ustat = fs;
					if (fs == CAF_OK) {
						if ((fstat (r->iocb.aio_fildes, &(r->sd))) == 0) {
							return r;
						} else {
							close (r->iocb.aio_fildes);
							xfree (r->path);
							xfree (r);
							r = (caf_aio_file_t *)NULL;
						}
					}
				}
				r->buf = cbuf_create (bsz);
				if (r->buf != (cbuffer_t *)NULL) {
					cbuf_clean (r->buf);
					r->iocb.aio_buf = r->buf->data;
					r->iocb.aio_nbytes = r->buf->sz;
					r->iocb.aio_offset = 0;
					CAF_AIO_CLEAN(r);
				} else {
					close (r->iocb.aio_fildes);
					xfree (r->path);
					xfree (r);
					r = (caf_aio_file_t *)NULL;
				}
			} else {
				xfree (r);
				r = (caf_aio_file_t *)NULL;
			}
		}
	}
	return r;
}


int
caf_aio_fclose (caf_aio_file_t *r) {
	if (r != (caf_aio_file_t *)NULL) {
		if ((close (r->iocb.aio_fildes)) == 0) {
			cbuf_delete (r->buf);
			xfree (r->path);
			xfree (r);
			return CAF_OK;
		}
	}
	return CAF_ERROR;
}


caf_aio_file_t *
caf_aio_reopen (caf_aio_file_t *r) {
	caf_aio_file_t *r2 = (caf_aio_file_t *)NULL;
	if (r != (caf_aio_file_t *)NULL) {
		r2 = caf_aio_fopen (r->path, r->flags, r->mode, r->ustat, r->buf->sz);
		if (r2 != (caf_aio_file_t *)NULL) {
			if ((caf_aio_fclose (r)) == CAF_OK) {
				return r2;
			} else {
				caf_aio_fclose (r2);
				r2 = (caf_aio_file_t *)NULL;
			}
		}
	}
	return r2;
}


int
caf_aio_restat (caf_aio_file_t *r) {
	struct stat sb;
	if (r != (caf_aio_file_t *)NULL) {
		if ((fstat (r->fd, &sb)) == 0) {
			r->sd = sb;
			return CAF_OK;
		}
	}
	return CAF_ERROR;
}


int
caf_aio_fchanged (caf_aio_file_t *r, struct timespec *lmt, struct timespec *lct) {
	time_t diff;
	if (r != (caf_aio_file_t *)NULL) {
		diff = (r->sd.st_mtime - lmt->tv_sec) + (r->sd.st_ctime - lct->tv_sec);
		return (int)diff;
	}
	return CAF_OK;
}


int
caf_aio_read (caf_aio_file_t *r, cbuffer_t *b) {
	int rd = -1;
	if (r != (caf_aio_file_t *)NULL && b != (cbuffer_t *)NULL) {
		if (r->fd >= 0 && (b->sz > 0 || b->iosz > 0)) {
			rd = aio_read (&(r->iocb));
		}
	}
	return rd;
}


int
caf_aio_write (caf_aio_file_t *r, cbuffer_t *b) {
	int wr = -1;
	if (r != (caf_aio_file_t *)NULL && b != (cbuffer_t *)NULL) {
		if ((b->iosz > 0 || b->sz > 0) && r->fd >= 0) {
			wr = aio_write (&(r->iocb));
		}
	}
	return wr;
}


int
caf_aio_fcntl (caf_aio_file_t *r, int cmd, int *arg) {
	if (r != (caf_aio_file_t *)NULL) {
		if (arg != (int *)NULL) {
			return fcntl (r->fd, cmd, *arg);
		} else {
			return fcntl (r->fd, cmd);
		}
	}
	return CAF_ERROR_SUB;
}


int
caf_aio_flseek (caf_aio_file_t *r, off_t o, int w) {
	if (r != (caf_aio_file_t *)NULL) {
		if ((lseek (r->fd, o, w)) > -1) {
			return CAF_OK;
		}
	}
	return CAF_ERROR;
}


int
caf_aio_cancel (caf_aio_file_t *r) {
	if (r != (caf_aio_file_t *)NULL) {
		return aio_cancel (r->iocb.aio_fildes, &(r->iocb));
	}
	return AIO_ALLDONE;
}


int
caf_aio_return (caf_aio_file_t *r) {
	if (r != (caf_aio_file_t *)NULL) {
		return aio_return (&(r->iocb));
	}
	return -1;
}


int
caf_aio_error (caf_aio_file_t *r) {
	if (r != (caf_aio_file_t *)NULL) {
		return aio_error (&(r->iocb));
	}
	return -1;
}


int
caf_aio_suspend (caf_aio_file_t *r, const struct timespec *to) {
	struct aiocb *l_iocb[1];
	if (r != (caf_aio_file_t *)NULL) {
		l_iocb[0] = &(r->iocb);
		return aio_suspend ((const struct aiocb * const *)l_iocb, 1, to);
	}
	return -1;
}


caf_aio_file_lst_t *
caf_aio_lst_new (const int flg, const mode_t md, int fs, int count) {
	size_t tsz;
	caf_aio_file_lst_t *r = (caf_aio_file_lst_t *)NULL;
	int c;
	if (count > 0) {
		tsz = CAF_AIO_CTRLB_SZ * (size_t)count;
		r = (caf_aio_file_lst_t *)xmalloc (tsz);
		if (r != (caf_aio_file_lst_t *)NULL) {
			memset (r, 0, tsz);
			r->flags = flg;
			r->ustat = fs;
			r->mode = md;
			r->io_lst = lstdl_create ();
		}
	}
	return r;
}


int
caf_aio_lst_delete (caf_aio_file_lst_t *r) {
	if (r != (caf_aio_file_lst_t *)NULL) {
		xfree (r);
		return CAF_OK;
	}
	return CAF_ERROR;
}


int
caf_aio_lst_open (caf_aio_file_lst_t *r, const char paths[]) {
	int ofc = 0;
	if (r != (caf_aio_file_lst_t *)NULL && paths != (char *)NULL) {
	}
	return ofc;
}


/* caf_aio_core.c ends here */
