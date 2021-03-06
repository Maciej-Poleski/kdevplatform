/***************************************************************************
 *   Copyright 2007 Robert Gruber <rgruber@users.sourceforge.net>          *
 *   Copyright 2007 Andreas Pakulat <apaku@gmx.de>                         *
 *                                                                         *
 *   Adapted for Git                                                       *
 *   Copyright 2008 Evgeniy Ivanov <powerfox@kde.ru>                       *
 *                                                                         *
 *   Pimpl-ed and exported                                                 *
 *   Copyright 2014 Maciej Poleski                                         *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU General Public License as        *
 *   published by the Free Software Foundation; either version 2 of        *
 *   the License or (at your option) version 3 or any later version        *
 *   accepted by the membership of KDE e.V. (or its successor approved     *
 *   by the membership of KDE e.V.), which shall act as a proxy            *
 *   defined in Section 14 of version 3 of the license.                    *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>. *
 ***************************************************************************/

#ifndef KDEVPLATFORM_DVCSIMPORTMETADATAWIDGET_H
#define KDEVPLATFORM_DVCSIMPORTMETADATAWIDGET_H

#include <vcs/widgets/vcsimportmetadatawidget.h>

namespace Ui {
class DvcsImportMetadataWidget;
}

class DvcsImportMetadataWidgetPrivate;

/**
 * Asks the user for all options needed to import an existing directory into
 * a Git repository
 * @author Robert Gruber <rgruber@users.sourceforge.net>
 */
class KDEVPLATFORMVCS_EXPORT DvcsImportMetadataWidget : public KDevelop::VcsImportMetadataWidget
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(DvcsImportMetadataWidget)
    
public:
    DvcsImportMetadataWidget(QWidget* parent=0);
    virtual ~DvcsImportMetadataWidget();

    virtual KUrl source() const;
    virtual KDevelop::VcsLocation destination() const;
    virtual QString message() const; //Is not used, it returns QString("")
    virtual void setSourceLocation( const KDevelop::VcsLocation& );
    virtual void setSourceLocationEditable( bool );
    virtual bool hasValidData() const;

private:
    DvcsImportMetadataWidgetPrivate *const d_ptr;
};

#endif
