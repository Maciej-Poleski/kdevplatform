/* This file is part of KDevelop
 *
 * Copyright 2013 Sven Brauch <svenbrauch@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 */

#include "icontentawareversioncontrol.h"

#include <QDebug>

namespace KDevelop {

struct CheckInRepositoryJobPrivate
{
    CheckInRepositoryJobPrivate(KTextEditor::Document* document)
        : document(document) { };
    friend class CheckInRepositoryJob;
    KTextEditor::Document* document;
};

CheckInRepositoryJob::CheckInRepositoryJob(KTextEditor::Document* document)
    : KJob()
    , d(new CheckInRepositoryJobPrivate(document))
{
    connect(this, SIGNAL(finished(bool)), SLOT(deleteLater()));
    setCapabilities(Killable);
};

KTextEditor::Document* CheckInRepositoryJob::document() const
{
    return d->document;
}

CheckInRepositoryJob::~CheckInRepositoryJob()
{
    delete d;
}

void CheckInRepositoryJob::abort()
{
    kill();
}

} // namespace KDevelop

#include "icontentawareversioncontrol.moc"
