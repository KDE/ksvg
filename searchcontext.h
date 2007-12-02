/*
 *   Copyright 2006-2007 Aaron Seigo <aseigo@kde.org>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library General Public License as
 *   published by the Free Software Foundation; either version 2, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details
 *
 *   You should have received a copy of the GNU Library General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef SEARCHCONTEXT_H
#define SEARCHCONTEXT_H

#include <QtCore/QList>
#include <QtCore/QObject>

#include <plasma/plasma_export.h>

class KCompletion;

namespace Plasma
{

class SearchAction;
class AbstractRunner;

/**
 * @short The SearchContext class provides information related to a search,
 *        including the search term, metadata on the search term and collected
 *        matches.
 */
class PLASMA_EXPORT SearchContext : public QObject
{
    Q_OBJECT

    public:
        enum Type { UnknownType = 0,
                    Directory,
                    File,
                    NetworkLocation,
                    Executable,
                    ShellCommand,
                    Help
                  };

        explicit SearchContext(QObject *parent = 0);
        ~SearchContext();

        SearchContext(QObject *parent, const SearchContext& other);

        /**
         * Sets the search term for this object. This clears all current
         * matches in the process.
         */
        void setSearchTerm(const QString&);

        /**
         * @return the current search term.
         */
        QString searchTerm() const;

        /**
         * The type of item the search term might refer to.
         * @see Type
         */
        Type type() const;

        /**
         * The mimetype that the search term refers to, if discoverable.
         *
         * @return QString() if the mimetype can not be determined, otherwise
         *         the mimetype of the object being refered to by the search
         *         string.
         */
        QString mimetype() const;

        /**
         * @return a completion object that can be used with UI elements
         */
        KCompletion* completionObject() const;

        /**
         * Adds an item to the completion object.
         */
        void addStringCompletion(const QString& completion);

        /**
         * Adds multiple items to the completion object.
         */
        void addStringCompletions(const QStringList& completions);

        /**
         * Add an action that represents a match to the current search term.
         * This action is informational in nature and does not represent and actionable
         * match.
         *
         * If string data is added to the action using QAction::setData(), that
         * string may be used in user interfaces when the item is selected.
         */
        SearchAction* addInformationalMatch(AbstractRunner *runner);

        /**
         * Add an action that represents an exact match to the current search term.
         */
        SearchAction* addExactMatch(AbstractRunner *runner);

        /**
         * Add an action that represents a possible match to the current search term.
         */
        SearchAction* addPossibleMatch(AbstractRunner *runner);

        /**
         * Appends lists of matches to the lists for exact, possible, and informational matches
         * @return true if matches were added, false if matches were outdated
         */
        bool addMatches( const QString& term, const QList<SearchAction *> &exactMatches,
                                     const QList<SearchAction *> &possibleMatches,
                                     const QList<SearchAction *> &informationalMatches );

        /**
         * Retrieves all available informational matches for the current
         * search term.
         */
        QList<SearchAction *> informationalMatches() const;

        /**
         * Retrieves all available exact matches for the current
         * search term.
         */
        QList<SearchAction *> exactMatches() const;

        /**
         * Retrieves all available possible matches for the current
         * search term.
         */
        QList<SearchAction *> possibleMatches() const;

        /**
         * Determines type of query
         */
        void determineType();

        /**
         * Clears matches
         */
        void clearMatches();

    signals:
        void matchesChanged();

    private:
        class Private;
        Private * const d;
};

}

#endif
