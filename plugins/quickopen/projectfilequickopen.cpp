/* This file is part of the KDE libraries
   Copyright (C) 2007 David Nolden <david.nolden.kdevelop@art-master.de>

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

#include "projectfilequickopen.h"

#include <QIcon>
#include <QTextBrowser>
#include <QApplication>

#include <KLocale>

#include <interfaces/iprojectcontroller.h>
#include <interfaces/idocumentcontroller.h>
#include <interfaces/iproject.h>
#include <interfaces/icore.h>

#include <language/duchain/topducontext.h>
#include <language/duchain/duchain.h>
#include <language/duchain/duchainlock.h>
#include <language/duchain/indexedstring.h>
#include <language/duchain/parsingenvironment.h>

#include <project/projectmodel.h>

#include "../openwith/iopenwith.h"

using namespace KDevelop;

namespace {

QSet<IndexedString> openFiles()
{
    QSet<IndexedString> openFiles;
    const QList<IDocument*>& docs = ICore::self()->documentController()->openDocuments();
    openFiles.reserve(docs.size());
    foreach( IDocument* doc, docs ) {
        openFiles << IndexedString(doc->url().pathOrUrl());
    }
    return openFiles;
}

QString iconNameForUrl(const IndexedString& url)
{
    if (url.isEmpty()) {
        return QString("tab-duplicate");
    }
    ProjectBaseItem* item = ICore::self()->projectController()->projectModel()->itemForPath(url);
    if (item) {
        return item->iconName();
    }
    return QString("unknown");
}

}

ProjectFileData::ProjectFileData( const ProjectFile& file )
: m_file(file)
{
}

QString ProjectFileData::text() const
{
    return m_file.projectPath.relativePath(m_file.path);
}

QString ProjectFileData::htmlDescription() const
{
  return "<small><small>" + i18nc("%1: project name", "Project %1", project())  + "</small></small>";
}

bool ProjectFileData::execute( QString& filterText )
{
    const KUrl url = m_file.path.toUrl();
    IOpenWith::openFiles(KUrl::List() << url);
    QString path;
    uint lineNumber;
    if (extractLineNumber(filterText, path, lineNumber)) {
        IDocument* doc = ICore::self()->documentController()->documentForUrl(url);
        if (doc) {
            doc->setCursorPosition(KTextEditor::Cursor(lineNumber - 1, 0));
        }
    }
    return true;
}

bool ProjectFileData::isExpandable() const
{
    return true;
}

QList<QVariant> ProjectFileData::highlighting() const
{
    QTextCharFormat boldFormat;
    boldFormat.setFontWeight(QFont::Bold);
    QTextCharFormat normalFormat;

    QString txt = text();

    QList<QVariant> ret;

    int fileNameLength = m_file.path.lastPathSegment().length();

    ret << 0;
    ret << txt.length() - fileNameLength;
    ret << QVariant(normalFormat);
    ret << txt.length() - fileNameLength;
    ret << fileNameLength;
    ret << QVariant(boldFormat);

    return ret;
}

QWidget* ProjectFileData::expandingWidget() const
{
    const KUrl url = m_file.path.toUrl();
    DUChainReadLocker lock;

    ///Find a du-chain for the document
    QList<TopDUContext*> contexts = DUChain::self()->chainsForDocument(url);

    ///Pick a non-proxy context
    TopDUContext* chosen = 0;
    foreach( TopDUContext* ctx, contexts ) {
        if( !(ctx->parsingEnvironmentFile() && ctx->parsingEnvironmentFile()->isProxyContext()) ) {
            chosen = ctx;
        }
    }

    if( chosen ) {
        return chosen->createNavigationWidget(0, 0,
            "<small><small>"
            + i18nc("%1: project name", "Project %1", project())
            + "</small></small>");
    } else {
        QTextBrowser* ret = new QTextBrowser();
        ret->resize(400, 100);
        ret->setText(
                "<small><small>"
                + i18nc("%1: project name", "Project %1", project())
                + "<br>" + i18n("Not parsed yet") + "</small></small>");
        return ret;
    }

    return 0;
}

QIcon ProjectFileData::icon() const
{
    const QString& iconName = iconNameForUrl(m_file.indexedPath);

    /**
     * FIXME: Move this cache into a more central place and reuse it elsewhere.
     *        The project model e.g. could reuse this as well.
     *
     * Note: We cache here since otherwise displaying and esp. scrolling
     *       in a large list of quickopen items becomes very slow.
     */
    static QHash<QString, QPixmap> iconCache;
    QHash< QString, QPixmap >::const_iterator it = iconCache.constFind(iconName);
    if (it != iconCache.constEnd()) {
        return it.value();
    }

    const QPixmap& pixmap = KIconLoader::global()->loadIcon(iconName, KIconLoader::Small);
    iconCache.insert(iconName, pixmap);
    return pixmap;
}

QString ProjectFileData::project() const
{
    const IProject* project = ICore::self()->projectController()->findProjectForUrl(m_file.path.toUrl());
    if (project) {
        return project->name();
    } else {
        return i18n("none");
    }
}

BaseFileDataProvider::BaseFileDataProvider()
{
}

