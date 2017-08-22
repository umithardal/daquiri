#include "dataspace.h"
#include "h5json.h"
#include "ascii_tree.h"

#include "custom_timer.h"
#include "custom_logger.h"

#define SLEEP_TIME_MS 200

using shared_lock = boost::shared_lock<boost::shared_mutex>;
using unique_lock = boost::unique_lock<boost::shared_mutex>;

namespace DAQuiri {

DataAxis::DataAxis(Calibration c, size_t resolution)
{
  calibration = c;
  domain.resize(resolution);
  for (size_t i=0; i < resolution; ++i)
    domain[i] = calibration.transform(i);
}

DataAxis::DataAxis(Calibration c, size_t resolution, uint16_t bits)
{
  calibration = c;
  domain.resize(resolution);
  for (size_t i=0; i < resolution; ++i)
    domain[i] = calibration.transform(i, bits);
}

Pair DataAxis::bounds() const
{
  return Pair(0, domain.size());
}

std::string DataAxis::label() const
{
//  if (!calibration.valid())
//    return "undefined axis";
  std::stringstream ss;
  if (!calibration.to().value.empty())
    ss << calibration.to().value;
  else
    ss << calibration.from().value;
  if (!calibration.to().units.empty())
    ss << " (" << calibration.to().units << ")";
  else
    ss << " (" << calibration.from().bits << " bits)";
  return ss.str();
}

std::string DataAxis::debug() const
{
  std::stringstream ss;
  ss << "domain_size=" << domain.size();
  if (domain.size())
    ss << " [" << domain[0]
       << "-" << domain[domain.size()-1]
       << "]";
  if (calibration.valid())
    ss << " " << calibration.debug();
  return ss.str();
}


Dataspace::Dataspace()
{}

Dataspace::Dataspace(uint16_t dimensions)
  : dimensions_(dimensions)
{
  axes_.resize(dimensions);
}

Dataspace::Dataspace(const Dataspace& other)
  : dimensions_(other.dimensions_)
  , axes_ (other.axes_)
{}

PreciseFloat Dataspace::get(std::initializer_list<size_t> list) const
{
  shared_lock lock(mutex_);
  return this->_get(list);
}

EntryList Dataspace::range(std::initializer_list<Pair> list) const
{
  shared_lock lock(mutex_);
  return this->_range(list);
}

void Dataspace::add(const Entry& e)
{
  shared_lock lock(mutex_);
  this->_add(e);
}

DataAxis Dataspace::axis(uint16_t dimension) const
{
  shared_lock lock(mutex_);
  
  if (dimension < axes_.size())
    return axes_[dimension];
  else
    return DataAxis();
}

void Dataspace::set_axis(size_t dim, const DataAxis& ax)
{
  unique_lock lock(mutex_, boost::defer_lock);
  if (dim < axes_.size())
    axes_[dim] = ax;
//  else throw?
}


uint16_t Dataspace::dimensions() const
{
  shared_lock lock(mutex_);
  return _dimensions();
}

uint16_t Dataspace::_dimensions() const
{
  return dimensions_;
}

std::string Dataspace::debug(std::string prepend) const
{
  shared_lock lock(mutex_);
  std::stringstream ss;
  if (axes_.empty())
    ss << prepend << k_branch_mid_B << "Axes undefined\n";
  else
  {
    ss << prepend << k_branch_mid_B << "Axes:\n";
    for (size_t i=0; i < axes_.size();++i)
    {
      ss << prepend << k_branch_pre_B
         << (((i+1) == axes_.size()) ? k_branch_end_B : k_branch_mid_B)
         << i << "   " << axes_.at(i).debug()
         << "\n";
    }
  }
  ss << prepend << k_branch_end_B << "Data:\n"
     << this->_data_debug(prepend + "  ");
  return ss.str();
}

void Dataspace::load(H5CC::Group& g)
{
  unique_lock lock(mutex_, boost::defer_lock);
  while (!lock.try_lock())
    wait_ms(SLEEP_TIME_MS);
  this->_load(g);
}

void Dataspace::save(H5CC::Group& g) const
{
  shared_lock lock(mutex_);
  this->_save(g);
}

std::string Dataspace::_data_debug(const std::string&) const
{
  return std::string();
}

}