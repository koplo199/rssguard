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

#include "network-web/adblock/adblockicon.h"

#include "network-web/adblock/adblockrule.h"
#include "network-web/adblock/adblockmanager.h"
#include "network-web/adblock/adblocksubscription.h"
#include "miscellaneous/application.h"
#include "network-web/webpage.h"
#include "gui/webbrowser.h"
#include "gui/webviewer.h"
#include "gui/dialogs/formmain.h"

#include <QMenu>
#include <QTimer>


AdBlockIcon::AdBlockIcon(BrowserWindow* window, QWidget* parent)
  : ClickableLabel(parent), m_window(window), m_menuAction(0), m_flashTimer(0), m_timerTicks(0), m_enabled(false) {
  setCursor(Qt::PointingHandCursor);
  setToolTip(tr("AdBlock lets you block unwanted content on web pages"));
  setFixedSize(16, 16);

  connect(this, SIGNAL(clicked(QPoint)), this, SLOT(showMenu(QPoint)));
  connect(AdBlockManager::instance(), SIGNAL(enabledChanged(bool)), this, SLOT(setEnabled(bool)));
}

AdBlockIcon::~AdBlockIcon() {
  for (int i = 0; i < m_blockedPopups.count(); ++i) {
    delete m_blockedPopups.at(i).first;
  }
}

void AdBlockIcon::popupBlocked(const QString &ruleString, const QUrl &url) {
  int index = ruleString.lastIndexOf(QLatin1String(" ("));

  const QString subscriptionName = ruleString.left(index);
  const QString filter = ruleString.mid(index + 2, ruleString.size() - index - 3);
  AdBlockSubscription *subscription = AdBlockManager::instance()->subscriptionByName(subscriptionName);
  if (filter.isEmpty() || !subscription) {
    return;
  }

  QPair<AdBlockRule*, QUrl> pair;
  pair.first = new AdBlockRule(filter, subscription);
  pair.second = url;
  m_blockedPopups.append(pair);

  qApp->showGuiMessage(tr("Blocked popup window"), tr("AdBlock blocked unwanted popup window."), QSystemTrayIcon::Information);

  if (!m_flashTimer) {
    m_flashTimer = new QTimer(this);
  }

  if (m_flashTimer->isActive()) {
    stopAnimation();
  }

  m_flashTimer->setInterval(500);
  m_flashTimer->start();

  connect(m_flashTimer, &QTimer::timeout, this, &AdBlockIcon::animateIcon);
}

QAction *AdBlockIcon::menuAction() {
  if (!m_menuAction) {
    m_menuAction = new QAction(tr("AdBlock"), this);
    m_menuAction->setMenu(new QMenu);
    connect(m_menuAction->menu(), SIGNAL(aboutToShow()), this, SLOT(createMenu()));
  }

  m_menuAction->setIcon(m_enabled ? qApp->icons()->miscIcon(ADBLOCK_ICON_ACTIVE) : qApp->icons()->miscIcon(ADBLOCK_ICON_DISABLED));
  return m_menuAction;
}

void AdBlockIcon::createMenu(QMenu *menu) {
  if (!menu) {
    menu = qobject_cast<QMenu*>(sender());

    if (!menu) {
      return;
    }
  }

  menu->clear();

  AdBlockManager* manager = AdBlockManager::instance();
  AdBlockCustomList* customList = manager->customList();

  WebPage *page = qApp->mainForm()->tabWidget()->currentWidget()->webBrowser()->viewer()->page();
  const QUrl pageUrl = page->url();

  menu->addAction(tr("Show AdBlock &settings"), manager, SLOT(showDialog()));
  menu->addSeparator();

  if (!pageUrl.host().isEmpty() && m_enabled && manager->canRunOnScheme(pageUrl.scheme())) {
    const QString host = page->url().host().contains(QLatin1String("www.")) ? pageUrl.host().mid(4) : pageUrl.host();
    const QString hostFilter = QString("@@||%1^$document").arg(host);
    const QString pageFilter = QString("@@|%1|$document").arg(pageUrl.toString());

    QAction *act = menu->addAction(tr("Disable on %1").arg(host));
    act->setCheckable(true);
    act->setChecked(customList->containsFilter(hostFilter));
    act->setData(hostFilter);
    connect(act, SIGNAL(triggered()), this, SLOT(toggleCustomFilter()));

    act = menu->addAction(tr("Disable only on this page"));
    act->setCheckable(true);
    act->setChecked(customList->containsFilter(pageFilter));
    act->setData(pageFilter);
    connect(act, SIGNAL(triggered()), this, SLOT(toggleCustomFilter()));

    menu->addSeparator();
  }

  if (!m_blockedPopups.isEmpty()) {
    menu->addAction(tr("Blocked popup windows"))->setEnabled(false);

    for (int i = 0; i < m_blockedPopups.count(); i++) {
      const QPair<AdBlockRule*, QUrl> &pair = m_blockedPopups.at(i);

      QString address = pair.second.toString().right(55);
      QString actionText = tr("%1 with (%2)").arg(address, pair.first->filter()).replace(QL1C('&'), QL1S("&&"));

      QAction *action = menu->addAction(actionText, manager, SLOT(showRule()));
      action->setData(QVariant::fromValue((void*)pair.first));
    }
  }
}

void AdBlockIcon::showMenu(const QPoint &pos) {
  QMenu menu;
  createMenu(&menu);

  menu.exec(pos);
}

void AdBlockIcon::toggleCustomFilter() {
  QAction *action = qobject_cast<QAction*>(sender());

  if (!action) {
    return;
  }

  const QString filter = action->data().toString();
  AdBlockManager *manager = AdBlockManager::instance();
  AdBlockCustomList *customList = manager->customList();

  if (customList->containsFilter(filter)) {
    customList->removeFilter(filter);
  }
  else {
    AdBlockRule *rule = new AdBlockRule(filter, customList);
    customList->addRule(rule);
  }
}

void AdBlockIcon::animateIcon() {
  ++m_timerTicks;

  if (m_timerTicks > 10) {
    stopAnimation();
    return;
  }

  if (pixmap()->isNull()) {
    setPixmap(qApp->icons()->miscIcon(ADBLOCK_ICON_ACTIVE).pixmap(16));
  }
  else {
    setPixmap(QPixmap());
  }
}

void AdBlockIcon::stopAnimation() {
  m_timerTicks = 0;
  m_flashTimer->stop();
  disconnect(m_flashTimer, SIGNAL(timeout()), this, SLOT(animateIcon()));

  setEnabled(m_enabled);
}

void AdBlockIcon::setEnabled(bool enabled) {
  if (enabled) {
    setPixmap(qApp->icons()->miscIcon(ADBLOCK_ICON_ACTIVE).pixmap(16));
  }
  else {
    setPixmap(qApp->icons()->miscIcon(ADBLOCK_ICON_DISABLED).pixmap(16));
  }

  m_enabled = enabled;
}