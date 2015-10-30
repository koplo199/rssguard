// This file is part of RSS Guard.
//
// Copyright (C) 2011-2015 by Martin Rotter <rotter.martinos@gmail.com>
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

#include "core/feedsselection.h"

#include "core/rootitem.h"
#include "services/standard/standardcategory.h"
#include "services/standard/standardfeed.h"
#include "definitions/definitions.h"


FeedsSelection::FeedsSelection(RootItem *root_of_selection) : m_selectedItem(root_of_selection) {
}

FeedsSelection::FeedsSelection(const FeedsSelection &other) {
  m_selectedItem = other.selectedItem();
}

FeedsSelection::~FeedsSelection() {
}

FeedsSelection::SelectionMode FeedsSelection::mode() {
  if (m_selectedItem == NULL) {
    return FeedsSelection::NoMode;
  }

  switch (m_selectedItem->kind()) {
    case RootItem::Bin:
      return FeedsSelection::MessagesFromRecycleBin;

    case RootItem::Cattegory:
    case RootItem::Feeed:
      return FeedsSelection::MessagesFromFeeds;

    default:
      return FeedsSelection::NoMode;
  }
}

RootItem *FeedsSelection::selectedItem() const {
  return m_selectedItem;
}

QString FeedsSelection::generateListOfIds() {
  if (m_selectedItem != NULL &&
      (m_selectedItem->kind() == RootItem::Feeed || m_selectedItem->kind() == RootItem::Cattegory)) {
    QList<RootItem*> children = m_selectedItem->getRecursiveChildren();
    QStringList stringy_ids;

    foreach (RootItem *child, children) {
      if (child->kind() == RootItem::Feeed) {
        stringy_ids.append(QString::number(child->id()));
      }
    }

    return stringy_ids.join(QSL(", "));
  }
  else {
    return QString();
  }
}
