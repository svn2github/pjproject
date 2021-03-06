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
#include <pjmedia/sound.h>
#include <pjmedia/errno.h>
#include <pj/assert.h>
#include <pj/log.h>
#include <pj/os.h>
#include <pj/string.h>

#if PJMEDIA_SOUND_IMPLEMENTATION == PJMEDIA_SOUND_WIN32_DIRECT_SOUND

#ifdef _MSC_VER
#   pragma warning(push, 3)
#endif

#include <windows.h>
#include <mmsystem.h>
#include <dsound.h>

#ifdef _MSC_VER
#   pragma warning(pop)
#endif



#define THIS_FILE	    "dsound.c"
#define BITS_PER_SAMPLE	    16
#define BYTES_PER_SAMPLE    (BITS_PER_SAMPLE/8)

#define MAX_PACKET_BUFFER_COUNT	    32
#define DEFAULT_BUFFER_COUNT	    5



/* Individual DirectSound capture/playback stream descriptor */
struct dsound_stream
{
    union 
    {
	struct
	{
	    LPDIRECTSOUND	    lpDs;
	    LPDIRECTSOUNDBUFFER	    lpDsBuffer;
	} play;

	struct
	{
	    LPDIRECTSOUNDCAPTURE	lpDs;
	    LPDIRECTSOUNDCAPTUREBUFFER	lpDsBuffer;
	} capture;
    } ds;

    HANDLE		    hEvent;
    LPDIRECTSOUNDNOTIFY	    lpDsNotify;
    DWORD		    dwBytePos;
    DWORD		    dwDsBufferSize;
    pj_timestamp	    timestamp;
};


/* Sound stream.    */
struct pjmedia_snd_stream
{
    pjmedia_dir		    dir;		/**< Sound direction.	    */
    pj_pool_t		   *pool;		/**< Memory pool.	    */
  
    pjmedia_snd_rec_cb	    rec_cb;		/**< Capture callback.	    */
    pjmedia_snd_play_cb	    play_cb;		/**< Playback callback.	    */
    void		   *user_data;		/**< Application data.	    */

    struct dsound_stream    play_strm;		/**< Playback stream.	    */
    struct dsound_stream    rec_strm;		/**< Capture stream.	    */

    void		   *buffer;		/**< Temp. frame buffer.    */
    unsigned		    samples_per_frame;	/**< Samples per frame.	    */

    pj_thread_t		   *thread;		/**< Thread handle.	    */
    pj_bool_t		    thread_quit_flag;	/**< Quit signal to thread  */
};


static pj_pool_factory *pool_factory;


static void init_waveformatex (PCMWAVEFORMAT *pcmwf, 
			       unsigned clock_rate,
			       unsigned channel_count)
{
    pj_memset(pcmwf, 0, sizeof(PCMWAVEFORMAT)); 
    pcmwf->wf.wFormatTag = WAVE_FORMAT_PCM; 
    pcmwf->wf.nChannels = (pj_uint16_t)channel_count;
    pcmwf->wf.nSamplesPerSec = clock_rate;
    pcmwf->wf.nBlockAlign = (pj_uint16_t)(channel_count * BYTES_PER_SAMPLE);
    pcmwf->wf.nAvgBytesPerSec = clock_rate * channel_count * BYTES_PER_SAMPLE;
    pcmwf->wBitsPerSample = BITS_PER_SAMPLE;
}


/*
 * Initialize DirectSound player device.
 */
