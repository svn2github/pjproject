# Microsoft Developer Studio Project File - Name="pjmedia" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=pjmedia - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "pjmedia.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "pjmedia.mak" CFG="pjmedia - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "pjmedia - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "pjmedia - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "pjmedia - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir ".\output\pjmedia-i386-win32-vc6-release"
# PROP BASE Intermediate_Dir ".\output\pjmedia-i386-win32-vc6-release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir ".\output\pjmedia-i386-win32-vc6-release"
# PROP Intermediate_Dir ".\output\pjmedia-i386-win32-vc6-release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD CPP /nologo /MD /W4 /GX /Zi /O2 /I "../include" /I "../../pjlib/include" /I "../../pjlib-util/include" /I "../src/pjmedia/portaudio" /I "../src/pjmedia-codec" /D "NDEBUG" /D "PA_NO_ASIO" /D "WIN32" /D "_MBCS" /D "_LIB" /D PJ_WIN32=1 /D PJ_M_I386=1 /FR /FD /c
# SUBTRACT CPP /YX
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"../lib/pjmedia-i386-win32-vc6-release.lib"

!ELSEIF  "$(CFG)" == "pjmedia - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir ".\output\pjmedia-i386-win32-vc6-debug"
# PROP BASE Intermediate_Dir ".\output\pjmedia-i386-win32-vc6-debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir ".\output\pjmedia-i386-win32-vc6-debug"
# PROP Intermediate_Dir ".\output\pjmedia-i386-win32-vc6-debug"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /YX /FD /GZ /c
# ADD CPP /nologo /MTd /W4 /Gm /GX /ZI /Od /I "../include" /I "../../pjlib/include" /I "../../pjlib-util/include" /I "../src/pjmedia/portaudio" /I "../src/pjmedia-codec" /D "_DEBUG" /D "PA_NO_ASIO" /D "WIN32" /D "_MBCS" /D "_LIB" /D PJ_WIN32=1 /D PJ_M_I386=1 /FR /FD /GZ /c
# SUBTRACT CPP /YX
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"../lib/pjmedia-i386-win32-vc6-debug.lib"

!ENDIF 

# Begin Target

# Name "pjmedia - Win32 Release"
# Name "pjmedia - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\src\pjmedia\alaw_ulaw.c
# End Source File
# Begin Source File

SOURCE=..\src\pjmedia\alaw_ulaw_table.c
# End Source File
# Begin Source File

SOURCE=..\src\pjmedia\bidirectional.c
# End Source File
# Begin Source File

SOURCE=..\src\pjmedia\clock_thread.c
# End Source File
# Begin Source File

SOURCE=..\src\pjmedia\codec.c
# End Source File
# Begin Source File

SOURCE=..\src\pjmedia\conference.c
# End Source File
# Begin Source File

SOURCE=..\src\pjmedia\dsound.c
# End Source File
# Begin Source File

SOURCE=..\src\pjmedia\echo_common.c
# End Source File
# Begin Source File

SOURCE=..\src\pjmedia\echo_port.c
# End Source File
# Begin Source File

SOURCE=..\src\pjmedia\echo_speex.c
# End Source File
# Begin Source File

SOURCE=..\src\pjmedia\echo_suppress.c
# End Source File
# Begin Source File

SOURCE=..\src\pjmedia\endpoint.c
# End Source File
# Begin Source File

SOURCE=..\src\pjmedia\errno.c
# End Source File
# Begin Source File

SOURCE=..\src\pjmedia\g711.c
# End Source File
# Begin Source File

SOURCE=..\src\pjmedia\jbuf.c
# End Source File
# Begin Source File

SOURCE=..\src\pjmedia\master_port.c
# End Source File
# Begin Source File

SOURCE=..\src\pjmedia\mem_capture.c
# End Source File
# Begin Source File

SOURCE=..\src\pjmedia\mem_player.c
# End Source File
# Begin Source File

SOURCE=..\src\pjmedia\mp3_writer.c
# End Source File
# Begin Source File

SOURCE=..\src\pjmedia\null_port.c
# End Source File
# Begin Source File

SOURCE=..\src\pjmedia\nullsound.c
# End Source File
# Begin Source File

SOURCE=..\src\pjmedia\pasound.c
# End Source File
# Begin Source File

SOURCE=..\src\pjmedia\plc_common.c
# End Source File
# Begin Source File

SOURCE=..\src\pjmedia\plc_steveu.c
# ADD CPP /W2
# End Source File
# Begin Source File

SOURCE=..\src\pjmedia\plc_steveu.h
# End Source File
# Begin Source File

