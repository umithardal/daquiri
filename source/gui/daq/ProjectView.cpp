#include <gui/daq/ProjectView.h>

#include <gui/daq/ConsumerScalar.h>
#include <gui/daq/Consumer1D.h>
#include <gui/daq/Consumer2D.h>

#include <gui/daq/ConsumerMulti1D.h>

#include <gui/daq/ConsumerDialog.h>
#include <gui/widgets/QColorExtensions.h>
#include <gui/widgets/qt_util.h>
#include <core/util/Timer.h>
#include <QPlot/QHist.h>

using namespace DAQuiri;

ProjectView::ProjectView(QWidget* parent) :
    QWidget(parent), ui(new Ui::ProjectView)
{
  ui->setupUi(this);

  selector_ = new SelectorWidget();
  selector_->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::MinimumExpanding);
  selector_->setMaximumWidth(280);
  ui->scrollArea->setWidget(selector_);
  //ui->scrollArea->viewport()->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::MinimumExpanding);
  //ui->scrollArea->viewport()->setMinimumWidth(283);

  connect(selector_, SIGNAL(itemSelected(SelectorItem)), this, SLOT(selectorItemSelected(SelectorItem)));
  connect(selector_, SIGNAL(itemToggled(SelectorItem)), this, SLOT(selectorItemToggled(SelectorItem)));
  connect(selector_, SIGNAL(itemDoubleclicked(SelectorItem)), this, SLOT(selectorItemDoubleclicked(SelectorItem)));

  /// \brief communicate rate to ProjectForm
  connect(this, SIGNAL(updateRate(int)), parent, SLOT(updateRate(int)));

  colors_menu_.addAction(QIcon(":/icons/show16.png"), "Show all",
                         this, SLOT(showAll()));
  colors_menu_.addAction(QIcon(":/icons/hide16.png"), "Hide all",
                         this, SLOT(hideAll()));
  colors_menu_.addAction(QIcon(":/icons/oxy/16/roll.png"), "Randomize colors",
                         this, SLOT(randAll()));
  ui->toolColors->setMenu(&colors_menu_);

  delete_menu_.addAction(QIcon(":/icons/oxy/16/editdelete.png"), "Delete selected",
                         this, SLOT(deleteSelected()));
  delete_menu_.addAction(QIcon(":/icons/show16.png"), "Delete shown",
                         this, SLOT(deleteShown()));
  delete_menu_.addAction(QIcon(":/icons/hide16.png"), "Delete hidden",
                         this, SLOT(deleteHidden()));
  ui->toolDelete->setMenu(&delete_menu_);

  /// \brief drop down tiling menu
  tile_menu_.addAction(QIcon(), "Free-for-all",
                       this, SLOT(tile_free()));
  tile_menu_.addAction(QIcon(":/icons/blank16.png"), "Tile grid",
                       this, SLOT(tile_grid()));
  tile_menu_.addAction(QIcon(":/icons/oxy/16/view_split_left_right.png"), "Tile horizontal",
                       this, SLOT(tile_horizontal()));
  tile_menu_.addAction(QIcon(":/icons/oxy/16/view_split_top_bottom.png"), "Tile vertical",
                       this, SLOT(tile_vertical()));
  enforce_tile_policy();
  ui->toolTile->setMenu(&tile_menu_);

  loadSettings();
}

ProjectView::~ProjectView()
{
  saveSettings();
  delete ui;
}

void ProjectView::set_manifest(DAQuiri::StreamManifest m)
{
  stream_manifest_ = m;
}

//void ProjectView::closeEvent(QCloseEvent *event)
//{
//  saveSettings();
//  event->accept();
//}

void ProjectView::set_project(ProjectPtr project)
{
  consumers_.clear();
  groups_.clear();
  ui->area->closeAllSubWindows();
  project_ = project;
  updateUI();
  enforce_all();
  update();
}

void ProjectView::selectorItemToggled(SelectorItem item)
{
  Q_UNUSED(item)
  enforce_all();
  update_plots();
}