static pj_status_t init_player_stream( struct dsound_stream *ds_strm,
				       unsigned clock_rate,
				       unsigned channel_count,
				       unsigned samples_per_frame,
				       unsigned buffer_count)
{
    HRESULT hr;
    HWND hwnd;
    PCMWAVEFORMAT pcmwf; 
    DSBUFFERDESC dsbdesc;
    DSBPOSITIONNOTIFY dsPosNotify[MAX_PACKET_BUFFER_COUNT];
    unsigned bytes_per_frame;
    unsigned i;


    PJ_ASSERT_RETURN(buffer_count <= MAX_PACKET_BUFFER_COUNT, PJ_EINVAL);

    /*
     * Create DirectSound device.
     */
    hr = DirectSoundCreate(NULL, &ds_strm->ds.play.lpDs, NULL);
    if (FAILED(hr))
	return PJ_RETURN_OS_ERROR(hr);

    hwnd = GetForegroundWindow();
    if (hwnd == NULL) {
	hwnd = GetDesktopWindow();
    }    
    hr = IDirectSound_SetCooperativeLevel( ds_strm->ds.play.lpDs, hwnd, 
					   DSSCL_PRIORITY);
    if FAILED(hr)
	return PJ_RETURN_OS_ERROR(hr);
    
    /*
     * Set up wave format structure for initialize DirectSound play
     * buffer. 
     */
    init_waveformatex(&pcmwf, clock_rate, channel_count);
    bytes_per_frame = samples_per_frame * BYTES_PER_SAMPLE;

    /* Set up DSBUFFERDESC structure. */
    pj_memset(&dsbdesc, 0, sizeof(DSBUFFERDESC)); 
    dsbdesc.dwSize = sizeof(DSBUFFERDESC); 
    dsbdesc.dwFlags = DSBCAPS_CTRLVOLUME | DSBCAPS_CTRLPOSITIONNOTIFY |
		      DSBCAPS_GETCURRENTPOSITION2 | DSBCAPS_GLOBALFOCUS;

    dsbdesc.dwBufferBytes = buffer_count * bytes_per_frame;
    dsbdesc.lpwfxFormat = (LPWAVEFORMATEX)&pcmwf; 

    /*
     * Create DirectSound playback buffer. 
     */
    hr = IDirectSound_CreateSoundBuffer(ds_strm->ds.play.lpDs, &dsbdesc, 
					&ds_strm->ds.play.lpDsBuffer, NULL); 
    if (FAILED(hr) )
	return PJ_RETURN_OS_ERROR(hr);

    /*
     * Create event for play notification.
     */
    ds_strm->hEvent = CreateEvent( NULL, FALSE, FALSE, NULL);
    if (ds_strm->hEvent == NULL)
	return pj_get_os_error();

    /*
     * Setup notification for play.
     */
    hr = IDirectSoundBuffer_QueryInterface( ds_strm->ds.play.lpDsBuffer, 
					    &IID_IDirectSoundNotify, 
					    (LPVOID *)&ds_strm->lpDsNotify); 
    if (FAILED(hr))
	return PJ_RETURN_OS_ERROR(hr);

    
    for (i=0; i<buffer_count; ++i) {
	dsPosNotify[i].dwOffset = i * bytes_per_frame;
	dsPosNotify[i].hEventNotify = ds_strm->hEvent;
    }
    
    hr = IDirectSoundNotify_SetNotificationPositions( ds_strm->lpDsNotify, 
						      buffer_count, 
						      dsPosNotify);
    if (FAILED(hr))
	return PJ_RETURN_OS_ERROR(hr);


    hr = IDirectSoundBuffer_SetCurrentPosition(ds_strm->ds.play.lpDsBuffer, 0);
    if (FAILED(hr))
	return PJ_RETURN_OS_ERROR(hr);


    ds_strm->dwBytePos = 0;
    ds_strm->dwDsBufferSize = buffer_count * bytes_per_frame;
    ds_strm->timestamp.u64 = 0;


    /* Done setting up play device. */
    PJ_LOG(5,(THIS_FILE, " DirectSound player stream initialized"));

    return PJ_SUCCESS;
}


/*
 * Initialize DirectSound recorder device
 */
