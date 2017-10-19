// This file is part of RSS Guard.

//
// Copyright (C) 2011-2017 by Martin Rotter <rotter.martinos@gmail.com>
// Copyright (C) 2010-2014 by David Rosca <nowrep@gmail.com>
//
// RSS Guard is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// RSS Guard is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with RSS Guard. If not, see <http://www.gnu.org/licenses/>.

#ifndef GMAILSERVICEROOT_H
#define GMAILSERVICEROOT_H

#include "services/abstract/cacheforserviceroot.h"
#include "services/abstract/serviceroot.h"

class GmailNetworkFactory;

class GmailServiceRoot : public ServiceRoot, public CacheForServiceRoot {
  Q_OBJECT

  public:
    explicit GmailServiceRoot(GmailNetworkFactory* network, RootItem* parent = nullptr);
    virtual ~GmailServiceRoot();

    void saveAccountDataToDatabase();

    void setNetwork(GmailNetworkFactory* network);
    GmailNetworkFactory* network() const;

    bool canBeEdited() const;
    bool editViaGui();
    bool canBeDeleted() const;
    bool deleteViaGui();
    bool supportsFeedAdding() const;
    bool supportsCategoryAdding() const;
    void start(bool freshly_activated);
    void stop();
    QString code() const;

    QString additionalTooltip() const;

    void saveAllCachedData(bool async = true);

  public slots:
    void updateTitle();

  private:
    void loadFromDatabase();

  private:
    QList<QAction*> m_serviceMenu;
    GmailNetworkFactory* m_network;
};

inline void GmailServiceRoot::setNetwork(GmailNetworkFactory* network) {
  m_network = network;
}

inline GmailNetworkFactory* GmailServiceRoot::network() const {
  return m_network;
}

#endif // GMAILSERVICEROOT_H
