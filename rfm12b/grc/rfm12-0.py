#!/home/stef/tasks/sdr/env/bin/python
##################################################
# Gnuradio Python Flow Graph
# Title: Top Block
# Generated: Fri Mar 20 04:05:10 2015
##################################################

from gnuradio import analog
from gnuradio import eng_notation
from gnuradio import filter
from gnuradio import gr
from gnuradio import wxgui
from gnuradio.eng_option import eng_option
from gnuradio.fft import window
from gnuradio.filter import firdes
from gnuradio.wxgui import fftsink2
from gnuradio.wxgui import forms
from gnuradio.wxgui import scopesink2
from grc_gnuradio import wxgui as grc_wxgui
from optparse import OptionParser
import math
import osmosdr
import wx

class top_block(grc_wxgui.top_block_gui):

    def __init__(self):
        grc_wxgui.top_block_gui.__init__(self, title="Top Block")
        _icon_path = "/usr/share/icons/hicolor/32x32/apps/gnuradio-grc.png"
        self.SetIcon(wx.Icon(_icon_path, wx.BITMAP_TYPE_ANY))

        ##################################################
        # Variables
        ##################################################
        self.tuner = tuner = 431.57e6
        self.squelch = squelch = -25
        self.samp_rate = samp_rate = 200e3
        self.offset = offset = 0
        self.demodgain = demodgain = 5

        ##################################################
        # Blocks
        ##################################################
        _tuner_sizer = wx.BoxSizer(wx.VERTICAL)
        self._tuner_text_box = forms.text_box(
        	parent=self.GetWin(),
        	sizer=_tuner_sizer,
        	value=self.tuner,
        	callback=self.set_tuner,
        	label='tuner',
        	converter=forms.float_converter(),
        	proportion=0,
        )
        self._tuner_slider = forms.slider(
        	parent=self.GetWin(),
        	sizer=_tuner_sizer,
        	value=self.tuner,
        	callback=self.set_tuner,
        	minimum=431e6,
        	maximum=434e6,
        	num_steps=1000,
        	style=wx.SL_HORIZONTAL,
        	cast=float,
        	proportion=1,
        )
        self.Add(_tuner_sizer)
        _squelch_sizer = wx.BoxSizer(wx.VERTICAL)
        self._squelch_text_box = forms.text_box(
        	parent=self.GetWin(),
        	sizer=_squelch_sizer,
        	value=self.squelch,
        	callback=self.set_squelch,
        	label="squelt",
        	converter=forms.float_converter(),
        	proportion=0,
        )
        self._squelch_slider = forms.slider(
        	parent=self.GetWin(),
        	sizer=_squelch_sizer,
        	value=self.squelch,
        	callback=self.set_squelch,
        	minimum=-50,
        	maximum=20,
        	num_steps=100,
        	style=wx.SL_HORIZONTAL,
        	cast=float,
        	proportion=1,
        )
        self.Add(_squelch_sizer)
        _demodgain_sizer = wx.BoxSizer(wx.VERTICAL)
        self._demodgain_text_box = forms.text_box(
        	parent=self.GetWin(),
        	sizer=_demodgain_sizer,
        	value=self.demodgain,
        	callback=self.set_demodgain,
        	label='demodgain',
        	converter=forms.float_converter(),
        	proportion=0,
        )
        self._demodgain_slider = forms.slider(
        	parent=self.GetWin(),
        	sizer=_demodgain_sizer,
        	value=self.demodgain,
        	callback=self.set_demodgain,
        	minimum=1,
        	maximum=10,
        	num_steps=100,
        	style=wx.SL_HORIZONTAL,
        	cast=float,
        	proportion=1,
        )
        self.Add(_demodgain_sizer)
        self.wxgui_scopesink2_1 = scopesink2.scope_sink_f(
        	self.GetWin(),
        	title="Scope Plot",
        	sample_rate=samp_rate,
        	v_scale=0,
        	v_offset=0,
        	t_scale=0,
        	ac_couple=False,
        	xy_mode=False,
        	num_inputs=1,
        	trig_mode=wxgui.TRIG_MODE_AUTO,
        	y_axis_label="Counts",
        )
        self.Add(self.wxgui_scopesink2_1.win)
        self.wxgui_fftsink2_0 = fftsink2.fft_sink_c(
        	self.GetWin(),
        	baseband_freq=0,
        	y_per_div=10,
        	y_divs=10,
        	ref_level=0,
        	ref_scale=2.0,
        	sample_rate=samp_rate,
        	fft_size=1024,
        	fft_rate=15,
        	average=False,
        	avg_alpha=None,
        	title="FFT Plot",
        	peak_hold=False,
        )
        self.Add(self.wxgui_fftsink2_0.win)
        self.osmosdr_source_c_0 = osmosdr.source()
        self.osmosdr_source_c_0.set_sample_rate(samp_rate)
        self.osmosdr_source_c_0.set_center_freq(tuner, 0)
        self.osmosdr_source_c_0.set_freq_corr(21, 0)
        self.osmosdr_source_c_0.set_iq_balance_mode(0, 0)
        self.osmosdr_source_c_0.set_gain_mode(0, 0)
        self.osmosdr_source_c_0.set_gain(10, 0)
        self.osmosdr_source_c_0.set_if_gain(24, 0)
        self.osmosdr_source_c_0.set_bb_gain(20, 0)
        self.osmosdr_source_c_0.set_antenna("", 0)
        self.osmosdr_source_c_0.set_bandwidth(0, 0)
          
        _offset_sizer = wx.BoxSizer(wx.VERTICAL)
        self._offset_text_box = forms.text_box(
        	parent=self.GetWin(),
        	sizer=_offset_sizer,
        	value=self.offset,
        	callback=self.set_offset,
        	label='offset',
        	converter=forms.float_converter(),
        	proportion=0,
        )
        self._offset_slider = forms.slider(
        	parent=self.GetWin(),
        	sizer=_offset_sizer,
        	value=self.offset,
        	callback=self.set_offset,
        	minimum=-1,
        	maximum=1,
        	num_steps=100,
        	style=wx.SL_HORIZONTAL,
        	cast=float,
        	proportion=1,
        )
        self.Add(_offset_sizer)
        self.low_pass_filter_0 = filter.fir_filter_ccf(1, firdes.low_pass(
        	1, samp_rate, 40e3, 10000, firdes.WIN_HAMMING, 6.76))
        self.analog_simple_squelch_cc_0 = analog.simple_squelch_cc(squelch, 1)
        self.analog_quadrature_demod_cf_0 = analog.quadrature_demod_cf(demodgain)

        fd=open('/tmp/rfm12','w')
        pipe_sink = blocks.file_sink(gr.sizeof_char*1, fd)
        pipe_sink.set_unbuffered(False)

        ##################################################
        # Connections
        ##################################################
        self.connect((self.low_pass_filter_0, 0), (self.analog_simple_squelch_cc_0, 0))
        self.connect((self.analog_simple_squelch_cc_0, 0), (self.analog_quadrature_demod_cf_0, 0))
        self.connect((self.osmosdr_source_c_0, 0), (self.low_pass_filter_0, 0))
        self.connect((self.analog_simple_squelch_cc_0, 0), (self.wxgui_fftsink2_0, 0))
        self.connect((self.analog_quadrature_demod_cf_0, 0), (self.wxgui_scopesink2_1, 0))


    def get_tuner(self):
        return self.tuner

    def set_tuner(self, tuner):
        self.tuner = tuner
        self._tuner_slider.set_value(self.tuner)
        self._tuner_text_box.set_value(self.tuner)
        self.osmosdr_source_c_0.set_center_freq(self.tuner, 0)

    def get_squelch(self):
        return self.squelch

    def set_squelch(self, squelch):
        self.squelch = squelch
        self._squelch_slider.set_value(self.squelch)
        self._squelch_text_box.set_value(self.squelch)
        self.analog_simple_squelch_cc_0.set_threshold(self.squelch)

    def get_samp_rate(self):
        return self.samp_rate

    def set_samp_rate(self, samp_rate):
        self.samp_rate = samp_rate
        self.low_pass_filter_0.set_taps(firdes.low_pass(1, self.samp_rate, 40e3, 10000, firdes.WIN_HAMMING, 6.76))
        self.wxgui_fftsink2_0.set_sample_rate(self.samp_rate)
        self.osmosdr_source_c_0.set_sample_rate(self.samp_rate)
        self.wxgui_scopesink2_1.set_sample_rate(self.samp_rate)

    def get_offset(self):
        return self.offset

    def set_offset(self, offset):
        self.offset = offset
        self._offset_slider.set_value(self.offset)
        self._offset_text_box.set_value(self.offset)

    def get_demodgain(self):
        return self.demodgain

    def set_demodgain(self, demodgain):
        self.demodgain = demodgain
        self._demodgain_slider.set_value(self.demodgain)
        self._demodgain_text_box.set_value(self.demodgain)
        self.analog_quadrature_demod_cf_0.set_gain(self.demodgain)

if __name__ == '__main__':
    import ctypes
    import sys
    if sys.platform.startswith('linux'):
        try:
            x11 = ctypes.cdll.LoadLibrary('libX11.so')
            x11.XInitThreads()
        except:
            print "Warning: failed to XInitThreads()"
    parser = OptionParser(option_class=eng_option, usage="%prog: [options]")
    (options, args) = parser.parse_args()
    tb = top_block()
    tb.Start(True)
    tb.Wait()