static pj_status_t init_capture_stream( struct dsound_stream *ds_strm,
				        unsigned clock_rate,
				        unsigned channel_count,
				        unsigned samples_per_frame,
				        unsigned buffer_count)
{
    HRESULT hr;
    PCMWAVEFORMAT pcmwf; 
    DSCBUFFERDESC dscbdesc;
    DSBPOSITIONNOTIFY dsPosNotify[MAX_PACKET_BUFFER_COUNT];
    unsigned bytes_per_frame;
    unsigned i;


    PJ_ASSERT_RETURN(buffer_count <= MAX_PACKET_BUFFER_COUNT, PJ_EINVAL);


    /*
     * Creating recorder device.
     */
    hr = DirectSoundCaptureCreate(NULL, &ds_strm->ds.capture.lpDs, NULL);
    if (FAILED(hr))
	return PJ_RETURN_OS_ERROR(hr);


    /* Init wave format to initialize buffer */
    init_waveformatex( &pcmwf, clock_rate, channel_count);
    bytes_per_frame = samples_per_frame * BYTES_PER_SAMPLE;

    /* 
     * Setup capture buffer using sound buffer structure that was passed
     * to play buffer creation earlier.
     */
    pj_memset(&dscbdesc, 0, sizeof(DSCBUFFERDESC));
    dscbdesc.dwSize = sizeof(DSCBUFFERDESC); 
    dscbdesc.dwFlags = DSCBCAPS_WAVEMAPPED ;
    dscbdesc.dwBufferBytes = buffer_count * bytes_per_frame; 
    dscbdesc.lpwfxFormat = (LPWAVEFORMATEX)&pcmwf; 

    hr = IDirectSoundCapture_CreateCaptureBuffer( ds_strm->ds.capture.lpDs,
						  &dscbdesc, 
						  &ds_strm->ds.capture.lpDsBuffer,
						  NULL);
    if (FAILED(hr))
	return PJ_RETURN_OS_ERROR(hr);

    /*
     * Create event for play notification.
     */
    ds_strm->hEvent = CreateEvent( NULL, FALSE, FALSE, NULL);
    if (ds_strm->hEvent == NULL)
	return pj_get_os_error();

    /*
     * Setup notifications for recording.
     */
    hr = IDirectSoundCaptureBuffer_QueryInterface( ds_strm->ds.capture.lpDsBuffer, 
						   &IID_IDirectSoundNotify, 
						   (LPVOID *)&ds_strm->lpDsNotify); 
    if (FAILED(hr))
	return PJ_RETURN_OS_ERROR(hr);

    
    for (i=0; i<buffer_count; ++i) {
	dsPosNotify[i].dwOffset = i * bytes_per_frame;
	dsPosNotify[i].hEventNotify = ds_strm->hEvent;
    }
    
    hr = IDirectSoundNotify_SetNotificationPositions( ds_strm->lpDsNotify, 
						      buffer_count, 
						      dsPosNotify);
    if (FAILED(hr))
	return PJ_RETURN_OS_ERROR(hr);

    hr = IDirectSoundCaptureBuffer_GetCurrentPosition( ds_strm->ds.capture.lpDsBuffer, 
						       NULL, &ds_strm->dwBytePos );
    if (FAILED(hr))
	return PJ_RETURN_OS_ERROR(hr);

    ds_strm->timestamp.u64 = 0;
    ds_strm->dwDsBufferSize = buffer_count * bytes_per_frame;

    /* Done setting up recorder device. */
    PJ_LOG(5,(THIS_FILE, " DirectSound capture stream initialized"));

    return PJ_SUCCESS;
}



static BOOL AppReadDataFromBuffer(LPDIRECTSOUNDCAPTUREBUFFER lpDsb, // The buffer.
				  DWORD dwOffset,		    // Our own write cursor.
				  LPBYTE lpbSoundData,		    // Start of our data.
				  DWORD dwSoundBytes)		    // Size of block to copy.
{ 
    LPVOID  lpvPtr1; 
    DWORD dwBytes1; 
    LPVOID  lpvPtr2; 
    DWORD dwBytes2; 
    HRESULT hr; 
    
    // Obtain memory address of write block. This will be in two parts
    // if the block wraps around.
    
    hr = IDirectSoundCaptureBuffer_Lock( lpDsb, dwOffset, dwSoundBytes, &lpvPtr1, 
					 &dwBytes1, &lpvPtr2, &dwBytes2, 0); 
    
    if SUCCEEDED(hr) { 
	// Read from pointers. 
	CopyMemory(lpbSoundData, lpvPtr1, dwBytes1); 
	if (lpvPtr2 != NULL)
	    CopyMemory(lpbSoundData+dwBytes1, lpvPtr2, dwBytes2); 
	
	// Release the data back to DirectSound. 
	hr = IDirectSoundCaptureBuffer_Unlock(lpDsb, lpvPtr1, dwBytes1, lpvPtr2, dwBytes2); 
	if SUCCEEDED(hr)
	    return TRUE; 
    } 
    
    // Lock, Unlock, or Restore failed. 
    return FALSE; 
}


