/* Copyright (C) 2016-2020 European Spallation Source, ERIC. See LICENSE file */
//===----------------------------------------------------------------------===//
///
/// \file Engine.h
///
/// \brief Class responsible for starting and stopping all producers as well
/// as starting the builder (consumer?)
///
//===----------------------------------------------------------------------===//
#pragma once

#include <core/thread_wrappers.h>
#include <atomic>

#include <core/Project.h>
#include <core/Producer.h>

namespace DAQuiri {

using Interruptor = std::atomic<bool>;

class Engine
{
  public:
    static Engine& singleton()
    {
      static Engine singleton_instance;
      return singleton_instance;
    }

    static Setting default_settings();

    void initialize(const json& profile);
    void boot();
    void die();

    ProducerStatus status() const;
    OscilData oscilloscope();
    ListData acquire_list(Interruptor& inturruptor, uint64_t timeout);
    void acquire(ProjectPtr project, Interruptor& interruptor,
                 uint64_t timeout);

    /////SETTINGS/////
    Setting settings() const;
    void settings(const Setting&);
    StreamManifest stream_manifest() const;
    void set_setting(Setting address, Match flags, bool greedy = false);
    void get_all_settings();

  private:
    mutable mutex_st mutex_;

    ProducerStatus aggregate_status_{ProducerStatus(0)};

    std::map<std::string, ProducerPtr> producers_;

    Setting settings_;
    // {SettingMeta("Engine", SettingType::stem)};
    int drop_packets_{0};
    size_t max_packets_{100};

    std::map<std::string, SettingMeta> setting_definitions_;

    void _die();
    void _set_settings(const Setting&);
    void _get_all_settings();
    void _read_settings_bulk();
    void _write_settings_bulk();

    void save_det_settings(Setting&, const Setting&, Match flags) const;
    void load_det_settings(Setting, Setting&, Match flags);

    bool daq_start(SpillMultiqueue * out_queue);
    bool daq_stop();
    bool daq_running() const;

    //threads
    void builder_naive(SpillMultiqueue * data_queue,
                       ProjectPtr project);

    //singleton assurance
    Engine();
    Engine(Engine const&);
    void operator=(Engine const&);
    ~Engine();
};

}
