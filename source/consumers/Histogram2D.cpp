#include <consumers/Histogram2D.h>

#include <consumers/dataspaces/SparseMap2D.h>
#include <consumers/dataspaces/SparseMatrix2D.h>
#include <consumers/dataspaces/DenseMatrix2D.h>

#include <core/util/logger.h>

namespace DAQuiri {

Histogram2D::Histogram2D()
    : Spectrum()
{
    //INFO("Histogram2D ctor");
//  data_ = std::make_shared<SparseMap2D>();
  data_ = std::make_shared<SparseMatrix2D>();
//  data_ = std::make_shared<DenseMatrix2D>();

  Setting base_options = metadata_.attributes();
  metadata_ = ConsumerMetadata(my_type(), "Event-based 2D spectrum");

  SettingMeta app("appearance", SettingType::text, "Appearance");
  app.set_flag("gradient-name");
  base_options.branches.add(app);

  SettingMeta flip("flip-y", SettingType::boolean, "Flip Y axis");
  base_options.branches.add(flip);

  base_options.branches.add_a(value_latch_x_.settings(0, "X value"));
  base_options.branches.add_a(value_latch_y_.settings(1, "Y value"));

  metadata_.overwrite_all_attributes(base_options);
}

void Histogram2D::_apply_attributes()
{
  Spectrum::_apply_attributes();

  value_latch_x_.settings(metadata_.get_attribute(value_latch_x_.settings(0, "X value")));
  metadata_.replace_attribute(value_latch_x_.settings(0, "X value"));

  value_latch_y_.settings(metadata_.get_attribute(value_latch_y_.settings(1, "Y value")));
  metadata_.replace_attribute(value_latch_y_.settings(1, "Y value"));
}

void Histogram2D::_recalc_axes()
{
  Detector det0, det1;
  if (data_->dimensions() == metadata_.detectors.size())
  {
    det0 = metadata_.detectors[0];
    det1 = metadata_.detectors[1];
  }

  auto calib0 = det0.get_calibration({value_latch_x_.value_id, det0.id()}, {value_latch_x_.value_id});
  data_->set_axis(0, DataAxis(calib0, value_latch_x_.downsample));

  auto calib1 = det1.get_calibration({value_latch_y_.value_id, det1.id()}, {value_latch_y_.value_id});
  data_->set_axis(1, DataAxis(calib1, value_latch_y_.downsample));

  data_->recalc_axes();
}

bool Histogram2D::_accept_spill(const Spill& spill)
{
  return (Spectrum::_accept_spill(spill) &&
      value_latch_x_.has_declared_value(spill) &&
      value_latch_y_.has_declared_value(spill));
}

void Histogram2D::_push_stats_pre(const Spill& spill)
{
  if (!this->_accept_spill(spill)) {
    return;
  }
  value_latch_x_.configure(spill);
  value_latch_y_.configure(spill);
  Spectrum::_push_stats_pre(spill);
}

bool Histogram2D::_accept_events(const Spill& /*spill*/)
{
  return value_latch_x_.valid() && value_latch_y_.valid();
}

void Histogram2D::_push_event(const Event& event)
{
  //INFO("<Histogram2D> _push_event()");
  if (!filters_.accept(event))
    return;
  value_latch_x_.extract(coords_[0], event);
  value_latch_y_.extract(coords_[1], event);
  //INFO("add_one event at ({},{})", coords_[0], coords_[1]);
  data_->add_one(coords_);
}


}