static BOOL AppWriteDataToBuffer(LPDIRECTSOUNDBUFFER lpDsb,  // The buffer.
				 DWORD dwOffset,	      // Our own write cursor.
				 LPBYTE lpbSoundData,	      // Start of our data.
				 DWORD dwSoundBytes)	      // Size of block to copy.
{ 
    LPVOID  lpvPtr1; 
    DWORD dwBytes1; 
    LPVOID  lpvPtr2; 
    DWORD dwBytes2; 
    HRESULT hr; 
    
    // Obtain memory address of write block. This will be in two parts
    // if the block wraps around.
    
    hr = IDirectSoundBuffer_Lock( lpDsb, dwOffset, dwSoundBytes, &lpvPtr1, 
				  &dwBytes1, &lpvPtr2, &dwBytes2, 0); 
    
    // If the buffer was lost, restore and retry lock. 
    if (DSERR_BUFFERLOST == hr) { 
	IDirectSoundBuffer_Restore(lpDsb); 
	hr = IDirectSoundBuffer_Lock( lpDsb, dwOffset, dwSoundBytes, 
				      &lpvPtr1, &dwBytes1, &lpvPtr2, &dwBytes2, 0); 
    } 
    if SUCCEEDED(hr) { 
	CopyMemory(lpvPtr1, lpbSoundData, dwBytes1); 
	if (NULL != lpvPtr2) 
	    CopyMemory(lpvPtr2, lpbSoundData+dwBytes1, dwBytes2); 
	
	hr = IDirectSoundBuffer_Unlock(lpDsb, lpvPtr1, dwBytes1, lpvPtr2, dwBytes2); 
	if SUCCEEDED(hr)
	    return TRUE; 
    } 
    
    return FALSE; 
}


/*
 * Player thread.
 */
