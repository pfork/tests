#!/home/stef/tasks/sdr/env/bin/python
##################################################
# Gnuradio Python Flow Graph
# Title: Top Block
# Generated: Sun Mar 22 00:53:42 2015
##################################################

from gnuradio import analog
from gnuradio import blocks
from gnuradio import digital
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
        self.samp_rate = samp_rate = 1024e3
        self.data_rate = data_rate = 9600
        self.channel_spacing = channel_spacing = 500000
        self.width = width = 150000
        self.tuner = tuner = 430.24e6
        self.squelch = squelch = -25
        self.samp_per_sym = samp_per_sym = int(samp_rate / data_rate)
        self.offset = offset = -0.2
        self.freq_offset = freq_offset = (channel_spacing / 2) + (channel_spacing * .1)
        self.demodgain = demodgain = 4
        self.cutoff = cutoff = 60000

        ##################################################
        # Blocks
        ##################################################
        _width_sizer = wx.BoxSizer(wx.VERTICAL)
        self._width_text_box = forms.text_box(
        	parent=self.GetWin(),
        	sizer=_width_sizer,
        	value=self.width,
        	callback=self.set_width,
        	label='width',
        	converter=forms.float_converter(),
        	proportion=0,
        )
        self._width_slider = forms.slider(
        	parent=self.GetWin(),
        	sizer=_width_sizer,
        	value=self.width,
        	callback=self.set_width,
        	minimum=0,
        	maximum=400000,
        	num_steps=100,
        	style=wx.SL_HORIZONTAL,
        	cast=float,
        	proportion=1,
        )
        self.GridAdd(_width_sizer, 1, 6, 1, 1)
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
        	minimum=430e6,
        	maximum=440e6,
        	num_steps=1000,
        	style=wx.SL_HORIZONTAL,
        	cast=float,
        	proportion=1,
        )
        self.GridAdd(_tuner_sizer, 1, 1, 1, 1)
        _squelch_sizer = wx.BoxSizer(wx.VERTICAL)
        self._squelch_text_box = forms.text_box(
        	parent=self.GetWin(),
        	sizer=_squelch_sizer,
        	value=self.squelch,
        	callback=self.set_squelch,
        	label='squelch',
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
        self.GridAdd(_squelch_sizer, 1, 3, 1, 1)
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
        	minimum=-1,
        	maximum=42,
        	num_steps=44,
        	style=wx.SL_HORIZONTAL,
        	cast=float,
        	proportion=1,
        )
        self.GridAdd(_demodgain_sizer, 1, 2, 1, 1)
        _cutoff_sizer = wx.BoxSizer(wx.VERTICAL)
        self._cutoff_text_box = forms.text_box(
        	parent=self.GetWin(),
        	sizer=_cutoff_sizer,
        	value=self.cutoff,
        	callback=self.set_cutoff,
        	label='cutoff',
        	converter=forms.float_converter(),
        	proportion=0,
        )
        self._cutoff_slider = forms.slider(
        	parent=self.GetWin(),
        	sizer=_cutoff_sizer,
        	value=self.cutoff,
        	callback=self.set_cutoff,
        	minimum=0,
        	maximum=400000,
        	num_steps=100,
        	style=wx.SL_HORIZONTAL,
        	cast=float,
        	proportion=1,
        )
        self.GridAdd(_cutoff_sizer, 1, 5, 1, 1)
        self.wxgui_scopesink2_0_0 = scopesink2.scope_sink_f(
        	self.GetWin(),
        	title="Scope Plot 1",
        	sample_rate=samp_rate,
        	v_scale=1,
        	v_offset=0.5,
        	t_scale=0.02,
        	ac_couple=False,
        	xy_mode=False,
        	num_inputs=2,
        	trig_mode=wxgui.TRIG_MODE_AUTO,
        	y_axis_label="Counts",
        )
        self.Add(self.wxgui_scopesink2_0_0.win)
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
        self.osmosdr_source_c_0_1 = osmosdr.source()
        self.osmosdr_source_c_0_1.set_sample_rate(samp_rate)
        self.osmosdr_source_c_0_1.set_center_freq(tuner+freq_offset, 0)
        self.osmosdr_source_c_0_1.set_freq_corr(21, 0)
        self.osmosdr_source_c_0_1.set_iq_balance_mode(0, 0)
        self.osmosdr_source_c_0_1.set_gain_mode(0, 0)
        self.osmosdr_source_c_0_1.set_gain(10, 0)
        self.osmosdr_source_c_0_1.set_if_gain(24, 0)
        self.osmosdr_source_c_0_1.set_bb_gain(20, 0)
        self.osmosdr_source_c_0_1.set_antenna("", 0)
        self.osmosdr_source_c_0_1.set_bandwidth(0, 0)
          
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
        self.GridAdd(_offset_sizer, 1, 4, 1, 1)
        self.freq_xlating_fir_filter_xxx_0_1 = filter.freq_xlating_fir_filter_ccc(1, (firdes.low_pass(1, samp_rate,cutoff, width,  firdes.WIN_BLACKMAN, 6.76)), -freq_offset, samp_rate)
        self.digital_clock_recovery_mm_xx_0 = digital.clock_recovery_mm_ff(samp_per_sym*(1+0.0), 0.25*0.175*0.175, 0.5, 0.175, 0.005)
        self.digital_binary_slicer_fb_1 = digital.binary_slicer_fb()
        self.blocks_char_to_float_0 = blocks.char_to_float(1, 1)
        self.analog_simple_squelch_cc_0 = analog.simple_squelch_cc(squelch, 1)
        self.analog_quadrature_demod_cf_0 = analog.quadrature_demod_cf(demodgain)

        ##################################################
        # Connections
        ##################################################
        self.connect((self.digital_binary_slicer_fb_1, 0), (self.blocks_char_to_float_0, 0))
        self.connect((self.digital_clock_recovery_mm_xx_0, 0), (self.digital_binary_slicer_fb_1, 0))
        self.connect((self.osmosdr_source_c_0_1, 0), (self.freq_xlating_fir_filter_xxx_0_1, 0))
        self.connect((self.blocks_char_to_float_0, 0), (self.wxgui_scopesink2_0_0, 0))
        self.connect((self.analog_quadrature_demod_cf_0, 0), (self.wxgui_scopesink2_0_0, 1))
        self.connect((self.analog_simple_squelch_cc_0, 0), (self.analog_quadrature_demod_cf_0, 0))
        self.connect((self.analog_quadrature_demod_cf_0, 0), (self.digital_clock_recovery_mm_xx_0, 0))
        self.connect((self.freq_xlating_fir_filter_xxx_0_1, 0), (self.analog_simple_squelch_cc_0, 0))
        self.connect((self.freq_xlating_fir_filter_xxx_0_1, 0), (self.wxgui_fftsink2_0, 0))



    def get_samp_rate(self):
        return self.samp_rate

    def set_samp_rate(self, samp_rate):
        self.samp_rate = samp_rate
        self.set_samp_per_sym(int(self.samp_rate / self.data_rate))
        self.osmosdr_source_c_0_1.set_sample_rate(self.samp_rate)
        self.wxgui_scopesink2_0_0.set_sample_rate(self.samp_rate)
        self.wxgui_fftsink2_0.set_sample_rate(self.samp_rate)
        self.freq_xlating_fir_filter_xxx_0_1.set_taps((firdes.low_pass(1, self.samp_rate,self.cutoff, self.width,  firdes.WIN_BLACKMAN, 6.76)))

    def get_data_rate(self):
        return self.data_rate

    def set_data_rate(self, data_rate):
        self.data_rate = data_rate
        self.set_samp_per_sym(int(self.samp_rate / self.data_rate))

    def get_channel_spacing(self):
        return self.channel_spacing

    def set_channel_spacing(self, channel_spacing):
        self.channel_spacing = channel_spacing
        self.set_freq_offset((self.channel_spacing / 2) + (self.channel_spacing * .1))

    def get_width(self):
        return self.width

    def set_width(self, width):
        self.width = width
        self._width_slider.set_value(self.width)
        self._width_text_box.set_value(self.width)
        self.freq_xlating_fir_filter_xxx_0_1.set_taps((firdes.low_pass(1, self.samp_rate,self.cutoff, self.width,  firdes.WIN_BLACKMAN, 6.76)))

    def get_tuner(self):
        return self.tuner

    def set_tuner(self, tuner):
        self.tuner = tuner
        self._tuner_slider.set_value(self.tuner)
        self._tuner_text_box.set_value(self.tuner)
        self.osmosdr_source_c_0_1.set_center_freq(self.tuner+self.freq_offset, 0)

    def get_squelch(self):
        return self.squelch

    def set_squelch(self, squelch):
        self.squelch = squelch
        self._squelch_slider.set_value(self.squelch)
        self._squelch_text_box.set_value(self.squelch)
        self.analog_simple_squelch_cc_0.set_threshold(self.squelch)

    def get_samp_per_sym(self):
        return self.samp_per_sym

    def set_samp_per_sym(self, samp_per_sym):
        self.samp_per_sym = samp_per_sym
        self.digital_clock_recovery_mm_xx_0.set_omega(self.samp_per_sym*(1+0.0))

    def get_offset(self):
        return self.offset

    def set_offset(self, offset):
        self.offset = offset
        self._offset_slider.set_value(self.offset)
        self._offset_text_box.set_value(self.offset)

    def get_freq_offset(self):
        return self.freq_offset

    def set_freq_offset(self, freq_offset):
        self.freq_offset = freq_offset
        self.osmosdr_source_c_0_1.set_center_freq(self.tuner+self.freq_offset, 0)
        self.freq_xlating_fir_filter_xxx_0_1.set_center_freq(-self.freq_offset)

    def get_demodgain(self):
        return self.demodgain

    def set_demodgain(self, demodgain):
        self.demodgain = demodgain
        self._demodgain_slider.set_value(self.demodgain)
        self._demodgain_text_box.set_value(self.demodgain)
        self.analog_quadrature_demod_cf_0.set_gain(self.demodgain)

    def get_cutoff(self):
        return self.cutoff

    def set_cutoff(self, cutoff):
        self.cutoff = cutoff
        self._cutoff_slider.set_value(self.cutoff)
        self._cutoff_text_box.set_value(self.cutoff)
        self.freq_xlating_fir_filter_xxx_0_1.set_taps((firdes.low_pass(1, self.samp_rate,self.cutoff, self.width,  firdes.WIN_BLACKMAN, 6.76)))

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