void ProjectView::enforce_all()
{
  auto initial_consumers = consumers_.size();
  auto items = selector_->items();
  std::set<size_t> visible_groups;
  for (auto it = items.rbegin(); it != items.rend(); it++)
  {
    auto item = *it;

    int64_t id = item.data.toLongLong();
    ConsumerPtr consumer = project_->get_consumer(id);
    if (!consumer)
      return;

    if (consumer->metadata().get_attribute("visible").get_bool() != item.visible)
      consumer->set_attribute(Setting::boolean("visible", item.visible));

    size_t group = consumer->metadata().get_attribute("window_group").get_int();
    if (item.visible && (group != 0u))
    {
      visible_groups.insert(group);
      continue;
    }

    if (consumers_.count(id) && !item.visible)
    {
      consumers_[id]->parentWidget()->close();
      consumers_.remove(id);
    }
    else if (!consumers_.count(id) && item.visible)
    {
      AbstractConsumerWidget* consumer_widget;
      if (consumer->dimensions() == 0)
        consumer_widget = new ConsumerScalar();
      else if (consumer->dimensions() == 1)
        consumer_widget = new Consumer1D();
      else if (consumer->dimensions() == 2)
        consumer_widget = new Consumer2D();
      else
        return;

      consumer_widget->setAttribute(Qt::WA_DeleteOnClose);
      connect(consumer_widget, SIGNAL(destroyed(QObject * )),
              this, SLOT(consumerWidgetDestroyed(QObject * )));
      auto w = ui->area->addSubWindow(consumer_widget);
      w->setContentsMargins(0, 0, 0, 0);
      consumers_[id] = consumer_widget;
      consumer_widget->show();
      consumer_widget->setConsumer(consumer);
    }
  }

  bool groups_changed = false;
  for (const auto& g : groups_)
  {
    auto id = g->group();
    if (!visible_groups.count(id))
    {
      groups_[id]->parentWidget()->close();
      groups_.remove(id);
      groups_changed = true;
    }
  }

  for (auto g : visible_groups)
  {
    if (!groups_.contains(g))
    {
      auto* consumer_widget = new ConsumerMulti1D();
      consumer_widget->setAttribute(Qt::WA_DeleteOnClose);
      connect(consumer_widget, SIGNAL(destroyed(QObject * )),
              this, SLOT(groupWidgetDestroyed(QObject * )));
      auto w = ui->area->addSubWindow(consumer_widget);
      w->setContentsMargins(0, 0, 0, 0);
      groups_[g] = consumer_widget;
      consumer_widget->show();
      consumer_widget->set_project(project_, g);
      groups_changed = true;
    }
  }

  if ((consumers_.size() != initial_consumers) || groups_changed)
    enforce_tile_policy();
}

void ProjectView::selectorItemDoubleclicked(SelectorItem /*item*/)
{
  on_pushFullInfo_clicked();
}

void ProjectView::selectorItemSelected(SelectorItem /*item*/)
{
  SelectorItem itm = selector_->selected();

  ConsumerPtr consumer = project_->get_consumer(itm.data.toLongLong());

  if (!consumer)
  {
    ui->labelSpectrumInfo->setText(
        "<html><head/><body><p>Left-click: see statistics here<br/>Right click: toggle visibility<br/>Double click: details / analysis</p></body></html>");
    ui->pushFullInfo->setEnabled(false);
    return;
  }

  if (consumers_.count(itm.data.toLongLong()))
    consumers_[itm.data.toLongLong()]->raise();

  ConsumerMetadata md = consumer->metadata();

  double real = md.get_attribute("real_time").duration().count() * 0.001;
  double live = md.get_attribute("live_time").duration().count() * 0.001;
  double total_count = md.get_attribute("total_count").get_number();
  double rate_total = 0;
  if (live > 0)
    rate_total = total_count / live;
  double dead = 100;
  if (real > 0)
    dead = (real - live) * 100.0 / real;
  double rate_inst = md.get_attribute("recent_native_time_rate").get_number();

  Detector det = Detector();
  if (!md.detectors.empty())
    det = md.detectors[0];

  uint16_t bits = md.get_attribute("resolution").selection();

  QString detstr = "Detector: " + QS(det.id());

  QString infoText =
      "<nobr>" + itm.text + "(" + QS(consumer->type())
          + ", " + QString::number(bits) + "bits)</nobr><br/>"
                                           "<nobr>" + detstr + "</nobr><br/>"
                                                               "<nobr>Count: " + QString::number(total_count)
          + "</nobr><br/>"
            "<nobr>Rate (inst/total): " + QString::number(rate_inst) + "cps / " + QString::number(rate_total)
          + "cps</nobr><br/>"
            "<nobr>Live / real:  " + QString::number(live) + "s / " + QString::number(real) + "s</nobr><br/>"
                                                                                              "<nobr>Dead:  "
          + QString::number(dead) + "%</nobr><br/>";

  ui->labelSpectrumInfo->setText(infoText);
  ui->pushFullInfo->setEnabled(true);
  emit updateRate(rate_total);
}