static int dsound_dev_thread(void *arg)
{
    pjmedia_snd_stream *strm = arg;
    HANDLE events[2];
    unsigned eventCount;
    pj_status_t status;


    eventCount = 0;
    if (strm->dir & PJMEDIA_DIR_PLAYBACK)
	events[eventCount++] = strm->play_strm.hEvent;
    if (strm->dir & PJMEDIA_DIR_CAPTURE)
	events[eventCount++] = strm->rec_strm.hEvent;


    /* Raise self priority */
    SetThreadPriority( GetCurrentThread(), THREAD_PRIORITY_HIGHEST);

    /*
     * Loop while not signalled to quit, wait for event object to be signalled
     * by DirectSound play buffer, then request for sound data from the 
     * application and write it to DirectSound buffer.
     */
    while (!strm->thread_quit_flag) {
	
	DWORD rc;
	pjmedia_dir signalled_dir;
	struct dsound_stream *dsound_strm;

	rc = WaitForMultipleObjects(eventCount, events, FALSE, 100);
	if (rc < WAIT_OBJECT_0 || rc >= WAIT_OBJECT_0+eventCount)
	    continue;


	if (rc == WAIT_OBJECT_0) {
	    if (events[0] == strm->play_strm.hEvent)
		signalled_dir = PJMEDIA_DIR_PLAYBACK;
	    else
		signalled_dir = PJMEDIA_DIR_CAPTURE;
	} else {
	    if (events[1] == strm->play_strm.hEvent)
		signalled_dir = PJMEDIA_DIR_PLAYBACK;
	    else
		signalled_dir = PJMEDIA_DIR_CAPTURE;
	}


	if (signalled_dir == PJMEDIA_DIR_PLAYBACK) {
	    
	    dsound_strm = &strm->play_strm;

	    /* Get frame from application. */
	    status = (*strm->play_cb)(strm->user_data, 
				      dsound_strm->timestamp.u32.lo,
				      strm->buffer,
				      strm->samples_per_frame * 
					BYTES_PER_SAMPLE);
	    if (status != PJ_SUCCESS)
		break;

	    /* Write to DirectSound buffer. */
	    AppWriteDataToBuffer( dsound_strm->ds.play.lpDsBuffer, 
				  dsound_strm->dwBytePos,
				  (LPBYTE)strm->buffer, 
				  strm->samples_per_frame * BYTES_PER_SAMPLE);

	} else {
	    BOOL rc;

	    dsound_strm = &strm->rec_strm;

	    /* Capture from DirectSound buffer. */
	    rc = AppReadDataFromBuffer(dsound_strm->ds.capture.lpDsBuffer, 
				       dsound_strm->dwBytePos,
				       (LPBYTE)strm->buffer, 
				       strm->samples_per_frame * 
					BYTES_PER_SAMPLE);
	    
	    if (!rc) {
		pj_memset(strm->buffer, 0, strm->samples_per_frame * 
					    BYTES_PER_SAMPLE);
	    }

	    /* Call callback */
	    status = (*strm->rec_cb)(strm->user_data, 
				     dsound_strm->timestamp.u32.lo, 
				     strm->buffer, 
				     strm->samples_per_frame * 
					BYTES_PER_SAMPLE);

	    /* Quit thread on error. */
	    if (status != PJ_SUCCESS)
		break;

	}

	/* Increment position. */
	dsound_strm->dwBytePos += strm->samples_per_frame * BYTES_PER_SAMPLE;
	if (dsound_strm->dwBytePos >= dsound_strm->dwDsBufferSize)
	    dsound_strm->dwBytePos -= dsound_strm->dwDsBufferSize;
	dsound_strm->timestamp.u64 += strm->samples_per_frame;
    }


    PJ_LOG(5,(THIS_FILE, "DirectSound: thread stopping.."));

    return 0;
}



/*
 * Init sound library.
 */
PJ_DEF(pj_status_t) pjmedia_snd_init(pj_pool_factory *factory)
{
    pool_factory = factory;
    return PJ_SUCCESS;
}

/*
 * Deinitialize sound library.
 */
PJ_DEF(pj_status_t) pjmedia_snd_deinit(void)
{
    return PJ_SUCCESS;
}

/*
 * Get device count.
 */
PJ_DEF(int) pjmedia_snd_get_dev_count(void)
{
    return 1;
}

/*
 * Get device info.
 */
PJ_DEF(const pjmedia_snd_dev_info*) pjmedia_snd_get_dev_info(unsigned index)
{
    static pjmedia_snd_dev_info info;

    PJ_UNUSED_ARG(index);

    pj_memset(&info, 0, sizeof(info));

    info.default_samples_per_sec = 44100;
    info.input_count = 1;
    info.output_count = 1;
    pj_ansi_strcpy(info.name, "Default DirectSound Device");

    return &info;
}


/*
 * Open stream.
 */
