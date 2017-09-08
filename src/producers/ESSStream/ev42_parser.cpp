#include "ev42_parser.h"
#include "ev42_events_generated.h"

#include "custom_logger.h"

ev42_events::ev42_events()
{
  std::string mp {"ev42_events/"};

  SettingMeta vc(mp + "DimensionCount", SettingType::integer, "Dimensions");
  vc.set_val("min", 1);
  vc.set_val("max", 16);
  add_definition(vc);

  SettingMeta valname(mp + "Dimension/Name", SettingType::text, "Dimension name");
  add_definition(valname);

  SettingMeta pc(mp + "Dimension/Extent", SettingType::integer, "Dimension extent");
  pc.set_val("min", 1);
  add_definition(pc);

  SettingMeta val(mp + "Dimension", SettingType::stem);
  val.set_enum(0, mp + "Dimension/Name");
  val.set_enum(1, mp + "Dimension/Extent");
  add_definition(val);

  SettingMeta root("ev42_events", SettingType::stem);
  root.set_flag("producer");
  root.set_enum(0, mp + "DimensionCount");
  add_definition(root);
}

void ev42_events::read_settings_bulk(Setting &set) const
{
  set = enrich_and_toggle_presets(set);

  auto dims = geometry_.names_.size();

  set.set(Setting::integer("ev42_events/DimensionCount", integer_t(dims)));

  while (set.branches.has_a(Setting({"ev42_events/Dimension", SettingType::stem})))
    set.branches.remove_a(Setting({"ev42_events/Dimension", SettingType::stem}));

  for (int i=0; i < int(dims); ++i)
  {
    Setting v = get_rich_setting("ev42_events/Dimension");
    v.set_indices({i});
    v.branches = get_rich_setting("ev42_events/Dimension").branches;
    std::string name;
    if (i < geometry_.names_.size())
    {
      name = geometry_.names_[i];
      v.set(Setting::text("ev42_events/Dimension/Name", name));
      v.set(Setting::integer("ev42_events/Dimension/Extent",
                             geometry_.dimensions_.at(name)));
    }
    for (auto& vv : v.branches)
      vv.set_indices({i});
    set.branches.add_a(v);
  }
}


void ev42_events::write_settings_bulk(const Setting& settings)
{
  auto set = enrich_and_toggle_presets(settings);

  auto dims = set.find({"ev42_events/DimensionCount"}).get_number();
  if ((dims < 1) || (dims > 16))
    dims = 1;

  geometry_ = GeometryInterpreter();
  for (Setting v : set.branches)
  {
    if (v.id() != "ev42_events/Dimension")
      continue;
    auto indices = v.indices();
    if (!indices.size())
      continue;
    size_t idx = *indices.begin();
    //    DBG << "Write idx " << idx;
    if (idx >= dims)
      continue;
    auto name = v.find({"ev42_events/Dimension/Name"}).get_text();
    size_t ext = v.find({"ev42_events/Dimension/Extent"}).get_number();
    geometry_.add_dimension(name, ext);
  }

  for (size_t i=geometry_.names_.size();
       i < dims; ++i)
    geometry_.add_dimension("", 1);
}

Spill* ev42_events::process_payload(void* msg, int16_t chan,
                                    TimeBase tb, uint64_t utime,
                                    PayloadStats &stats)
{
  Spill* ret {nullptr};
  auto em = GetEventMessage(msg);

  auto buf_id = em->message_id();
  if (buf_id < latest_buf_id_)
    WARN << "Buffer out of order (" << buf_id
         << "<" << latest_buf_id_ << ") "
         << debug(*em);
  latest_buf_id_ = std::max(latest_buf_id_, buf_id);

  std::string source_name = em->source_name()->str();

  auto t_len = em->time_of_flight()->Length();
  auto p_len = em->detector_id()->Length();
  if ((t_len != p_len) || !t_len)
  {
    DBG << "Empty buffer " << debug(*em);
    return ret;
  }

  CustomTimer timer(true);

  EventModel evt_model = geometry_.model(tb);

  uint64_t time_high = em->pulse_time();
  if (utime)
    time_high = utime << 32;


  ret = Spill::make_new(chan, StatusType::running);
  ret->stats[chan].set_value("pulse_time", time_high);
  ret->stats[chan].set_value("buf_id", buf_id);
  ret->stats[chan].set_model(evt_model);

  for (auto i=0; i < t_len; ++i)
  {
    uint64_t time = em->time_of_flight()->Get(i);
    time |= time_high;

    const auto& id = em->detector_id()->Get(i);
    if (id) //must be non0?
    {
      Event e(chan, evt_model);
      e.set_native_time(time);
      geometry_.interpret_id(e, id);
      ret->events.push_back(e);
    }

    if (i==1)
      stats.time_start = time;
    stats.time_start = std::min(stats.time_start, time);
    stats.time_end = std::max(stats.time_end, time);
  }

  stats.time_spent += timer.s();

  return ret;
}

std::string ev42_events::debug(const EventMessage& em)
{
  std::stringstream ss;

  ss << em.source_name()->str() << " #" << em.message_id()
     << " time=" << em.pulse_time()
     << " tof_size=" << em.time_of_flight()->Length()
     << " det_size=" << em.detector_id()->Length();

  return ss.str();
}
