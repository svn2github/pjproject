
# Before we include local os-mak, set default sound framework to use
# PortAudio.
export SOUND_OBJS = $(PORTAUDIO_OBJS)

# Also set default codec files to be included
# If these codecs are to be excluded, then the appropriate PJMEDIA_HAS_XXX_CODEC macros 
# MUST be set too in pj/config_site.h, to inform applications about the codec 
# availability.
#
# E.g., to exclude Speex, we need to exclude SPEEX_OBJS from CODEC_OBJS, AND specify
#       "#define PJMEDIA_HAS_SPEEX_CODEC 0" in pj/config_site.h.
#
export CODEC_OBJS = l16.o $(GSM_OBJS) $(SPEEX_OBJS) $(ILBC_OBJS)


include ../../build/common.mak

RULES_MAK := ../../build/rules.mak

PJLIB_LIB:=../../pjlib/lib/libpj-$(TARGET_NAME)$(LIBEXT)
PJLIB_UTIL_LIB:=../../pjlib-util/lib/libpjlib-util-$(TARGET_NAME)$(LIBEXT)

export PJMEDIA_LIB:=../lib/libpjmedia-$(TARGET_NAME)$(LIBEXT)
export PJMEDIA_CODEC_LIB:=../lib/libpjmedia-codec-$(TARGET_NAME)$(LIBEXT)
export PJSDP_LIB:=../lib/libpjsdp-$(TARGET_NAME)$(LIBEXT)

###############################################################################
# Sound frameworks
# These are activated by setting SOUND_OBJS var, e.g.:
#  export SOUND_OBJS = $(PORTAUDIO_OBJS), or
#  export SOUND_OBJS = $(NULLSOUND_OBJS)
#
PA_DIR:=portaudio
PORTAUDIO_OBJS := pasound.o \
		  $(PA_DIR)/pa_allocation.o \
		  $(PA_DIR)/pa_converters.o \
		  $(PA_DIR)/pa_cpuload.o \
		  $(PA_DIR)/pa_dither.o \
		  $(PA_DIR)/pa_front.o \
		  $(PA_DIR)/pa_process.o \
		  $(PA_DIR)/pa_skeleton.o \
		  $(PA_DIR)/pa_stream.o \
		  $(PA_DIR)/pa_trace.o

NULLSOUND_OBJS := nullsound.o

###############################################################################
# Gather all flags.
#
export _CFLAGS 	:= $(CC_CFLAGS) $(OS_CFLAGS) $(HOST_CFLAGS) $(M_CFLAGS) \
		   $(CFLAGS) $(CC_INC)../include $(CC_INC)../../pjlib/include \
		   $(CC_INC)../../pjlib-util/include \
		   $(CC_INC)../src/pjmedia/portaudio \
		   $(CC_INC)../src/pjmedia-codec
export _CXXFLAGS:= $(_CFLAGS) $(CC_CXXFLAGS) $(OS_CXXFLAGS) $(M_CXXFLAGS) \
		   $(HOST_CXXFLAGS) $(CXXFLAGS)
export _LDFLAGS := $(subst /,$(HOST_PSEP),$(PJMEDIA_LIB)) \
		   $(subst /,$(HOST_PSEP),$(PJLIB_LIB)) \
		   $(subst /,$(HOST_PSEP),$(PJLIB_UTIL_LIB)) \
		   $(CC_LDFLAGS) $(OS_LDFLAGS) $(M_LDFLAGS) $(HOST_LDFLAGS) \
		   $(LDFLAGS) 

###############################################################################
# Defines for building PJMEDIA library
#
export PJMEDIA_SRCDIR = ../src/pjmedia
export PJMEDIA_OBJS += $(OS_OBJS) $(M_OBJS) $(CC_OBJS) $(HOST_OBJS) \
			alaw_ulaw.o clock_thread.o codec.o conference.o \
			echo_common.o echo_port.o echo_speex.o \
			echo_suppress.o endpoint.o errno.o \
			g711.o jbuf.o master_port.o mem_capture.o mem_player.o \
			null_port.o plc_common.o plc_steveu.o \
			port.o splitcomb.o resample.o \
			resample_port.o rtcp.o rtp.o sdp.o sdp_cmp.o sdp_neg.o \
			session.o silencedet.o sound_port.o stream.o \
			tonegen.o transport_udp.o \
			wav_player.o wav_writer.o wave.o \
			$(SOUND_OBJS) $(NULLSOUND_OBJS)

