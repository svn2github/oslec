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
## $Id: Makefile.am,v 1.77 2006/11/26 13:13:30 steveu Exp $

AM_CFLAGS = $(COMP_VENDOR_CFLAGS)

LIBS += $(TESTLIBS)

nobase_data_DATA = sound_c1_8k.wav sound_c3_8k.wav

EXTRA_DIST = regression_tests.sh v42bis_tests.sh fax_tests.sh
 
MAINTAINERCLEANFILES = Makefile.in

INCLUDES = -I$(top_builddir)/src -DDATADIR="\"$(pkgdatadir)\""

noinst_PROGRAMS =   adsi_tests \
                    async_tests \
                    at_interpreter_tests \
                    awgn_tests \
                    bell_mf_rx_tests \
                    bell_mf_tx_tests \
                    bert_tests \
                    bit_operations_tests \
                    dc_restore_tests \
                    dds_tests \
                    dtmf_rx_tests \
                    dtmf_tx_tests \
                    echo_tests \
                    fax_decode \
                    fax_tests \
                    fsk_tests \
                    g168_tests \
                    g711_tests \
                    g722_tests \
                    g726_tests \
                    gsm0610_tests \
                    hdlc_tests \
                    ima_adpcm_tests \
                    line_model_tests \
                    logging_tests \
                    lpc10_tests \
                    make_g168_css \
                    make_line_models \
                    modem_echo_tests \
                    modem_connect_tones_tests \
                    noise_tests \
                    oki_adpcm_tests \
                    playout_tests \
                    plc_tests \
                    power_meter_tests \
                    r2_mf_rx_tests \
                    r2_mf_tx_tests \
                    schedule_tests \
                    sig_tone_tests \
                    super_tone_rx_tests \
                    super_tone_tx_tests \
                    t4_tests \
                    t31_tests \
                    t38_gateway_tests \
                    t38_gateway_to_terminal_tests \
                    t38_terminal_tests \
                    t38_terminal_to_gateway_tests \
                    time_scale_tests \
                    tone_generate_tests \
                    v17_tests \
                    v22bis_tests \
                    v27ter_tests \
                    v29_tests \
                    v42_tests \
                    v42bis_tests \
                    v8_tests \
                    vector_float_tests \
                    vector_int_tests \
                    testadsi \
                    testfax

noinst_HEADERS =    modem_monitor.h \
                    echo_monitor.h \
                    ip_network_model.h \
                    line_model.h \
                    line_model_monitor.h \
                    test_utils.h

adsi_tests_SOURCES = adsi_tests.c
adsi_tests_LDADD = -L$(top_builddir)/src -lspandsp

async_tests_SOURCES = async_tests.c
async_tests_LDADD = -L$(top_builddir)/src -lspandsp

at_interpreter_tests_SOURCES = at_interpreter_tests.c
at_interpreter_tests_LDADD = -L$(top_builddir)/src -lspandsp

awgn_tests_SOURCES = awgn_tests.c
awgn_tests_LDADD = -L$(top_builddir)/src -lspandsp

bell_mf_rx_tests_SOURCES = bell_mf_rx_tests.c
bell_mf_rx_tests_LDADD = -L$(top_builddir)/src -lspandsp

bell_mf_tx_tests_SOURCES = bell_mf_tx_tests.c
bell_mf_tx_tests_LDADD = -L$(top_builddir)/src -lspandsp

bert_tests_SOURCES = bert_tests.c
bert_tests_LDADD = -L$(top_builddir)/src -lspandsp

bit_operations_tests_SOURCES = bit_operations_tests.c
bit_operations_tests_LDADD = -L$(top_builddir)/src -lspandsp

dc_restore_tests_SOURCES = dc_restore_tests.c
dc_restore_tests_LDADD = -L$(top_builddir)/src -lspandsp

dds_tests_SOURCES = dds_tests.c
dds_tests_LDADD = -L$(top_builddir)/src -lspandsp

dtmf_rx_tests_SOURCES = dtmf_rx_tests.c test_utils.c
dtmf_rx_tests_LDADD = -L$(top_builddir)/src -lspandsp

