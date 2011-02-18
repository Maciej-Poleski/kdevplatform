/*
 * KDevelop Debugger Support
 *
 * Copyright 2007 Hamish Rodda <rodda@kde.org>
 * Copyright 2008 Vladimir Prus <ghost@cs.msu.su>
 * Copyright 2009 Niko Sams <niko.sams@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include "variablecollection.h"

#include <QFont>

#include <KLocale>
#include <KDebug>
#include <KTextEditor/TextHintInterface>
#include <KTextEditor/Document>
#include <KParts/PartManager>

#include "../../interfaces/icore.h"
#include "../../interfaces/idocumentcontroller.h"
#include "../../interfaces/iuicontroller.h"
#include "../../sublime/controller.h"
#include "../../sublime/view.h"
#include "../../interfaces/idebugcontroller.h"
#include "../interfaces/idebugsession.h"
#include "../interfaces/ivariablecontroller.h"
#include "../../language/duchain/duchainlock.h"
#include "../../language/duchain/duchainutils.h"
#include "../../language/duchain/declaration.h"
#include "variabletooltip.h"

namespace KDevelop {

IDebugSession* currentSession()
{
    IDebugController* dc = ICore::self()->debugController();
    return dc ? dc->currentSession() : 0;
}

IDebugSession::DebuggerState currentSessionState()
{
    if (!currentSession()) return IDebugSession::NotStartedState;
    return currentSession()->state();
}

bool hasStartedSession()
{
    IDebugSession::DebuggerState s = currentSessionState();
    return s != IDebugSession::NotStartedState && s != IDebugSession::EndedState;
}

Variable::Variable(TreeModel* model, TreeItem* parent,
                   const QString& expression,
                   const QString& display)
  : TreeItem(model, parent),
    inScope_(true), topLevel_(true), changed_(false), m_format(Natural)
{
    expression_ = expression;
    // FIXME: should not duplicate the data, instead overload 'data'
    // and return expression_ directly.
    if (display.isEmpty())
        setData(QVector<QVariant>() << expression << QString());
    else
        setData(QVector<QVariant>() << display << QString());
}

QString Variable::expression() const
{
    return expression_;
}

bool Variable::inScope() const
{
    return inScope_;
}

void Variable::setValue(const QString& v)
{
    itemData[1] = v;
    reportChange();
}

QString Variable::value() const
{
    return itemData[1].toString();
}

void Variable::setTopLevel(bool v)
{
    topLevel_ = v;
}

void Variable::setInScope(bool v)
{
    inScope_ = v;
    for (int i=0; i < childCount(); ++i) {
        if (Variable *var = dynamic_cast<Variable*>(child(i))) {
            var->setInScope(v);
        }
    }
    reportChange();
}

Variable::~Variable()
{
}

void Variable::die()
{
    removeSelf();
    deleteLater();
}


void Variable::setChanged(bool c)
{
    changed_=c;
    reportChange();
}

void Variable::resetChanged()
{
    setChanged(false);
    for (int i=0; i<childCount(); ++i) {
        TreeItem* childItem = child(i);
        if (dynamic_cast<Variable*>(childItem)) {
            static_cast<Variable*>(childItem)->resetChanged();
        }
    }
}

Variable::format_t Variable::str2format(const QString& str)
{
    if(str=="Binary" || str=="binary")          return Binary;
    if(str=="Octal" || str=="octal")            return Octal;
    if(str=="Decimal" || str=="decimal")        return Decimal;
    if(str=="Hexadecimal" || str=="hexadecimal")return Hexadecimal;

    return Natural; // maybe most reasonable default
}

QString Variable::format2str(format_t format)
{
    switch(format) {
        case Natural:       return "natural";
        case Binary:        return "binary";
        case Octal:         return "octal";
        case Decimal:       return "decimal";
        case Hexadecimal:   return "hexadecimal";
        default:            return "";
    }
}


void Variable::setFormat(Variable::format_t format)
{
    if (m_format != format) {
        m_format = format;
        formatChanged();
    }
}

void Variable::formatChanged()
{
}

QVariant Variable::data(int column, int role) const
{
    if (column == 1 && role == Qt::ForegroundRole)
    {
        // FIXME: returning hardcoded gray is bad,
        // but we don't have access to any widget, or pallette
        // thereof, at this point.
        if(!inScope_) return QColor(128, 128, 128);
        if(changed_) return QColor(255, 0, 0);
    }
   
    return TreeItem::data(column, role);
}

Watches::Watches(TreeModel* model, TreeItem* parent)
: TreeItem(model, parent), finishResult_(0)
{
    setData(QVector<QVariant>() << "Auto" << QString());
}

Variable* Watches::add(const QString& expression)
{
    if (!hasStartedSession()) return 0;

    Variable* v = currentSession()->variableController()->createVariable(
        model(), this, expression);
    appendChild(v);
    v->attachMaybe();
    return v;
}

Variable *Watches::addFinishResult(const QString& convenienceVarible)
{
    if( finishResult_ )
    {
        removeFinishResult();
    }
    finishResult_ = currentSession()->variableController()->createVariable(
        model(), this, convenienceVarible, "$ret");
    appendChild(finishResult_);
    finishResult_->attachMaybe();
    return finishResult_;
}

void Watches::removeFinishResult()
{
    if (finishResult_)
    {
        finishResult_->die();
        finishResult_ = 0;
    }
}

void Watches::resetChanged()
{
    for (int i=0; i<childCount(); ++i) {
        TreeItem* childItem = child(i);
        if (dynamic_cast<Variable*>(childItem)) {
            static_cast<Variable*>(childItem)->resetChanged();
        }
    }
}

QVariant Watches::data(int column, int role) const
{
#if 0
    if (column == 0 && role == Qt::FontRole)
    {
        /* FIXME: is creating font again and agian efficient? */
        QFont f = font();
        f.setBold(true);
        return f;
    }
