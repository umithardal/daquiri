#pragma once

#include "spectrum.h"

class TimeSpectrum : public Spectrum
{
public:
  TimeSpectrum();
  TimeSpectrum* clone() const override
  { return new TimeSpectrum(*this); }
  
protected:
  std::string my_type() const override {return "TimeSpectrum";}

  bool _initialize() override;
  void _init_from_file() override;
  void _set_detectors(const std::vector<Detector>& dets) override;
  void _recalc_axes() override;

  //event processing
  void _push_event(const Event&) override;
  void _push_stats(const Status&) override;
  bool channel_relevant(int16_t channel) const override;

  // cached parameters:
  uint16_t bits_ {0};
  uint16_t cutoff_ {0};
  Pattern channels_;
  std::string val_name_;

  //from status manifest
  std::vector<int> value_idx_;

  //data
  std::vector<std::vector<PreciseFloat>> spectra_;
  std::vector<PreciseFloat> seconds_;
  std::vector<Status> updates_;
};