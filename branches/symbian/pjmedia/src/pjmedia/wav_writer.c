/* $Id$ */
/* 
 * Copyright (C) 2003-2006 Benny Prijono <benny@prijono.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA 
 */
#include <pjmedia/wav_port.h>
#include <pjmedia/errno.h>
#include <pjmedia/wave.h>
#include <pj/assert.h>
#include <pj/file_access.h>
#include <pj/file_io.h>
#include <pj/log.h>
#include <pj/pool.h>
#include <pj/string.h>


#define THIS_FILE	    "wav_writer.c"
#define SIGNATURE	    ('F'<<24|'W'<<16|'R'<<8|'T')
#define BYTES_PER_SAMPLE    2


struct file_port
{
    pjmedia_port     base;
    pj_size_t	     bufsize;
    char	    *buf;
    char	    *writepos;

    pj_oshandle_t    fd;
};

static pj_status_t file_put_frame(pjmedia_port *this_port, 
				  const pjmedia_frame *frame);
static pj_status_t file_get_frame(pjmedia_port *this_port, 
				  pjmedia_frame *frame);
static pj_status_t file_on_destroy(pjmedia_port *this_port);


/*
 * Create file writer port.
 */
PJ_DEF(pj_status_t) pjmedia_wav_writer_port_create( pj_pool_t *pool,
						     const char *filename,
						     unsigned sampling_rate,
						     unsigned channel_count,
						     unsigned samples_per_frame,
						     unsigned bits_per_sample,
						     unsigned flags,
						     pj_ssize_t buff_size,
						     void *user_data,
						     pjmedia_port **p_port )
{
    struct file_port *fport;
    pjmedia_wave_hdr wave_hdr;
    pj_ssize_t size;
    pj_status_t status;

    PJ_UNUSED_ARG(flags);
    PJ_UNUSED_ARG(user_data);

    /* Check arguments. */
    PJ_ASSERT_RETURN(pool && filename && p_port, PJ_EINVAL);

    /* Only supports 16bits per sample for now.
     * See flush_buffer().
     */
    PJ_ASSERT_RETURN(bits_per_sample == 16, PJ_EINVAL);

    /* Create file port instance. */
    fport = pj_pool_zalloc(pool, sizeof(struct file_port));
    PJ_ASSERT_RETURN(fport != NULL, PJ_ENOMEM);

    /* Initialize port info. */
    fport->base.info.bits_per_sample = bits_per_sample;
    fport->base.info.bytes_per_frame = samples_per_frame * bits_per_sample *
				       channel_count / 8;
    fport->base.info.channel_count = channel_count;
    fport->base.info.encoding_name = pj_str("pom");
    fport->base.info.has_info = 1;
    pj_strdup2(pool, &fport->base.info.name, filename);
    fport->base.info.need_info = 0;
    fport->base.info.pt = 0xFF;
    fport->base.info.clock_rate = sampling_rate;
    fport->base.info.samples_per_frame = samples_per_frame;
    fport->base.info.signature = SIGNATURE;
    fport->base.info.type = PJMEDIA_TYPE_AUDIO;
    
    fport->base.get_frame = &file_get_frame;
    fport->base.put_frame = &file_put_frame;
    fport->base.on_destroy = &file_on_destroy;


    /* Open file in write and read mode.
     * We need the read mode because we'll modify the WAVE header once
     * the recording has completed.
     */
    status = pj_file_open(pool, filename, PJ_O_WRONLY, &fport->fd);
    if (status != PJ_SUCCESS)
	return status;

    /* Initialize WAVE header */
    pj_memset(&wave_hdr, 0, sizeof(pjmedia_wave_hdr));
    wave_hdr.riff_hdr.riff = PJMEDIA_RIFF_TAG;
    wave_hdr.riff_hdr.file_len = 0; /* will be filled later */
    wave_hdr.riff_hdr.wave = PJMEDIA_WAVE_TAG;

    wave_hdr.fmt_hdr.fmt = PJMEDIA_FMT_TAG;
    wave_hdr.fmt_hdr.len = 16;
    wave_hdr.fmt_hdr.fmt_tag = 1;
    wave_hdr.fmt_hdr.nchan = (pj_int16_t)channel_count;
    wave_hdr.fmt_hdr.sample_rate = sampling_rate;
    wave_hdr.fmt_hdr.bytes_per_sec = sampling_rate * channel_count * 
				     bits_per_sample / 8;
    wave_hdr.fmt_hdr.block_align = (pj_int16_t) (channel_count * 
						 bits_per_sample / 8);
    wave_hdr.fmt_hdr.bits_per_sample = (pj_int16_t)bits_per_sample;

    wave_hdr.data_hdr.data = PJMEDIA_DATA_TAG;
    wave_hdr.data_hdr.len = 0;	    /* will be filled later */


    /* Convert WAVE header from host byte order to little endian
     * before writing the header.
     */
    pjmedia_wave_hdr_host_to_file(&wave_hdr);


    /* Write WAVE header */
    size = sizeof(pjmedia_wave_hdr);
    status = pj_file_write(fport->fd, &wave_hdr, &size);
    if (status != PJ_SUCCESS) {
	pj_file_close(fport->fd);
	return status;
    }

    /* Set buffer size. */
    if (buff_size < 1) buff_size = PJMEDIA_FILE_PORT_BUFSIZE;
    fport->bufsize = buff_size;

    /* Check that buffer size is greater than bytes per frame */
    pj_assert(fport->bufsize >= fport->base.info.bytes_per_frame);


    /* Allocate buffer and set initial write position */
    fport->buf = pj_pool_alloc(pool, fport->bufsize);
    if (fport->buf == NULL) {
	pj_file_close(fport->fd);
	return PJ_ENOMEM;
    }
    fport->writepos = fport->buf;

    /* Done. */
    *p_port = &fport->base;

    PJ_LOG(4,(THIS_FILE, 
	      "File writer '%.*s' created: samp.rate=%d, bufsize=%uKB",
	      (int)fport->base.info.name.slen,
	      fport->base.info.name.ptr,
	      fport->base.info.clock_rate,
	      fport->bufsize / 1000));


    return PJ_SUCCESS;
}