#endif
    return TreeItem::data(column, role);
}

void Watches::reinstall()
{
    for (int i = 0; i < childItems.size(); ++i)
    {
        Variable* v = static_cast<Variable*>(child(i));
        v->attachMaybe();
    }
}

Locals::Locals(TreeModel* model, TreeItem* parent, const QString &name)
: TreeItem(model, parent)
{
    setData(QVector<QVariant>() << name << QString());
}

// update current list of local variables
QList<Variable*> Locals::updateLocals(QStringList locals)
{
    // 1. get all displayed variables into 'existing' list
    QSet<QString> existing, current;
    for (int i = 0; i < childItems.size(); i++)
    {
        Q_ASSERT(dynamic_cast<KDevelop::Variable*>(child(i)));
        Variable* var= static_cast<KDevelop::Variable*>(child(i));
        existing << var->expression();
    }

    // 2. add to view all variables that not present there 
    foreach (const QString& var, locals) {
        current << var;
        // If we currently don't display this local var, add it.
        if( !existing.contains( var ) ) {
            // FIXME: passing variableCollection this way is awkward.
            // In future, variableCollection probably should get a
            // method to create variable.
            Variable* v = 
                currentSession()->variableController()->createVariable(
                    ICore::self()->debugController()->variableCollection(),
                    this, var );
            appendChild( v, false );
        }
    }

    // 3. delete from view variables which do not exist in new list
    for (int i = 0; i < childItems.size(); ++i) {
        KDevelop::Variable* v = static_cast<KDevelop::Variable*>(child(i));
        if (!current.contains(v->expression())) {
            removeChild(i);
            --i;
            // FIXME: check that -var-delete is sent.
            delete v;
        }
    }


    if (hasMore()) {
        setHasMore(false);
    }

    // Variables which changed just value are updated by a call to -var-update.
    // Variables that changed type -- likewise.

    QList<Variable*> ret;
    foreach (TreeItem *i, childItems) {
        Q_ASSERT(dynamic_cast<Variable*>(i));
        ret << static_cast<Variable*>(i);
    }
    return ret;
}