export PJMEDIA_CFLAGS += $(_CFLAGS)


###############################################################################
# Defines for building PJSDP library
# Note that SDP functionality is already INCLUDED in PJMEDIA.
# The PJSDP library should only be used for applications that want SDP
# but don't want to use the rest of the media framework.
#
export PJSDP_SRCDIR = ../src/pjmedia
export PJSDP_OBJS += 	$(OS_OBJS) $(M_OBJS) $(CC_OBJS) $(HOST_OBJS) \
			errno.o sdp.o sdp_cmp.o sdp_neg.o
export PJSDP_CFLAGS += $(_CFLAGS)


###############################################################################
# Defines for building PJMEDIA-Codec library
#
GSM_OBJS := 	gsm.o \
		gsm/add.o gsm/code.o gsm/decode.o \
		gsm/gsm_create.o gsm/gsm_decode.o gsm/gsm_destroy.o \
		gsm/gsm_encode.o gsm/gsm_explode.o gsm/gsm_implode.o \
		gsm/gsm_option.o gsm/long_term.o \
		gsm/lpc.o gsm/preprocess.o gsm/rpe.o gsm/short_term.o \
		gsm/table.o
GSM_CFLAGS :=

SPEEX_OBJS :=	speex_codec.o \
		speex/bits.o speex/cb_search.o speex/exc_10_16_table.o  \
		speex/exc_10_32_table.o speex/exc_20_32_table.o \
		speex/exc_5_256_table.o speex/exc_5_64_table.o \
		speex/exc_8_128_table.o speex/fftwrap.o speex/filters.o \
		speex/gain_table.o speex/gain_table_lbr.o \
		speex/hexc_10_32_table.o speex/hexc_table.o \
		speex/high_lsp_tables.o speex/jitter.o \
		speex/kiss_fft.o speex/kiss_fftr.o speex/lpc_spx.o \
		speex/lsp.o speex/lsp_tables_nb.o speex/ltp.o \
		speex/math_approx.o speex/misc.o speex/mdf.o speex/modes.o \
		speex/nb_celp.o speex/preprocess_spx.o \
		speex/quant_lsp.o speex/sb_celp.o speex/smallft.o \
		speex/speex.o speex/speex_callbacks.o speex/speex_header.o \
		speex/stereo.o speex/vbr.o speex/vq.o speex/window.o
SPEEX_CFLAGS := -DHAVE_CONFIG_H=1 -I../src/pjmedia-codec

ILBC_OBJS := 	ilbc.o \
		ilbc/FrameClassify.o ilbc/LPCdecode.o ilbc/LPCencode.o \
		ilbc/StateConstructW.o ilbc/StateSearchW.o ilbc/anaFilter.o \
		ilbc/constants.o ilbc/createCB.o ilbc/doCPLC.o \
		ilbc/enhancer.o ilbc/filter.o ilbc/gainquant.o \
		ilbc/getCBvec.o ilbc/helpfun.o ilbc/hpInput.o \
		ilbc/hpOutput.o ilbc/iCBConstruct.o ilbc/iCBSearch.o \
		ilbc/iLBC_decode.o ilbc/iLBC_encode.o ilbc/lsf.o \
		ilbc/packing.o ilbc/syntFilter.o
ILBC_CFLAGS :=

export PJMEDIA_CODEC_SRCDIR = ../src/pjmedia-codec
export PJMEDIA_CODEC_OBJS += \
			$(OS_OBJS) $(M_OBJS) $(CC_OBJS) $(HOST_OBJS) \
			$(CODEC_OBJS)
export PJMEDIA_CODEC_CFLAGS += $(_CFLAGS) $(GSM_CFLAGS) $(SPEEX_CFLAGS) \
			$(ILBC_CFLAGS)


###############################################################################
# Defines for building test application
#
export PJMEDIA_TEST_SRCDIR = ../src/test
export PJMEDIA_TEST_OBJS += jbuf_test.o main.o rtp_test.o sdp_neg_test.o test.o
export PJMEDIA_TEST_CFLAGS += $(_CFLAGS)
export PJMEDIA_TEST_LDFLAGS += $(_LDFLAGS)
export PJMEDIA_TEST_EXE:=../bin/pjmedia-test-$(TARGET_NAME)$(HOST_EXE)

	
export CC_OUT CC AR RANLIB HOST_MV HOST_RM HOST_RMDIR HOST_MKDIR OBJEXT LD LDOUT 
###############################################################################
# Main entry
#
# $(TARGET) is defined in os-$(OS_NAME).mak file in current directory.
#
TARGETS := pjmedia pjmedia-codec pjsdp
#pjmedia-test