#if defined(PJ_IS_BIG_ENDIAN) && PJ_IS_BIG_ENDIAN!=0
    static void swap_samples(pj_int16_t *samples, unsigned count)
    {
	unsigned i;
	for (i=0; i<count; ++i) {
	    samples[i] = pj_swap16(samples[i]);
	}
    }
#else
#   define swap_samples(samples,count)
#endif

/*
 * Flush the contents of the buffer to the file.
 */
static pj_status_t flush_buffer(struct file_port *fport)
{
    pj_ssize_t bytes = fport->writepos - fport->buf;
    pj_status_t status;

    /* Convert samples to little endian */
    swap_samples((pj_int16_t*)fport->buf, bytes/BYTES_PER_SAMPLE);

    /* Write to file. */
    status = pj_file_write(fport->fd, fport->buf, &bytes);

    /* Reset writepos */
    fport->writepos = fport->buf;

    return status;
}

/*
 * Put a frame into the buffer. When the buffer is full, flush the buffer
 * to the file.
 */
static pj_status_t file_put_frame(pjmedia_port *this_port, 
				  const pjmedia_frame *frame)
{
    struct file_port *fport = (struct file_port *)this_port;

    /* Flush buffer if we don't have enough room for the frame. */
    if (fport->writepos + frame->size > fport->buf + fport->bufsize) {
	pj_status_t status;
	status = flush_buffer(fport);
	if (status != PJ_SUCCESS)
	    return status;
    }

    /* Check if frame is not too large. */
    PJ_ASSERT_RETURN(fport->writepos+frame->size <= fport->buf+fport->bufsize,
		     PJMEDIA_EFRMFILETOOBIG);

    /* Copy frame to buffer. */
    pj_memcpy(fport->writepos, frame->buf, frame->size);
    fport->writepos += frame->size;

    return PJ_SUCCESS;
}

/*
 * Get frame, basicy is a no-op operation.
 */
static pj_status_t file_get_frame(pjmedia_port *this_port, 
				  pjmedia_frame *frame)
{
    PJ_UNUSED_ARG(this_port);
    PJ_UNUSED_ARG(frame);
    return PJ_EINVALIDOP;
}

/*
 * Close the port, modify file header with updated file length.
 */
static pj_status_t file_on_destroy(pjmedia_port *this_port)
{
    enum { FILE_LEN_POS = 4, DATA_LEN_POS = 40 };
    struct file_port *fport = (struct file_port *)this_port;
    pj_off_t file_size;
    pj_ssize_t bytes;
    pj_uint32_t wave_file_len;
    pj_uint32_t wave_data_len;
    pj_status_t status;

    /* Flush remaining buffers. */
    if (fport->writepos != fport->buf) 
	flush_buffer(fport);

    /* Get file size. */
    status = pj_file_getpos(fport->fd, &file_size);
    PJ_ASSERT_RETURN(status == PJ_SUCCESS, status);

    /* Calculate wave fields */
    wave_file_len = (pj_uint32_t)(file_size - 8);
    wave_data_len = (pj_uint32_t)(file_size - sizeof(pjmedia_wave_hdr));

#if defined(PJ_IS_BIG_ENDIAN) && PJ_IS_BIG_ENDIAN!=0
    wave_file_len = pj_swap32(wave_file_len);
    wave_data_len = pj_swap32(wave_data_len);
#endif

    /* Seek to the file_len field. */
    status = pj_file_setpos(fport->fd, FILE_LEN_POS, PJ_SEEK_SET);
    PJ_ASSERT_RETURN(status == PJ_SUCCESS, status);

    /* Write file_len */
    bytes = sizeof(wave_file_len);
    status = pj_file_write(fport->fd, &wave_file_len, &bytes);
    PJ_ASSERT_RETURN(status == PJ_SUCCESS, status);

    /* Seek to data_len field. */
    status = pj_file_setpos(fport->fd, DATA_LEN_POS, PJ_SEEK_SET);
    PJ_ASSERT_RETURN(status == PJ_SUCCESS, status);

    /* Write file_len */
    bytes = sizeof(wave_data_len);
    status = pj_file_write(fport->fd, &wave_data_len, &bytes);
    PJ_ASSERT_RETURN(status == PJ_SUCCESS, status);

    /* Close file */
    status = pj_file_close(fport->fd);
    PJ_ASSERT_RETURN(status == PJ_SUCCESS, status);

    /* Done. */
    return PJ_SUCCESS;
}