static pj_status_t open_stream( pjmedia_dir dir,
			        int rec_id,
				int play_id,
				unsigned clock_rate,
				unsigned channel_count,
				unsigned samples_per_frame,
				unsigned bits_per_sample,
				pjmedia_snd_rec_cb rec_cb,
				pjmedia_snd_play_cb play_cb,
				void *user_data,
				pjmedia_snd_stream **p_snd_strm)
{
    pj_pool_t *pool;
    pjmedia_snd_stream *strm;
    pj_status_t status;


    /* Make sure sound subsystem has been initialized with
     * pjmedia_snd_init()
     */
    PJ_ASSERT_RETURN( pool_factory != NULL, PJ_EINVALIDOP );


    /* Can only support 16bits per sample */
    PJ_ASSERT_RETURN(bits_per_sample == BITS_PER_SAMPLE, PJ_EINVAL);

    /* Don't support device at present */
    PJ_UNUSED_ARG(rec_id);
    PJ_UNUSED_ARG(play_id);
    

    /* Create and Initialize stream descriptor */
    pool = pj_pool_create(pool_factory, "dsound-dev", 1000, 1000, NULL);
    PJ_ASSERT_RETURN(pool != NULL, PJ_ENOMEM);

    strm = pj_pool_zalloc(pool, sizeof(pjmedia_snd_stream));
    strm->dir = dir;
    strm->pool = pool;
    strm->rec_cb = rec_cb;
    strm->play_cb = play_cb;
    strm->user_data = user_data;

    strm->samples_per_frame = samples_per_frame;
    strm->buffer = pj_pool_alloc(pool, samples_per_frame * BYTES_PER_SAMPLE);
    if (!strm->buffer) {
	pj_pool_release(pool);
	return PJ_ENOMEM;
    }

    /* Create player stream */
    if (dir & PJMEDIA_DIR_PLAYBACK) {
	status = init_player_stream( &strm->play_strm, clock_rate,
				     channel_count, samples_per_frame,
				     DEFAULT_BUFFER_COUNT );
	if (status != PJ_SUCCESS) {
	    pjmedia_snd_stream_close(strm);
	    return status;
	}
    }

    /* Create capture stream */
    if (dir & PJMEDIA_DIR_CAPTURE) {
	status = init_capture_stream( &strm->rec_strm, clock_rate,
				      channel_count, samples_per_frame,
				      DEFAULT_BUFFER_COUNT);
	if (status != PJ_SUCCESS) {
	    pjmedia_snd_stream_close(strm);
	    return status;
	}
    }


    /* Create and start the thread */
    status = pj_thread_create(pool, "dsound", &dsound_dev_thread, strm,
			      0, 0, &strm->thread);
    if (status != PJ_SUCCESS) {
	pjmedia_snd_stream_close(strm);
	return status;
    }

    *p_snd_strm = strm;

    return PJ_SUCCESS;
}

/*
 * Open stream.
 */
PJ_DEF(pj_status_t) pjmedia_snd_open_rec( int index,
					  unsigned clock_rate,
					  unsigned channel_count,
					  unsigned samples_per_frame,
					  unsigned bits_per_sample,
					  pjmedia_snd_rec_cb rec_cb,
					  void *user_data,
					  pjmedia_snd_stream **p_snd_strm)
{
    PJ_ASSERT_RETURN(rec_cb && p_snd_strm, PJ_EINVAL);

    return open_stream( PJMEDIA_DIR_CAPTURE, index, -1,
			clock_rate, channel_count, samples_per_frame,
			bits_per_sample, rec_cb, NULL, user_data,
			p_snd_strm);
}

PJ_DEF(pj_status_t) pjmedia_snd_open_player( int index,
					unsigned clock_rate,
					unsigned channel_count,
					unsigned samples_per_frame,
					unsigned bits_per_sample,
					pjmedia_snd_play_cb play_cb,
					void *user_data,
					pjmedia_snd_stream **p_snd_strm)
{
    PJ_ASSERT_RETURN(play_cb && p_snd_strm, PJ_EINVAL);

    return open_stream( PJMEDIA_DIR_PLAYBACK, -1, index,
			clock_rate, channel_count, samples_per_frame,
			bits_per_sample, NULL, play_cb, user_data,
			p_snd_strm);
}

/*
 * Open both player and recorder.
 */
PJ_DEF(pj_status_t) pjmedia_snd_open( int rec_id,
				      int play_id,
				      unsigned clock_rate,
				      unsigned channel_count,
				      unsigned samples_per_frame,
				      unsigned bits_per_sample,
				      pjmedia_snd_rec_cb rec_cb,
				      pjmedia_snd_play_cb play_cb,
				      void *user_data,
				      pjmedia_snd_stream **p_snd_strm)
{
    PJ_ASSERT_RETURN(rec_cb && play_cb && p_snd_strm, PJ_EINVAL);

    return open_stream( PJMEDIA_DIR_CAPTURE_PLAYBACK, rec_id, play_id,
			clock_rate, channel_count, samples_per_frame,
			bits_per_sample, rec_cb, play_cb, user_data,
			p_snd_strm );
}

