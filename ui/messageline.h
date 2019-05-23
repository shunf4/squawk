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
#include <QHBoxLayout>
#include <QLabel>
#include <QResizeEvent>
#include "../global.h"

class MessageLine : public QWidget
{
    Q_OBJECT
public:
    enum Position {
        beggining,
        middle,
        end,
        invalid
    };
    MessageLine(QWidget* parent = 0);
    ~MessageLine();
    
    Position message(const Shared::Message& msg);
    void setMyName(const QString& name);
    void setPalName(const QString& jid, const QString& name);
    QString firstMessageId() const;
    void showBusyIndicator();
    void hideBusyIndicator();
    
signals:
    void resize(int amount);
    
protected:
    void resizeEvent(QResizeEvent * event) override;
    
private:
    struct Comparator {
        bool operator()(const Shared::Message& a, const Shared::Message& b) const {
            return a.getTime() < b.getTime();
        }
        bool operator()(const Shared::Message* a, const Shared::Message* b) const {
            return a->getTime() < b->getTime();
        }
    };
    typedef std::map<QDateTime, Shared::Message*> Order;
    typedef std::map<QString, Shared::Message*> Index;
    Index messageIndex;
    Order messageOrder;
    QVBoxLayout* layout;
    
    QString myName;
    std::map<QString, QString> palNames;
    std::deque<QHBoxLayout*> views;
};

#endif // MESSAGELINE_H