SOURCE=..\src\pjmedia\port.c
# End Source File
# Begin Source File

SOURCE=..\src\pjmedia\resample.c
# End Source File
# Begin Source File

SOURCE=..\src\pjmedia\resample_port.c
# End Source File
# Begin Source File

SOURCE=..\src\pjmedia\rtcp.c
# End Source File
# Begin Source File

SOURCE=..\src\pjmedia\rtp.c
# End Source File
# Begin Source File

SOURCE=..\src\pjmedia\sdp.c
# End Source File
# Begin Source File

SOURCE=..\src\pjmedia\sdp_cmp.c
# End Source File
# Begin Source File

SOURCE=..\src\pjmedia\sdp_neg.c
# End Source File
# Begin Source File

SOURCE=..\src\pjmedia\session.c
# End Source File
# Begin Source File

SOURCE=..\src\pjmedia\silencedet.c
# End Source File
# Begin Source File

SOURCE=..\src\pjmedia\sound_port.c
# End Source File
# Begin Source File

SOURCE=..\src\pjmedia\splitcomb.c
# End Source File
# Begin Source File

SOURCE=..\src\pjmedia\stream.c
# End Source File
# Begin Source File

SOURCE=..\src\pjmedia\tonegen.c
# End Source File
# Begin Source File

SOURCE=..\src\pjmedia\transport_udp.c
# End Source File
# Begin Source File

SOURCE=..\src\pjmedia\wav_player.c
# End Source File
# Begin Source File

SOURCE=..\src\pjmedia\wav_playlist.c
# End Source File
# Begin Source File

SOURCE=..\src\pjmedia\wav_writer.c
# End Source File
# Begin Source File

SOURCE=..\src\pjmedia\wave.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\include\pjmedia\alaw_ulaw.h
# End Source File
# Begin Source File

SOURCE=..\include\pjmedia\bidirectional.h
# End Source File
# Begin Source File

SOURCE=..\include\pjmedia\clock.h
# End Source File
# Begin Source File

SOURCE=..\include\pjmedia\codec.h
# End Source File
# Begin Source File

SOURCE=..\include\pjmedia\conference.h
# End Source File
# Begin Source File

SOURCE=..\include\pjmedia\config.h
# End Source File
# Begin Source File

SOURCE=..\include\pjmedia\doxygen.h
# End Source File
# Begin Source File

SOURCE=..\include\pjmedia\echo.h
# End Source File
# Begin Source File

SOURCE=..\include\pjmedia\echo_port.h
# End Source File
# Begin Source File

SOURCE=..\include\pjmedia\endpoint.h
# End Source File
# Begin Source File

SOURCE=..\include\pjmedia\errno.h
# End Source File
# Begin Source File

SOURCE=..\include\pjmedia\g711.h
# End Source File
# Begin Source File

SOURCE=..\include\pjmedia\jbuf.h
# End Source File
# Begin Source File

SOURCE=..\include\pjmedia\master_port.h
# End Source File
# Begin Source File

SOURCE=..\include\pjmedia\mem_port.h
# End Source File
# Begin Source File

SOURCE=..\include\pjmedia\mp3_port.h
# End Source File
# Begin Source File

SOURCE=..\include\pjmedia\null_port.h
# End Source File
# Begin Source File

SOURCE=..\include\pjmedia.h
# End Source File
# Begin Source File

SOURCE=..\include\pjmedia\plc.h
# End Source File
# Begin Source File

SOURCE=..\include\pjmedia\port.h
# End Source File
# Begin Source File

SOURCE=..\include\pjmedia\resample.h
# End Source File
# Begin Source File

SOURCE=..\include\pjmedia\rtcp.h
# End Source File
# Begin Source File

SOURCE=..\include\pjmedia\rtp.h
# End Source File
# Begin Source File

SOURCE=..\include\pjmedia\sdp.h
# End Source File
# Begin Source File

SOURCE=..\include\pjmedia\sdp_neg.h
# End Source File
# Begin Source File

SOURCE=..\include\pjmedia\session.h
# End Source File
# Begin Source File

SOURCE=..\include\pjmedia\silencedet.h
# End Source File
# Begin Source File

SOURCE=..\include\pjmedia\sound.h
# End Source File
# Begin Source File

SOURCE=..\include\pjmedia\sound_port.h
# End Source File
# Begin Source File

SOURCE=..\include\pjmedia\splitcomb.h
# End Source File
# Begin Source File

SOURCE=..\include\pjmedia\stream.h
# End Source File
# Begin Source File