all: $(TARGETS)

doc:
	cd .. && doxygen docs/doxygen.cfg

dep: depend
distclean: realclean

.PHONY: dep depend pjmedia pjmedia-codec pjmedia-test clean realclean distclean

pjmedia:
	$(MAKE) -f $(RULES_MAK) APP=PJMEDIA app=pjmedia $(PJMEDIA_LIB)

pjmedia-codec:
	$(MAKE) -f $(RULES_MAK) APP=PJMEDIA_CODEC app=pjmedia-codec $(PJMEDIA_CODEC_LIB)

pjsdp:
	$(MAKE) -f $(RULES_MAK) APP=PJSDP app=pjsdp $(PJSDP_LIB)

pjmedia-test: $(PJMEDIA_LIB)
	$(MAKE) -f $(RULES_MAK) APP=PJMEDIA_TEST app=pjmedia-test $(PJMEDIA_TEST_EXE)

.PHONY: ../lib/pjmedia.ko
../lib/pjmedia.ko:
	echo Making $@
	$(MAKE) -f $(RULES_MAK) APP=PJMEDIA app=pjmedia $@

.PHONY: ../lib/pjmedia-codec.ko
../lib/pjmedia-codec.ko:
	echo Making $@
	$(MAKE) -f $(RULES_MAK) APP=PJMEDIA_CODEC app=pjmedia-codec $@

.PHONY: ../lib/pjmedia-test.ko
../lib/pjmedia-test.ko:
	$(MAKE) -f $(RULES_MAK) APP=PJMEDIA_TEST app=pjmedia-test $@

clean:
	$(MAKE) -f $(RULES_MAK) APP=PJMEDIA app=pjmedia $@
	$(MAKE) -f $(RULES_MAK) APP=PJMEDIA_CODEC app=pjmedia-codec $@
	$(MAKE) -f $(RULES_MAK) APP=PJSDP app=pjsdp $@
	$(MAKE) -f $(RULES_MAK) APP=PJMEDIA_TEST app=pjmedia-test $@

realclean:
	$(subst @@,$(subst /,$(HOST_PSEP),.pjmedia-$(TARGET_NAME).depend),$(HOST_RMR))
	$(subst @@,$(subst /,$(HOST_PSEP),.pjmedia-codec-$(TARGET_NAME).depend),$(HOST_RMR))
	$(subst @@,$(subst /,$(HOST_PSEP),.pjmedia-test-$(TARGET_NAME).depend),$(HOST_RMR))
	$(subst @@,$(subst /,$(HOST_PSEP),.pjsdp-$(TARGET_NAME).depend),$(HOST_RMR))
	
	$(MAKE) -f $(RULES_MAK) APP=PJMEDIA app=pjmedia $@
	$(MAKE) -f $(RULES_MAK) APP=PJMEDIA_CODEC app=pjmedia-codec $@
	$(MAKE) -f $(RULES_MAK) APP=PJMEDIA_TEST app=pjmedia-test $@
	$(MAKE) -f $(RULES_MAK) APP=PJSDP app=pjsdp $@

depend:
	$(MAKE) -f $(RULES_MAK) APP=PJMEDIA app=pjmedia $@
	$(MAKE) -f $(RULES_MAK) APP=PJMEDIA_CODEC app=pjmedia-codec $@
	$(MAKE) -f $(RULES_MAK) APP=PJMEDIA_TEST app=pjmedia-test $@
	$(MAKE) -f $(RULES_MAK) APP=PJSDP app=pjsdp $@
	echo '$(PJMEDIA_LIB): .pjmedia-$(TARGET_NAME).depend' >> .pjmedia-$(TARGET_NAME).depend; \
	echo '$(PJMEDIA_CODEC_LIB): .pjmedia-codec-$(TARGET_NAME).depend' >> .pjmedia-codec-$(TARGET_NAME).depend; \
	echo '$(PJMEDIA_TEST_EXE): $(PJMEDIA_LIB) $(PJMEDIA_CODEC_LIB)' >> .pjmedia-test-$(TARGET_NAME).depend; \


