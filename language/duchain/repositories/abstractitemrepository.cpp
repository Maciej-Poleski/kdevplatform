/*
   Copyright 2008 David Nolden <david.nolden.kdevelop@art-master.de>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "abstractitemrepository.h"

namespace KDevelop {

uint staticItemRepositoryVersion()
{
  //Increase this to reset incompatible item-repositories
  return 77;
}

AbstractItemRepository::~AbstractItemRepository()
{
}

AbstractRepositoryManager::AbstractRepositoryManager() : m_repository(0)
{
}

AbstractRepositoryManager::~AbstractRepositoryManager()
{
}

void AbstractRepositoryManager::deleteRepository()
{
  delete m_repository;
  m_repository = 0;

  repositoryDeleted();
}

void AbstractRepositoryManager::repositoryDeleted()
{
}

}