dtmf_tx_tests_SOURCES = dtmf_tx_tests.c
dtmf_tx_tests_LDADD = -L$(top_builddir)/src -lspandsp

echo_tests_SOURCES = echo_tests.c echo_monitor.cpp
echo_tests_LDADD = -L$(top_builddir)/src -lspandsp

fax_decode_SOURCES = fax_decode.c
fax_decode_LDADD = -L$(top_builddir)/src -lspandsp

fax_tests_SOURCES = fax_tests.c
fax_tests_LDADD = -L$(top_builddir)/src -lspandsp

fsk_tests_SOURCES = fsk_tests.c line_model.c test_utils.c
fsk_tests_LDADD = -L$(top_builddir)/src -lspandsp

g168_tests_SOURCES = g168_tests.c
g168_tests_LDADD = -L$(top_builddir)/src -lspandsp

g711_tests_SOURCES = g711_tests.c
g711_tests_LDADD = -L$(top_builddir)/src -lspandsp

g722_tests_SOURCES = g722_tests.c
g722_tests_LDADD = -L$(top_builddir)/src -lspandsp

g726_tests_SOURCES = g726_tests.c
g726_tests_LDADD = -L$(top_builddir)/src -lspandsp

gsm0610_tests_SOURCES = gsm0610_tests.c
gsm0610_tests_LDADD = -L$(top_builddir)/src -lspandsp

hdlc_tests_SOURCES = hdlc_tests.c
hdlc_tests_LDADD = -L$(top_builddir)/src -lspandsp

ima_adpcm_tests_SOURCES = ima_adpcm_tests.c
ima_adpcm_tests_LDADD = -L$(top_builddir)/src -lspandsp

line_model_tests_SOURCES = line_model_tests.c line_model.c test_utils.c
line_model_tests_LDADD = -L$(top_builddir)/src -lspandsp

logging_tests_SOURCES = logging_tests.c
logging_tests_LDADD = -L$(top_builddir)/src -lspandsp

lpc10_tests_SOURCES = lpc10_tests.c
lpc10_tests_LDADD = -L$(top_builddir)/src -lspandsp

make_g168_css_SOURCES = make_g168_css.c
make_g168_css_LDADD = -L$(top_builddir)/src -lspandsp

make_line_models_SOURCES = make_line_models.c
make_line_models_LDADD = -L$(top_builddir)/src -lspandsp

modem_echo_tests_SOURCES = modem_echo_tests.c echo_monitor.cpp
modem_echo_tests_LDADD = -L$(top_builddir)/src -lspandsp

modem_connect_tones_tests_SOURCES = modem_connect_tones_tests.c
modem_connect_tones_tests_LDADD = -L$(top_builddir)/src -lspandsp

noise_tests_SOURCES = noise_tests.c
noise_tests_LDADD = -L$(top_builddir)/src -lspandsp

oki_adpcm_tests_SOURCES = oki_adpcm_tests.c
oki_adpcm_tests_LDADD = -L$(top_builddir)/src -lspandsp

playout_tests_SOURCES = playout_tests.c
playout_tests_LDADD = -L$(top_builddir)/src -lspandsp

plc_tests_SOURCES = plc_tests.c
plc_tests_LDADD = -L$(top_builddir)/src -lspandsp

power_meter_tests_SOURCES = power_meter_tests.c
power_meter_tests_LDADD = -L$(top_builddir)/src -lspandsp

r2_mf_rx_tests_SOURCES = r2_mf_rx_tests.c
r2_mf_rx_tests_LDADD = -L$(top_builddir)/src -lspandsp

r2_mf_tx_tests_SOURCES = r2_mf_tx_tests.c
r2_mf_tx_tests_LDADD = -L$(top_builddir)/src -lspandsp

schedule_tests_SOURCES = schedule_tests.c
schedule_tests_LDADD = -L$(top_builddir)/src -lspandsp 

sig_tone_tests_SOURCES = sig_tone_tests.c
sig_tone_tests_LDADD = -L$(top_builddir)/src -lspandsp 