void ProjectView::updateUI()
{
  SelectorItem chosen = selector_->selected();
  QVector<SelectorItem> items;

  size_t i = 0;
  for (auto& q : project_->get_consumers())
  {
    if (!q)
      continue;

    ConsumerMetadata md = q->metadata();

    Setting appearance = md.get_attribute("appearance");
    QColor color;
    if (appearance != Setting())
      color = QColor(QS(appearance.get_text()));
    else
      color = Qt::black;

    size_t group = md.get_attribute("window_group").get_int();

    SelectorItem consumer_item;
    if (group != 0u)
      consumer_item.text = "[" + QString::number(group) + "] ";
    else
      consumer_item.text = "";
    consumer_item.text += QS(md.get_attribute("name").get_text());
    consumer_item.data = QVariant::fromValue(i++);
    consumer_item.color = color;
    consumer_item.visible = md.get_attribute("visible").triggered();

    items.push_back(consumer_item);
  }

  selector_->setItems(items);
  selector_->setSelected(chosen.text);

  ui->scrollArea->updateGeometry();

  ui->toolColors->setEnabled(selector_->items().size());
  ui->toolDelete->setEnabled(selector_->items().size());
}

void ProjectView::update_plots()
{
//  Timer t(true);
  for (auto& consumer_widget : consumers_)
    consumer_widget->update_data();

  for (auto& consumer_widget : groups_)
    consumer_widget->update_data();

  for (auto& consumer_widget : consumers_)
    consumer_widget->refresh();

  for (auto& consumer_widget : groups_)
    consumer_widget->refresh();

  selectorItemSelected(SelectorItem());
//  DBG( "<ProjectView> plotting took " << t.s();
}

void ProjectView::on_pushFullInfo_clicked()
{
  ConsumerPtr consumer = project_->get_consumer(selector_->selected().data.toLongLong());
  if (!consumer)
    return;

  ConsumerDialog* newSpecDia =
      new ConsumerDialog(consumer, std::vector<Detector>(),
                         detectors_, stream_manifest_, false, this);

  //DBG( "Consumer:\n" << consumer->debug() << "\n";

  if (newSpecDia->exec() == QDialog::Accepted)
  {
    ConsumerMetadata md = newSpecDia->product()->metadata();
    consumer->set_detectors(md.detectors);
    consumer->set_attributes(md.attributes());
    updateUI();
    enforce_all();
    update();
  }
}

void ProjectView::showAll()
{
  selector_->show_all();
  for (auto& q : project_->get_consumers())
    if (q)
      q->set_attribute(Setting::boolean("visible", true));
  updateUI();
  enforce_all();
  update();
}

void ProjectView::hideAll()
{
  selector_->hide_all();
  for (auto& q : project_->get_consumers())
    if (q)
      q->set_attribute(Setting::boolean("visible", false));
  updateUI();
  enforce_all();
  update();
}

void ProjectView::randAll()
{
  for (auto& q : project_->get_consumers())
    if (q)
    {
      Setting appearance = q->metadata().get_attribute("appearance");
      if (appearance.metadata().has_flag("color"))
      {
        appearance.set_text(generateColor().name(QColor::HexArgb).toStdString());
        q->set_attribute(appearance);
      }
    }
  updateUI();
  update();
}

void ProjectView::deleteSelected()
{
  project_->delete_consumer(selector_->selected().data.toLongLong());
  updateUI();
  enforce_all();
  update();
}

void ProjectView::deleteShown()
{
  for (auto& q : selector_->items())
    if (q.visible)
      project_->delete_consumer(q.data.toLongLong());
  updateUI();
  enforce_all();
  update();
}

void ProjectView::deleteHidden()
{
  for (auto& q : selector_->items())
    if (!q.visible)
      project_->delete_consumer(q.data.toLongLong());
  updateUI();
  enforce_all();
  update();
}

void ProjectView::tile_free()
{
  tile_policy_ = "free";
  enforce_tile_policy();
}

