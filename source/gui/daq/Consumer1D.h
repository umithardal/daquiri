#pragma once

#include "AbstractConsumerWidget.h"
#include "QPlot1D.h"

class Consumer1D : public AbstractConsumerWidget
{
    Q_OBJECT

  public:
    Consumer1D(QWidget *parent = 0);

    void update() override;
    void refresh() override;

  private slots:
    void mouseWheel (QWheelEvent *event);
    void zoomedOut();
    void scaleChanged(QString);

  private:
    QPlot::Multi1D* plot_ {nullptr};
    bool initial_scale_ {false};
    bool user_zoomed_ {false};
};