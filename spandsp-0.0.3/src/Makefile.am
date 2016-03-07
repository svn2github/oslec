## Makefile.am -- Process this file with automake to produce Makefile.in
##
## This program is free software; you can redistribute it and/or modify
## it under the terms of the GNU General Public License version 2, as
## published by the Free Software Foundation.
##
## This program is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## GNU General Public License for more details.
##
## You should have received a copy of the GNU General Public License
## along with this program; if not, write to the Free Software
## Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
##
## $Id: Makefile.am,v 1.61 2006/11/30 15:41:47 steveu Exp $

AM_CFLAGS = $(COMP_VENDOR_CFLAGS)

MAINTAINERCLEANFILES = Makefile.in

INCLUDES = -I$(top_builddir)

lib_LTLIBRARIES = libspandsp.la

libspandsp_la_SOURCES = adsi.c \
                        async.c \
                        at_interpreter.c \
                        awgn.c \
                        bell_r2_mf.c \
                        bert.c \
                        bit_operations.c \
                        bitstream.c \
                        complex_dds.c \
                        complex_filters.c \
                        complex_vector_float.c \
                        dds.c \
                        dtmf.c \
                        echo.c \
                        fax.c \
                        fsk.c \
                        g711.c \
                        g722_encode.c \
                        g722_decode.c \
                        g726.c \
                        gsm0610_decode.c \
                        gsm0610_encode.c \
                        gsm0610_long_term.c \
                        gsm0610_lpc.c \
                        gsm0610_preprocess.c \
                        gsm0610_rpe.c \
                        gsm0610_short_term.c \
                        hdlc.c \
                        ima_adpcm.c \
                        logging.c \
                        lpc10_analyse.c \
                        lpc10_decode.c \
                        lpc10_encode.c \
                        lpc10_placev.c \
                        lpc10_voicing.c \
                        modem_echo.c \
                        modem_connect_tones.c \
                        noise.c \
                        oki_adpcm.c \
                        playout.c \
                        plc.c \
                        power_meter.c \
                        queue.c \
                        schedule.c \
                        sig_tone.c \
                        silence_gen.c \
                        super_tone_rx.c \
                        super_tone_tx.c \
                        t4.c \
                        t30.c \
                        t31.c \
                        t35.c \
                        t38_core.c \
                        t38_gateway.c \
                        t38_terminal.c \
                        testcpuid.c \
                        time_scale.c \
                        tone_detect.c \
                        tone_generate.c \
                        v17rx.c \
                        v17tx.c \
                        v22bis_rx.c \
                        v22bis_tx.c \
                        v27ter_rx.c \
                        v27ter_tx.c \
                        v29rx.c \
                        v29tx.c \
                        v42.c \
                        v42bis.c \
                        v8.c \
                        vector_float.c \
                        vector_int.c

libspandsp_la_LDFLAGS = -version-info @SPANDSP_LT_CURRENT@:@SPANDSP_LT_REVISION@:@SPANDSP_LT_AGE@

nobase_include_HEADERS = spandsp/adsi.h \
                        spandsp/async.h \
                        spandsp/arctan2.h \
                        spandsp/at_interpreter.h \
                        spandsp/awgn.h \
                        spandsp/bell_r2_mf.h \
                        spandsp/bert.h \
                        spandsp/biquad.h \
                        spandsp/bit_operations.h \
                        spandsp/bitstream.h \
                        spandsp/complex.h \
                        spandsp/complex_filters.h \
                        spandsp/complex_vector_float.h \
                        spandsp/dc_restore.h \
                        spandsp/dds.h \
                        spandsp/dtmf.h \
                        spandsp/echo.h \
                        spandsp/fax.h \
                        spandsp/fir.h \
                        spandsp/fsk.h \
                        spandsp/g168models.h \
                        spandsp/g711.h \
                        spandsp/g722.h \
                        spandsp/g726.h \
                        spandsp/gsm0610.h \
                        spandsp/hdlc.h \
                        spandsp/ima_adpcm.h \
                        spandsp/logging.h \
                        spandsp/lpc10.h \
                        spandsp/modem_echo.h \
                        spandsp/modem_connect_tones.h \
                        spandsp/noise.h \
                        spandsp/oki_adpcm.h \
                        spandsp/playout.h \
                        spandsp/plc.h \
                        spandsp/power_meter.h \
                        spandsp/queue.h \
                        spandsp/schedule.h \
                        spandsp/sig_tone.h \
                        spandsp/silence_gen.h \
                        spandsp/super_tone_rx.h \
                        spandsp/super_tone_tx.h \
                        spandsp/t4.h \
                        spandsp/t30.h \
                        spandsp/t30_fcf.h \
                        spandsp/t31.h \
                        spandsp/t35.h \
                        spandsp/t38_core.h \
                        spandsp/t38_gateway.h \
                        spandsp/t38_terminal.h \
                        spandsp/telephony.h \
                        spandsp/time_scale.h \
                        spandsp/timing.h \
                        spandsp/tone_detect.h \
                        spandsp/tone_generate.h \
                        spandsp/v17rx.h \
                        spandsp/v17tx.h \
                        spandsp/v22bis.h \
                        spandsp/v27ter_rx.h \
                        spandsp/v27ter_tx.h \
                        spandsp/v29rx.h \
                        spandsp/v29tx.h \
                        spandsp/v42.h \
                        spandsp/v42bis.h \
                        spandsp/v8.h \
                        spandsp/vector_float.h \
                        spandsp/vector_int.h \
                        spandsp.h