void BaseFileDataProvider::setFilterText( const QString& text )
{
    QString path(text);
    uint lineNumber;
    extractLineNumber(text, path, lineNumber);
    if ( path.startsWith(QLatin1String("./")) || path.startsWith(QLatin1String("../")) ) {
        // assume we want to filter relative to active document's url
        IDocument* doc = ICore::self()->documentController()->activeDocument();
        if (doc) {
            KUrl url = doc->url().upUrl();
            url.addPath( path);
            url.cleanPath();
            url.adjustPath(KUrl::RemoveTrailingSlash);
            path = url.pathOrUrl();
        }
    }
    setFilter( path.split('/', QString::SkipEmptyParts) );
}

uint BaseFileDataProvider::itemCount() const
{
    return filteredItems().count();
}

uint BaseFileDataProvider::unfilteredItemCount() const
{
    return items().count();
}

QuickOpenDataPointer BaseFileDataProvider::data(uint row) const
{
    return QuickOpenDataPointer(new ProjectFileData( filteredItems().at(row) ));
}

ProjectFileDataProvider::ProjectFileDataProvider()
{
    connect(ICore::self()->projectController(), SIGNAL(projectClosing(KDevelop::IProject*)),
            this, SLOT(projectClosing(KDevelop::IProject*)));
    connect(ICore::self()->projectController(), SIGNAL(projectOpened(KDevelop::IProject*)),
            this, SLOT(projectOpened(KDevelop::IProject*)));
}

void ProjectFileDataProvider::projectClosing( IProject* project )
{
    foreach(ProjectFileItem* file, project->files()) {
        fileRemovedFromSet(file);
    }
}

void ProjectFileDataProvider::projectOpened( IProject* project )
{
    const int processAfter = 1000;
    int processed = 0;
    foreach(ProjectFileItem* file, project->files()) {
        fileAddedToSet(file);
        if (++processed == processAfter) {
            // prevent UI-lockup when a huge project was imported
            QApplication::processEvents();
            processed = 0;
        }
    }

    connect(project, SIGNAL(fileAddedToSet(KDevelop::ProjectFileItem*)),
            this, SLOT(fileAddedToSet(KDevelop::ProjectFileItem*)));
    connect(project, SIGNAL(fileRemovedFromSet(KDevelop::ProjectFileItem*)),
            this, SLOT(fileRemovedFromSet(KDevelop::ProjectFileItem*)));
}

void ProjectFileDataProvider::fileAddedToSet( ProjectFileItem* file )
{
    ProjectFile f;
    f.projectPath = file->project()->path();
    f.path = file->path();
    f.indexedPath = file->indexedPath();
    f.outsideOfProject = !f.projectPath.isParentOf(f.path);
    QList<ProjectFile>::iterator it = qLowerBound(m_projectFiles.begin(), m_projectFiles.end(), f);
    if (it == m_projectFiles.end() || it->path != f.path) {
        m_projectFiles.insert(it, f);
    }
}

void ProjectFileDataProvider::fileRemovedFromSet( ProjectFileItem* file )
{
    ProjectFile item;
    item.path = file->path();

    // fast-path for non-generated files
    // NOTE: figuring out whether something is generated is expensive... and since
    // generated files are rare we apply this two-step algorithm here
    QList<ProjectFile>::iterator it = qBinaryFind(m_projectFiles.begin(), m_projectFiles.end(), item);
    if (it != m_projectFiles.end()) {
        m_projectFiles.erase(it);
        return;
    }

    // last try: maybe it was generated
    item.outsideOfProject = true;
    it = qBinaryFind(m_projectFiles.begin(), m_projectFiles.end(), item);
    if (it != m_projectFiles.end()) {
        m_projectFiles.erase(it);
        return;
    }
}

void ProjectFileDataProvider::reset()
{
    clearFilter();

    QList<ProjectFile> projectFiles = m_projectFiles;

    const auto& open = openFiles();
    for(QList<ProjectFile>::iterator it = projectFiles.begin();
        it != projectFiles.end();)
    {
        if (open.contains(it->indexedPath)) {
            it = projectFiles.erase(it);
        } else {
            ++it;
        }
    }

    setItems(projectFiles);
}

QSet<IndexedString> ProjectFileDataProvider::files() const
{
    QSet<IndexedString> ret;

    foreach( IProject* project, ICore::self()->projectController()->projects() )
        ret += project->fileSet();

    return ret - openFiles();
}

void OpenFilesDataProvider::reset()
{
    clearFilter();
    IProjectController* projCtrl = ICore::self()->projectController();
    IDocumentController* docCtrl = ICore::self()->documentController();
    const QList<IDocument*>& docs = docCtrl->openDocuments();

    QList<ProjectFile> currentFiles;
    currentFiles.reserve(docs.size());
    foreach( IDocument* doc, docs ) {
        ProjectFile f;
        f.path = Path(doc->url());
        IProject* project = projCtrl->findProjectForUrl(doc->url());
        if (project) {
            f.projectPath = project->path();
        }
        currentFiles << f;
    }

    qSort(currentFiles);

    setItems(currentFiles);
}

QSet<IndexedString> OpenFilesDataProvider::files() const
{
    return openFiles();
}

#include "projectfilequickopen.moc"
