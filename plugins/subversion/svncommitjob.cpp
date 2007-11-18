/***************************************************************************
 *   This file is part of KDevelop                                         *
 *   Copyright 2007 Andreas Pakulat <apaku@gmx.de>                         *
 *   Copyright 2007 Dukju Ahn <dukjuahn@gmail.com>                         *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU Library General Public License as       *
 *   published by the Free Software Foundation; either version 2 of the    *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this program; if not, write to the                 *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/

#include "svncommitjob.h"
#include "svncommitjob_p.h"

#include <QMutexLocker>

#include <ThreadWeaver.h>
#include <kparts/mainwindow.h>
#include <kdebug.h>

#include <vector>

#include <svncpp/client.hpp>
#include <svncpp/path.hpp>
#include <svncpp/targets.hpp>

#include <iostream>

#include "svninternaljobbase.h"
#include "svncommitdialog.h"
#include "kdevsvnplugin.h"

SvnInternalCommitJob::SvnInternalCommitJob( SvnJobBase* parent )
    : SvnInternalJobBase( parent ), m_recursive( true ), m_keepLock( false )
{
}

void SvnInternalCommitJob::setRecursive( bool recursive )
{
    QMutexLocker l( m_mutex );
    m_recursive = recursive;
}

void SvnInternalCommitJob::setCommitMessage( const QString& msg )
{
    QMutexLocker l( m_mutex );
    m_commitMessage = msg;
}

void SvnInternalCommitJob::setUrls( const KUrl::List& urls )
{
    QMutexLocker l( m_mutex );
    m_urls = urls;
}

void SvnInternalCommitJob::setKeepLock( bool lock )
{
    QMutexLocker l( m_mutex );
    m_keepLock = lock;
}

KUrl::List SvnInternalCommitJob::urls() const
{
    QMutexLocker l( m_mutex );
    return m_urls;
}

QString SvnInternalCommitJob::commitMessage() const
{
    QMutexLocker l( m_mutex );
    return m_commitMessage;
}

bool SvnInternalCommitJob::recursive() const
{
    QMutexLocker l( m_mutex );
    return m_recursive;
}

bool SvnInternalCommitJob::keepLock() const
{
    QMutexLocker l( m_mutex );
    return m_keepLock;
}


void SvnInternalCommitJob::run()
{
    initBeforeRun();
    svn::Client cli(m_ctxt);
    std::vector<svn::Path> targets;
    KUrl::List l = urls();
    foreach( KUrl u, l )
    {
        QByteArray path = u.path().toUtf8();
        targets.push_back( svn::Path( path.data() ) );
    }
    QByteArray ba = commitMessage().toUtf8();
    try
    {
        svn_revnum_t rev = cli.commit( svn::Targets(targets), ba.data(), recursive(), keepLock() );
    }catch( svn::ClientException ce )
    {
        kDebug(9510) << "Couldn't commit:" << QString::fromUtf8( ce.message() );
        setErrorMessage( QString::fromUtf8( ce.message() ) );
        m_success = false;
    }
}

SvnCommitJob::SvnCommitJob( KDevSvnPlugin* parent )
    : SvnJobBase( parent ), m_commitdlg( 0 )
{
    setType( KDevelop::VcsJob::Commit );
    m_job = new SvnInternalCommitJob( this );
}

QVariant SvnCommitJob::fetchResults()
{
    return QVariant();
}

SvnInternalJobBase* SvnCommitJob::internalJob() const
{
    return m_job;
}

void SvnCommitJob::start()
{
    if( m_job->urls().isEmpty() )
    {
        internalJobFailed( m_job );
        setErrorText( i18n( "Not enough information to execute commit" ) );
    }else
    {
        ThreadWeaver::Weaver::instance()->enqueue( m_job );
    }
}

void SvnCommitJob::setCommitMessage( const QString& msg )
{
    if( status() == KDevelop::VcsJob::JobNotStarted )
        m_job->setCommitMessage( msg );
}

void SvnCommitJob::setKeepLock( bool keepLock )
{
    if( status() == KDevelop::VcsJob::JobNotStarted )
        m_job->setKeepLock( keepLock );
}

void SvnCommitJob::setUrls( const KUrl::List& urls )
{
    kDebug(9510) << "Setting urls?" <<  status() << urls;
    if( status() == KDevelop::VcsJob::JobNotStarted )
        m_job->setUrls( urls );
}

void SvnCommitJob::setRecursive( bool recursive )
{
    if( status() == KDevelop::VcsJob::JobNotStarted )
        m_job->setRecursive( recursive );
}



#include "svncommitjob.moc"
#include "svncommitjob_p.moc"