void ProjectView::tile_grid()
{
  tile_policy_ = "grid";
  enforce_tile_policy();
}

void ProjectView::tile_horizontal()
{
  tile_policy_ = "horizontal";
  enforce_tile_policy();
}

void ProjectView::tile_vertical()
{
  tile_policy_ = "vertical";
  enforce_tile_policy();
}

void ProjectView::enforce_tile_policy()
{
//  ui->area->adjustSize();

  if (tile_policy_ == "grid")
    tile_grid(ui->area);
  else if (tile_policy_ == "vertical")
    tile_vertical(ui->area);
  else if (tile_policy_ == "horizontal")
    tile_horizontal(ui->area);

  for (auto& q : tile_menu_.actions())
  {
    q->setCheckable(true);
    q->setChecked(
        ((q->text() == "Free-for-all") && (tile_policy_ == "free")) ||
            ((q->text() == "Tile grid") && (tile_policy_ == "grid")) ||
            ((q->text() == "Tile horizontal") && (tile_policy_ == "horizontal")) ||
            ((q->text() == "Tile vertical") && (tile_policy_ == "vertical"))
    );
  }
}

void ProjectView::tile_grid(QMdiArea* area)
{
  area->tileSubWindows();
}

void ProjectView::tile_horizontal(QMdiArea* area)
{
  if (area->subWindowList().isEmpty())
    return;

  QPoint position(0, 0);

      foreach (QMdiSubWindow* window, area->subWindowList())
    {
      QRect rect(0, 0, area->width() / area->subWindowList().count(),
                 area->height());
      window->setGeometry(rect);
      window->move(position);
      position.setX(position.x() + window->width());
    }
}

void ProjectView::tile_vertical(QMdiArea* area)
{
  if (area->subWindowList().isEmpty())
    return;

  QPoint position(0, 0);

      foreach (QMdiSubWindow* window, area->subWindowList())
    {
      QRect rect(0, 0, area->width(),
                 area->height() / area->subWindowList().count());
      window->setGeometry(rect);
      window->move(position);
      position.setY(position.y() + window->height());
    }
}

void ProjectView::consumerWidgetDestroyed(QObject* o)
{
  for (auto it = consumers_.begin(); it != consumers_.end();)
    if (it.value() == o)
    {
      auto c = project_->get_consumer(it.key());
      if (c)
        c->set_attribute(Setting::boolean("visible", false));
      consumers_.erase(it++);
    }
    else
      ++it;
  updateUI();
}

void ProjectView::groupWidgetDestroyed(QObject* o)
{

  for (auto it = groups_.begin(); it != groups_.end();)
    if (it.value() == o)
    {
      auto id = it.key();
      for (auto& c : project_->get_consumers())
      {
        if (static_cast<size_t>(c->metadata().get_attribute("window_group").get_int()) == id)
          c->set_attribute(Setting::boolean("visible", false));
      }
      groups_.erase(it++);
    }
    else
      ++it;

  updateUI();
}

void ProjectView::on_pushHideControls_clicked()
{
  if (ui->widgetControls->isVisible())
  {
    ui->widgetControls->setVisible(false);
    ui->pushHideControls->setIcon(QIcon(":/icons/oxy/16/forward.png"));
  }
  else
  {
    ui->widgetControls->setVisible(true);
    ui->pushHideControls->setIcon(QIcon(":/icons/oxy/16/back.png"));
  }

  QTimer::singleShot(50, this, SLOT(enforce_tile_policy()));
}

void ProjectView::loadSettings()
{
  QSettings settings_;
  settings_.beginGroup("ProjectView");

  tile_policy_ = settings_.value("tile_policy", "grid").toString();
  if (settings_.value("show_controls", true).toBool())
  {
    ui->widgetControls->setVisible(true);
    ui->pushHideControls->setIcon(QIcon(":/icons/oxy/16/back.png"));
  }
  else
  {
    ui->widgetControls->setVisible(false);
    ui->pushHideControls->setIcon(QIcon(":/icons/oxy/16/forward.png"));
  }
  enforce_tile_policy();
}

void ProjectView::saveSettings()
{
  QSettings settings_;
  settings_.beginGroup("ProjectView");

  settings_.setValue("tile_policy", tile_policy_);
  settings_.setValue("show_controls", ui->widgetControls->isVisible());
}