/*
 * Start stream.
 */
PJ_DEF(pj_status_t) pjmedia_snd_stream_start(pjmedia_snd_stream *stream)
{
    HRESULT hr;

    PJ_UNUSED_ARG(stream);

    if (stream->play_strm.ds.play.lpDsBuffer) {
	hr = IDirectSoundBuffer_Play(stream->play_strm.ds.play.lpDsBuffer, 
				     0, 0, DSBPLAY_LOOPING);
	if (FAILED(hr))
	    return PJ_RETURN_OS_ERROR(hr);
    }
    
    if (stream->rec_strm.ds.capture.lpDsBuffer) {
	hr = IDirectSoundCaptureBuffer_Start(stream->rec_strm.ds.capture.lpDsBuffer,
					     DSCBSTART_LOOPING );
	if (FAILED(hr))
	    return PJ_RETURN_OS_ERROR(hr);
    }

    return PJ_SUCCESS;
}

/*
 * Stop stream.
 */
PJ_DEF(pj_status_t) pjmedia_snd_stream_stop(pjmedia_snd_stream *stream)
{
    PJ_ASSERT_RETURN(stream != NULL, PJ_EINVAL);

    if (stream->play_strm.ds.play.lpDsBuffer) {
	PJ_LOG(5,(THIS_FILE, "Stopping DirectSound playback stream"));
	IDirectSoundBuffer_Stop( stream->play_strm.ds.play.lpDsBuffer );
    }

    if (stream->rec_strm.ds.capture.lpDsBuffer) {
	PJ_LOG(5,(THIS_FILE, "Stopping DirectSound capture stream"));
	IDirectSoundCaptureBuffer_Stop(stream->rec_strm.ds.capture.lpDsBuffer);
    }

    return PJ_SUCCESS;
}


/*
 * Destroy stream.
 */
PJ_DEF(pj_status_t) pjmedia_snd_stream_close(pjmedia_snd_stream *stream)
{
    PJ_ASSERT_RETURN(stream != NULL, PJ_EINVAL);

    pjmedia_snd_stream_stop(stream);

    if (stream->play_strm.lpDsNotify) {
	IDirectSoundNotify_Release( stream->play_strm.lpDsNotify );
	stream->play_strm.lpDsNotify = NULL;
    }
    
    if (stream->play_strm.hEvent) {
	CloseHandle(stream->play_strm.hEvent);
	stream->play_strm.hEvent = NULL;
    }

    if (stream->play_strm.ds.play.lpDsBuffer) {
	IDirectSoundBuffer_Release( stream->play_strm.ds.play.lpDsBuffer );
	stream->play_strm.ds.play.lpDsBuffer = NULL;
    }

    if (stream->play_strm.ds.play.lpDs) {
	IDirectSound_Release( stream->play_strm.ds.play.lpDs );
	stream->play_strm.ds.play.lpDs = NULL;
    }

    if (stream->rec_strm.lpDsNotify) {
	IDirectSoundNotify_Release( stream->rec_strm.lpDsNotify );
	stream->rec_strm.lpDsNotify = NULL;
    }
    
    if (stream->rec_strm.hEvent) {
	CloseHandle(stream->rec_strm.hEvent);
	stream->rec_strm.hEvent = NULL;
    }

    if (stream->rec_strm.ds.capture.lpDsBuffer) {
	IDirectSoundCaptureBuffer_Release( stream->rec_strm.ds.capture.lpDsBuffer );
	stream->rec_strm.ds.capture.lpDsBuffer = NULL;
    }

    if (stream->rec_strm.ds.capture.lpDs) {
	IDirectSoundCapture_Release( stream->rec_strm.ds.capture.lpDs );
	stream->rec_strm.ds.capture.lpDs = NULL;
    }

    if (stream->thread) {
	stream->thread_quit_flag = 1;
	pj_thread_join(stream->thread);
	pj_thread_destroy(stream->thread);
	stream->thread = NULL;
    }

    pj_pool_release(stream->pool);

    return PJ_SUCCESS;
}


#endif	/* PJMEDIA_SOUND_IMPLEMENTATION */

