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

#ifndef KDEVPLATFORM_CODEREPRESENTATION_H
#define KDEVPLATFORM_CODEREPRESENTATION_H

#include "../languageexport.h"
#include <language/duchain/indexedstring.h>

#include <ksharedptr.h>

class QString;

namespace KTextEditor {
    class Range;
}

namespace KDevelop {

    class SimpleRange;
    class IndexedString;
    
/**
  * Allows getting code-lines conveniently, either through an open editor, or from a disk-loaded file.
  */
class KDEVPLATFORMLANGUAGE_EXPORT CodeRepresentation : public QSharedData {
  public:
    virtual ~CodeRepresentation() {
    }
    virtual QString line(int line) const = 0;
    virtual int lines() const = 0;
    virtual QString text() const = 0;
    virtual QString rangeText(const KTextEditor::Range& range) const;
    /**
     * Search for the given identifier in the document, and returns all ranges
     * where it was found.
     * @param identifier The identifier to search for
     * @param surroundedByBoundary Whether only matches that are surrounded by typical word-boundaries
     *                             should be acceded. Everything except letters, numbers, and the _ character
     *                             counts as word boundary.
     * */
    virtual QVector<SimpleRange> grep(const QString& identifier, bool surroundedByBoundary = true) const = 0;
    /**
      * Overwrites the text in the file with the new given one
      *
      * @return true on success
      */
    virtual bool setText(const QString&) = 0;
    /** @return true if this representation represents an actual file on disk */
    virtual bool fileExists() = 0;
    
    /** 
      * Can be used for example from tests to disallow on-disk changes. When such a change is done, an assertion is triggered.
      * You should enable this within tests, unless you really want to work on the disk.
      */
    static void setDiskChangesForbidden(bool changesForbidden);
   
    /**
      * Returns the specified name as a url for aritificial source code
      * suitable for code being inserted into the parser
      */
    static QString artificialPath(const QString & name);
 
    typedef KSharedPtr<CodeRepresentation> Ptr;
};

class KDEVPLATFORMLANGUAGE_EXPORT DynamicCodeRepresentation : public CodeRepresentation {
  public:
      /** Used to group edit-history together. Call this optionally before a bunch
       *  of replace() calls, and then call endEdit in the end, to group them together. */
      virtual void startEdit() = 0;
      virtual bool replace(const KTextEditor::Range& range, const QString& oldText,
                           const QString& newText, bool ignoreOldText = false) = 0;
      /** Must be called exactly once per startEdit() */
      virtual void endEdit() = 0;
    
      typedef KSharedPtr<DynamicCodeRepresentation> Ptr;
};

/**
  * Creates a code-representation for the given url, that allows conveniently accessing its data. Returns zero on failure.
  */
KDEVPLATFORMLANGUAGE_EXPORT CodeRepresentation::Ptr createCodeRepresentation(const IndexedString& url);

/**
  * @return true if an artificial code representation already exists for the specified URL
  */
KDEVPLATFORMLANGUAGE_EXPORT bool artificialCodeRepresentationExists(const IndexedString& url);

/**
 *   Allows inserting artificial source-code into the code-representation and parsing framework.
 *  The source-code logically represents a file.
 *
 *  The artificial code is automatically removed when the object is destroyed.
 */
class KDEVPLATFORMLANGUAGE_EXPORT InsertArtificialCodeRepresentation : public QSharedData {
    public:
        /**
         * Inserts an artifial source-code representation with filename @p file and the contents @p text
         * If @p file is not an absolute path or url, it will be made absolute using the CodeRepresentation::artifialUrl()
         * function, while ensuring that the name is unique.
         */
        InsertArtificialCodeRepresentation(const IndexedString& file, const QString& text);
        ~InsertArtificialCodeRepresentation();
        
        void setText(const QString& text);
        QString text();
        /**
          * Returns the file-name for this code-representation.
          */
        IndexedString file();
    private:
        InsertArtificialCodeRepresentation(const InsertArtificialCodeRepresentation&);
        InsertArtificialCodeRepresentation& operator=(const InsertArtificialCodeRepresentation&);
        
        IndexedString m_file;
};

typedef KSharedPtr<InsertArtificialCodeRepresentation> InsertArtificialCodeRepresentationPointer;

}


#endif