void Locals::resetChanged()
{
    for (int i=0; i<childCount(); ++i) {
        TreeItem* childItem = child(i);
        if (dynamic_cast<Variable*>(childItem)) {
            static_cast<Variable*>(childItem)->resetChanged();
        }
    }
}

Autos::Autos(TreeModel* model, TreeItem* parent)
: TreeItem(model, parent)
{
    setData(QVector<QVariant>() << "Context autos" << QString());
}

QStringList getAutos(IDebugSession* session=0)
{
    QStringList result;
    
    if(!session) session = currentSession();
    if(!session) return result;

    const QString& file=session->currentFile();
    int line=session->currentLine();
    if(file.isEmpty() || line<0) return result;

    // get a current line text
    QString lineText;
    QPair<KUrl,int> openUrl = session->convertToLocalUrl(qMakePair<KUrl,int>( file, line ));
    KDevelop::IDocument* document = KDevelop::ICore::self()
        ->documentController()
        ->openDocument(openUrl.first, KTextEditor::Cursor(openUrl.second, 0));
    if(document)
    {
        lineText = document->textDocument()->line(line);
    }
        
    DUChainReadLocker lock;

    foreach(DUChainUtils::DeclPosPair pair, DUChainUtils::usesInLine(file, line) )
    {
        Declaration* pDecl=pair.first;
        int column=pair.second;

        if(pDecl && pDecl->kind()==Declaration::Instance &&
            !pDecl->isFunctionDeclaration())
        {
            QString expr = session->variableController()->expressionAtPosition(lineText,column);
            if(expr.isEmpty())
                expr= pDecl->identifier().toString();  // should not get here but to be sure we do our best..
                
            if(!expr.isEmpty() && !result.contains(expr))
                result << expr; // should be unique
        }
    }
    
    return result;
}

// updates autos list to display previous and current line references
void Autos::update()
{
    QStringList new_autos=getAutos();
    
    // basically we need to delete variables present only in prev_autos_
    // and show curr_autos_ and new_autos
    // then reset prev_autos_ to curr_autos_ and curr_autos_ to new_autos for next step

    // add new_autos that are not yet shown
    foreach (const QString& var, new_autos) {
        // If we currently don't display this local var, add it.
        if( !curr_autos_.contains(var) && !prev_autos_.contains(var) ) {
            // FIXME: passing variableCollection this way is awkward.
            // In future, variableCollection probably should get a
            // method to create variable.
            Variable* v = 
                currentSession()->variableController()->createVariable(
                    ICore::self()->debugController()->variableCollection(),
                    this, var );
            appendChild( v, false );
        }
    }

    // delete old variables (not in curr_autos_ 
    for (int i = 0; i < childItems.size(); ++i) {
        KDevelop::Variable* v = static_cast<KDevelop::Variable*>(child(i));
        if (!curr_autos_.contains(v->expression()) && !new_autos.contains(v->expression())) {
            removeChild(i);
            --i;
            // FIXME: check that -var-delete is sent.
            delete v;
        }
    }

    prev_autos_ = curr_autos_;
    curr_autos_ = new_autos;

    if (hasMore()) {
        setHasMore(false);
    }

    // call to update variables
    foreach (TreeItem *i, childItems) {
        Q_ASSERT(dynamic_cast<Variable*>(i));
        Variable* var=static_cast<Variable*>(i);
        var->setChanged(false);
        var->attachMaybe();
    }
}


VariablesRoot::VariablesRoot(TreeModel* model)
: TreeItem(model)
{
    watches_ = new Watches(model, this);
    appendChild(watches_, true);
    autos_   = new Autos(model, this);
    appendChild(autos_, true);
}


Locals* VariablesRoot::locals(const QString& name)
{
    if (!locals_.contains(name)) {
        locals_[name] = new Locals(model(), this, name);
        appendChild(locals_[name]);
    }
    return locals_[name];
}

QHash<QString, Locals*> VariablesRoot::allLocals() const
{
    return locals_;
}

void VariablesRoot::resetChanged()
{
    watches_->resetChanged();
    foreach (Locals *l, locals_) {
        l->resetChanged();
    }
}

