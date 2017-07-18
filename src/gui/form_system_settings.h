#pragma once

#include <QWidget>
#include <QCloseEvent>
#include <QTableView>
#include <QTreeView>
#include <QMenu>
#include "detector.h"
#include "special_delegate.h"
#include "table_settings.h"
#include "tree_settings.h"

#include "thread_runner.h"

namespace Ui {
class FormSystemSettings;
}

class FormSystemSettings : public QWidget
{
  Q_OBJECT

public:
  explicit FormSystemSettings(ThreadRunner&,
                              Container<Detector>&,
                              QWidget *parent = 0);
  Setting get_tree() {return dev_settings_;}
  ~FormSystemSettings();

  void exit();

public slots:
  void refresh();
  void update(const Setting &tree, const std::vector<DAQuiri::Detector> &channelsupdate, DAQuiri::ProducerStatus);

signals:
  void toggleIO(bool);
  void statusText(QString);

protected:
  void closeEvent(QCloseEvent*);

public slots:
  void updateDetDB();
  
private slots:
  void begin_editing();
  void stop_editing(QWidget*,QAbstractItemDelegate::EndEditHint);
  void on_pushSettingsRefresh_clicked();

  void toggle_push(bool enable, DAQuiri::ProducerStatus status);
  void post_boot();

  void push_settings();
  void push_from_table(DAQuiri::Setting setting);
  void chose_detector(int chan, std::string name);

  void ask_binary_tree(DAQuiri::Setting, QModelIndex index);
  void ask_execute_tree(DAQuiri::Setting, QModelIndex index);
  void ask_binary_table(DAQuiri::Setting, QModelIndex index);
  void ask_execute_table(DAQuiri::Setting, QModelIndex index);

  void on_checkShowRO_clicked();
  void on_bootButton_clicked();

  void apply_detector_presets();
  void open_detector_DB();

  void on_spinRefreshFrequency_valueChanged(int arg1);

  void on_pushChangeProfile_clicked();

  void choose_profiles();
  void profile_chosen();


  void refresh_oscil();

  void on_pushExpandAll_clicked();

private:
  Ui::FormSystemSettings *ui;

  DAQuiri::ProducerStatus current_status_;
  Container<Detector>     &detectors_;
  ThreadRunner            &runner_thread_;
  bool editing_;

  std::vector<DAQuiri::Detector>   channels_;
  QTableView*             viewTableSettings;
  TableChanSettings       table_settings_model_;
  DAQuiriSpecialDelegate  table_settings_delegate_;

  Setting                 dev_settings_;
  QTreeView*              viewTreeSettings;
  TreeSettings            tree_settings_model_;
  DAQuiriSpecialDelegate  tree_delegate_;

  QMenu detectorOptions;

  void loadSettings();
  void saveSettings();
  void chan_settings_to_det_DB();

  bool exiting;
};