noinst_HEADERS =        faxfont.h \
                        gsm0610_local.h \
                        lpc10_encdecs.h \
                        t4states.h

noinst_PROGRAMS =       at_dictionary_gen

# We need to run at_dictionary_gen, so it generates the at_interpreter_dictionary.h file
# used by several of the test programs.

at_interpreter.$(OBJEXT): at_dictionary_gen$(EXEEXT) at_interpreter_dictionary.h

at_interpreter.lo: at_dictionary_gen$(EXEEXT) at_interpreter_dictionary.h

at_interpreter_dictionary.h: at_dictionary_gen$(EXEEXT)
	./at_dictionary_gen$(EXEEXT) >at_interpreter_dictionary.h

DSP = libspandsp.dsp
VCPROJ = libspandsp.vcproj

WIN32SOURCES = $(libspandsp_la_SOURCES) gettimeofday.c
WIN32HEADERS = $(nobase_include_HEADERS) spandsp.h

DSPOUT = | awk '{printf("%s\r\n", $$0)}' >> $(DSP)
VCPROJOUT = | awk '{printf("%s\r\n", $$0)}' >> $(VCPROJ)

$(DSP): msvc/msvcproj.head msvc/msvcproj.foot Makefile.am
	echo "creating $(DSP)"
	@(cp $(srcdir)/msvc/msvcproj.head $(DSP); \
	echo "# Begin Group \"Source Files\"" $(DSPOUT); \
	for file in $(WIN32SOURCES); do \
	echo "# Begin Source File" $(DSPOUT); \
	echo "" $(DSPOUT); \
	echo "SOURCE=.\\"$$file $(DSPOUT); \
	echo "# End Source File" $(DSPOUT); \
	done; \
	echo "# End Group" $(DSPOUT); \
	echo "# Begin Group \"Header Files\"" $(DSPOUT); \
	for file in $(WIN32HEADERS); do \
	echo "# Begin Source File" $(DSPOUT); \
	echo "" $(DSPOUT); \
	echo "SOURCE=.\\"$$file $(DSPOUT); \
	echo "# End Source File" $(DSPOUT); \
	done; \
	echo "# End Group" $(DSPOUT); \
	cat $(srcdir)/msvc/msvcproj.foot $(DSPOUT) )

$(VCPROJ): msvc/vc8proj.head msvc/vc8proj.foot Makefile.am
	echo "creating $(VCPROJ)"
	@(cp $(srcdir)/msvc/vc8proj.head $(VCPROJ); \
	for file in $(WIN32SOURCES); do \
		echo "<File RelativePath=\""$$file"\"></File>" $(VCPROJOUT); \
	done; \
	echo "</Filter><Filter  Name=\"Header Files\">" $(VCPROJOUT); \
	for file in $(WIN32HEADERS); do \
		echo "<File RelativePath=\""$$file"\"></File>" $(VCPROJOUT); \
	done; \
	cat $(srcdir)/msvc/vc8proj.foot $(VCPROJOUT) )
