#pragma once

#include "consumer_metadata.h"
#include "spill.h"
#include <initializer_list>
#include <boost/thread.hpp>
#include "H5CC_Group.h"

#include "dataspace.h"

namespace DAQuiri {

class Consumer
{
protected:
  ConsumerMetadata metadata_;
  std::vector<DataAxis> axes_;

  mutable boost::shared_mutex shared_mutex_;
  mutable boost::mutex unique_mutex_;
  bool changed_;

public:
  Consumer();
  Consumer(const Consumer& other)
    : metadata_(other.metadata_)
    , axes_ (other.axes_) {}
  virtual Consumer* clone() const = 0;
  virtual ~Consumer() {}

  //named constructors, used by factory
  bool from_prototype(const ConsumerMetadata&);

  virtual bool load(H5CC::Group&, bool withdata);
  virtual bool save(H5CC::Group&) const;

  //data acquisition
  void push_spill(const Spill&);
  void flush();

  //get count at coordinates in n-dimensional list
  PreciseFloat data(std::initializer_list<size_t> list = {}) const;

  //optimized retrieval of bulk data as list of Entries
  //parameters take dimensions_number of ranges (inclusive)
  std::unique_ptr<EntryList> data_range(std::initializer_list<Pair> list = {});
  void append(const Entry&);

  //retrieve axis-values for given dimension (can be precalculated energies)
  DataAxis axis(uint16_t dimension) const;

  //ConsumerMetadata
  ConsumerMetadata metadata() const;
  void reset_changed();
  bool changed() const;

  //Convenience functions for most common metadata
  std::string type() const;
  uint16_t dimensions() const;
  std::string debug(std::string prepend = "", bool verbose = true) const;

  //Change metadata
  void set_attribute(const Setting &setting, bool greedy = false);
  void set_attributes(const Setting &settings);
  void set_detectors(const std::vector<Detector>& dets);

protected:
  //////////////////////////////////////////
  //////////THIS IS THE MEAT////////////////
  ///implement these to make custom types///
  //////////////////////////////////////////
  
  virtual std::string my_type() const = 0;
  virtual bool _initialize();
  virtual void _init_from_file(std::string name);
  virtual void _recalc_axes() = 0;

  virtual void _set_detectors(const std::vector<Detector>& dets) = 0;
  virtual void _push_spill(const Spill&);
  virtual void _push_event(const Event&) = 0;
  virtual void _push_stats(const Status&) = 0;
  virtual void _flush() {}

  virtual PreciseFloat _data(std::initializer_list<size_t>) const {return 0;}
  virtual std::unique_ptr<std::list<Entry>> _data_range(std::initializer_list<Pair>)
    { return std::unique_ptr<std::list<Entry>>(new std::list<Entry>); }
  virtual void _append(const Entry&) {}

  virtual void _load_data(H5CC::Group&) {}
  virtual void _save_data(H5CC::Group&) const {}
  virtual std::string _data_debug(const std::string& prepend) const
  { return std::string(); }
};

typedef std::shared_ptr<Consumer> ConsumerPtr;

}