super_tone_rx_tests_SOURCES = super_tone_rx_tests.c
super_tone_rx_tests_LDADD = -L$(top_builddir)/src -lspandsp

super_tone_tx_tests_SOURCES = super_tone_tx_tests.c
super_tone_tx_tests_LDADD = -L$(top_builddir)/src -lspandsp

t4_tests_SOURCES = t4_tests.c
t4_tests_LDADD = -L$(top_builddir)/src -lspandsp

t31_tests_SOURCES = t31_tests.c
t31_tests_LDADD = -L$(top_builddir)/src -lspandsp

t38_gateway_tests_SOURCES = t38_gateway_tests.c ip_network_model.c
t38_gateway_tests_LDADD = -L$(top_builddir)/src -lspandsp

t38_gateway_to_terminal_tests_SOURCES = t38_gateway_to_terminal_tests.c ip_network_model.c
t38_gateway_to_terminal_tests_LDADD = -L$(top_builddir)/src -lspandsp

t38_terminal_tests_SOURCES = t38_terminal_tests.c ip_network_model.c
t38_terminal_tests_LDADD = -L$(top_builddir)/src -lspandsp

t38_terminal_to_gateway_tests_SOURCES = t38_terminal_to_gateway_tests.c ip_network_model.c
t38_terminal_to_gateway_tests_LDADD = -L$(top_builddir)/src -lspandsp

time_scale_tests_SOURCES = time_scale_tests.c
time_scale_tests_LDADD = -L$(top_builddir)/src -lspandsp

tone_generate_tests_SOURCES = tone_generate_tests.c
tone_generate_tests_LDADD = -L$(top_builddir)/src -lspandsp

v17_tests_SOURCES = v17_tests.c line_model.c test_utils.c line_model_monitor.cpp modem_monitor.cpp
v17_tests_LDADD = -L$(top_builddir)/src -lspandsp

v22bis_tests_SOURCES = v22bis_tests.c line_model.c test_utils.c line_model_monitor.cpp modem_monitor.cpp
v22bis_tests_LDADD = -L$(top_builddir)/src -lspandsp

v27ter_tests_SOURCES = v27ter_tests.c line_model.c test_utils.c line_model_monitor.cpp modem_monitor.cpp
v27ter_tests_LDADD = -L$(top_builddir)/src -lspandsp

v29_tests_SOURCES = v29_tests.c line_model.c test_utils.c line_model_monitor.cpp modem_monitor.cpp
v29_tests_LDADD = -L$(top_builddir)/src -lspandsp

v42_tests_SOURCES = v42_tests.c
v42_tests_LDADD = -L$(top_builddir)/src -lspandsp

v42bis_tests_SOURCES = v42bis_tests.c
v42bis_tests_LDADD = -L$(top_builddir)/src -lspandsp

v8_tests_SOURCES = v8_tests.c
v8_tests_LDADD = -L$(top_builddir)/src -lspandsp

vector_float_tests_SOURCES = vector_float_tests.c
vector_float_tests_LDADD = -L$(top_builddir)/src -lspandsp

vector_int_tests_SOURCES = vector_int_tests.c
vector_int_tests_LDADD = -L$(top_builddir)/src -lspandsp

testadsi_SOURCES = testadsi.c
testadsi_LDADD = -L$(top_builddir)/src -lspandsp

testfax_SOURCES = testfax.c
testfax_LDADD = -L$(top_builddir)/src -lspandsp

# We need to run make_line_models, so it generates the line_models.h file
# used by several of the test programs.

line_model.$(OBJEXT): make_line_models$(EXEEXT) line_models.h

line_models.h: make_line_models$(EXEEXT)
	./make_line_models$(EXEEXT)
    
# We need to create the CSS files for echo cancellation tests.

sound_c1_8k.wav sound_c3_8k.wav: make_g168_css$(EXEEXT)
	./make_g168_css$(EXEEXT)
	sox sound_c1.wav -r8000 sound_c1_8k.wav
	sox sound_c3.wav -r8000 sound_c3_8k.wav
	rm sound_c1.wav sound_c3.wav
