/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2019  Yury Gubich <blue@macaw.me>
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

#ifndef MESSAGELINE_H
#define MESSAGELINE_H

#include <QWidget>
#include <QVBoxLayout>
#include <QLabel>
#include "../global.h"
#include "../order.h"


class MessageLine : public QWidget
{
    Q_OBJECT
public:
    MessageLine(QWidget* parent = 0);
    ~MessageLine();
    
    void message(const Shared::Message& msg);
    void setMyName(const QString& name);
    void setPalName(const QString& jid, const QString& name);
    
private:
    typedef W::Order<Shared::Message*> Order;
    std::map<QString, Shared::Message*> messageIndex;
    Order messageOrder;
    QVBoxLayout* layout;
    
    QString myName;
    std::map<QString, QString> palNames;
};

#endif // MESSAGELINE_H