SOURCE=..\include\pjmedia\tonegen.h
# End Source File
# Begin Source File

SOURCE=..\include\pjmedia\transport.h
# End Source File
# Begin Source File

SOURCE=..\include\pjmedia\transport_udp.h
# End Source File
# Begin Source File

SOURCE=..\include\pjmedia\types.h
# End Source File
# Begin Source File

SOURCE=..\include\pjmedia\wav_playlist.h
# End Source File
# Begin Source File

SOURCE=..\include\pjmedia\wav_port.h
# End Source File
# Begin Source File

SOURCE=..\include\pjmedia\wave.h
# End Source File
# End Group
# Begin Group "PortAudio Files"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\src\pjmedia\portaudio\dsound_wrapper.c
# ADD CPP /W3
# End Source File
# Begin Source File

SOURCE=..\src\pjmedia\portaudio\dsound_wrapper.h
# End Source File
# Begin Source File

SOURCE=..\src\pjmedia\portaudio\pa_allocation.c
# ADD CPP /W3
# End Source File
# Begin Source File

SOURCE=..\src\pjmedia\portaudio\pa_allocation.h
# End Source File
# Begin Source File

SOURCE=..\src\pjmedia\portaudio\pa_converters.c
# ADD CPP /W3
# End Source File
# Begin Source File

SOURCE=..\src\pjmedia\portaudio\pa_converters.h
# End Source File
# Begin Source File

SOURCE=..\src\pjmedia\portaudio\pa_cpuload.c
# ADD CPP /W3
# End Source File
# Begin Source File

SOURCE=..\src\pjmedia\portaudio\pa_cpuload.h
# End Source File
# Begin Source File

SOURCE=..\src\pjmedia\portaudio\pa_dither.c
# ADD CPP /W3
# End Source File
# Begin Source File

SOURCE=..\src\pjmedia\portaudio\pa_dither.h
# End Source File
# Begin Source File

SOURCE=..\src\pjmedia\portaudio\pa_endianness.h
# End Source File
# Begin Source File

SOURCE=..\src\pjmedia\portaudio\pa_front.c
# ADD CPP /W3
# End Source File
# Begin Source File

SOURCE=..\src\pjmedia\portaudio\pa_hostapi.h
# End Source File
# Begin Source File

SOURCE=..\src\pjmedia\portaudio\pa_process.c
# ADD CPP /W3
# End Source File
# Begin Source File

SOURCE=..\src\pjmedia\portaudio\pa_process.h
# End Source File
# Begin Source File

SOURCE=..\src\pjmedia\portaudio\pa_skeleton.c
# ADD CPP /W3
# End Source File
# Begin Source File

SOURCE=..\src\pjmedia\portaudio\pa_stream.c
# ADD CPP /W3
# End Source File
# Begin Source File

SOURCE=..\src\pjmedia\portaudio\pa_stream.h
# End Source File
# Begin Source File

SOURCE=..\src\pjmedia\portaudio\pa_trace.c
# ADD CPP /W3
# End Source File
# Begin Source File

SOURCE=..\src\pjmedia\portaudio\pa_trace.h
# End Source File
# Begin Source File

SOURCE=..\src\pjmedia\portaudio\pa_types.h
# End Source File
# Begin Source File

SOURCE=..\src\pjmedia\portaudio\pa_util.h
# End Source File
# Begin Source File

SOURCE=..\src\pjmedia\portaudio\pa_win_ds.c
# ADD CPP /W3
# End Source File
# Begin Source File

SOURCE=..\src\pjmedia\portaudio\pa_win_hostapis.c
# ADD CPP /W3
# End Source File
# Begin Source File

SOURCE=..\src\pjmedia\portaudio\pa_win_util.c
# ADD CPP /W3
# End Source File
# Begin Source File

SOURCE=..\src\pjmedia\portaudio\pa_win_wmme.c
# ADD CPP /W3
# End Source File
# Begin Source File

SOURCE=..\src\pjmedia\portaudio\pa_win_wmme.h
# End Source File
# Begin Source File

SOURCE=..\src\pjmedia\portaudio\pa_x86_plain_converters.c
# ADD CPP /W3
# End Source File
# Begin Source File

SOURCE=..\src\pjmedia\portaudio\pa_x86_plain_converters.h
# End Source File
# Begin Source File

SOURCE=..\src\pjmedia\portaudio\portaudio.h
# End Source File
# End Group
# Begin Group "Resample Files"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\src\pjmedia\largefilter.h
# End Source File
# Begin Source File

SOURCE=..\src\pjmedia\smallfilter.h
# End Source File
# End Group
# End Target
# End Project
