/*
 * Squawk messenger. 
 * Copyright (C) 2019 Yury Gubich <blue@macaw.me>
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "progress.h"

#include "shared/icons.h"

Progress::Progress(quint16 p_size, QWidget* parent):
    QWidget(parent),
    pixmap(new QGraphicsPixmapItem(Shared::icon("view-refresh", true).pixmap(p_size))),
    scene(),
    label(&scene),
    progress(false),
    animation(),
    size(p_size)
{
    scene.addItem(pixmap);
    label.setMaximumSize(size, size);
    label.setMinimumSize(size, size);
    label.setSceneRect(0, 0, size, size);
    label.setFrameStyle(0);
    label.setContentsMargins(0, 0, 0, 0);
    label.setInteractive(false);
    label.setStyleSheet("background: transparent");
    pixmap->setTransformOriginPoint(size / 2, size / 2);
    pixmap->setTransformationMode(Qt::SmoothTransformation);
    pixmap->setOffset(0, 0);
    
    animation.setDuration(1000);
    animation.setStartValue(0.0f);
    animation.setEndValue(360.0f);
    animation.setLoopCount(-1);
    connect(&animation, &QVariantAnimation::valueChanged, this, &Progress::onValueChanged);
    
    QGridLayout* layout = new QGridLayout();
    setLayout(layout);
    layout->setMargin(0);
    layout->setVerticalSpacing(0);
    layout->setHorizontalSpacing(0);
    
    setContentsMargins(0, 0, 0, 0);
    
    layout->addWidget(&label, 0, 0, 1, 1);
    label.hide();
}

Progress::~Progress()
{
}

void Progress::onValueChanged(const QVariant& value)
{
    pixmap->setRotation(value.toReal());
}

void Progress::start()
{
    if (!progress) {
        label.show();
        animation.start();
        progress = true;
    }
}

void Progress::stop()
{
    if (progress) {
        label.hide();
        animation.stop();
        progress = false;
    }
}