VariableCollection::VariableCollection(IDebugController* controller)
: TreeModel(QVector<QString>() << "Name" << "Value", controller), m_widgetVisible(false)
{
    universe_ = new VariablesRoot(this);
    setRootItem(universe_);

    //new ModelTest(this);

    connect (ICore::self()->documentController(),
         SIGNAL( textDocumentCreated( KDevelop::IDocument* ) ),
         this,
         SLOT( textDocumentCreated( KDevelop::IDocument* ) ) );

    connect(controller, SIGNAL(currentSessionChanged(KDevelop::IDebugSession*)),
             SLOT(updateAutoUpdate(KDevelop::IDebugSession*)));

    connect(locals(), SIGNAL(expanded()), SLOT(updateAutoUpdate()));
    connect(locals(), SIGNAL(collapsed()), SLOT(updateAutoUpdate()));
    connect(watches(), SIGNAL(expanded()), SLOT(updateAutoUpdate()));
    connect(watches(), SIGNAL(collapsed()), SLOT(updateAutoUpdate()));
    connect(autos(), SIGNAL(expanded()), SLOT(updateAutoUpdate()));
    connect(autos(), SIGNAL(collapsed()), SLOT(updateAutoUpdate()));
}

void VariableCollection::variableWidgetHidden()
{
    m_widgetVisible = false;
    updateAutoUpdate();
}

void VariableCollection::variableWidgetShown()
{
    m_widgetVisible = true;
    updateAutoUpdate();
}

void VariableCollection::updateAutoUpdate(IDebugSession* session)
{
    if (!session) session = currentSession();
    kDebug() << session;
    if (!session) return;

    if (!m_widgetVisible) {
        session->variableController()->setAutoUpdate(IVariableController::UpdateNone);
    } else {
        QFlags<IVariableController::UpdateType> t = IVariableController::UpdateNone;
        if (locals()->isExpanded()) t |= IVariableController::UpdateLocals;
        if (watches()->isExpanded()) t |= IVariableController::UpdateWatches;
        if (autos()->isExpanded()) t |= IVariableController::UpdateAutos;
        session->variableController()->setAutoUpdate(t);
    }
}

VariableCollection::~ VariableCollection()
{
}

void VariableCollection::textDocumentCreated(IDocument* doc)
{
  connect( doc->textDocument(),
       SIGNAL( viewCreated( KTextEditor::Document* , KTextEditor::View* ) ),
       this, SLOT( viewCreated( KTextEditor::Document*, KTextEditor::View* ) ) );

  foreach( KTextEditor::View* view, doc->textDocument()->views() )
    viewCreated( doc->textDocument(), view );
}

void VariableCollection::viewCreated(KTextEditor::Document* doc,
                                     KTextEditor::View* view)
{
    Q_UNUSED(doc);
    using namespace KTextEditor;
    TextHintInterface *iface = dynamic_cast<TextHintInterface*>(view);
    if( !iface )
        return;

    iface->enableTextHints(500);

    connect(view,
            SIGNAL(needTextHint(const KTextEditor::Cursor&, QString&)),
            this,
            SLOT(textHintRequested(const KTextEditor::Cursor&, QString&)));
}

void VariableCollection::
textHintRequested(const KTextEditor::Cursor& cursor, QString&)
{
    // Don't do anything if there's already an open tooltip.
    if (activeTooltip_)
        return;

    if (!hasStartedSession())
        return;

    // Figure what is the parent widget and what is the text to show
    KTextEditor::View* view = dynamic_cast<KTextEditor::View*>(sender());
    if (!view)
        return;

    KTextEditor::Document* doc = view->document();

    QString expression = currentSession()->variableController()->
            expressionAtPosition(doc->line(cursor.line()), cursor.column());
   
    if (expression.isEmpty())
        return;

    QPoint local = view->cursorToCoordinate(cursor);
    QPoint global = view->mapToGlobal(local);
    QWidget* w = view->childAt(local);
    if (!w)
        w = view;

    activeTooltip_ = new VariableToolTip(w, global+QPoint(30,30), expression);
}

}

#include "variablecollection.moc"
