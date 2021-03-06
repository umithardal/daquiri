/* Copyright (C) 2016-2020 European Spallation Source, ERIC. See LICENSE file */
//===----------------------------------------------------------------------===//
///
/// \file Histogram2D.h
///
/// \brief key primitive for showing detector images
///
//===----------------------------------------------------------------------===//
#pragma once

#include <consumers/Spectrum.h>
#include <consumers/add_ons/ValueLatch.h>

namespace DAQuiri {

class Histogram2D : public Spectrum
{
  public:
    Histogram2D();
    Histogram2D* clone() const override { return new Histogram2D(*this); }

  protected:
    std::string my_type() const override { return "Histogram 2D"; }

    void _apply_attributes() override;
    void _recalc_axes() override;

    void _push_event(const Event&) override;
    void _push_stats_pre(const Spill& spill) override;

    bool _accept_spill(const Spill& spill) override;
    bool _accept_events(const Spill& spill) override;

    //cached parameters
    ValueLatch value_latch_x_;
    ValueLatch value_latch_y_;

    //reserve memory
    Coords coords_{0, 0};
};

}
