/***************************************************************************
 *   Copyright (C) 2011~2011 by CSSlayer                                   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.              *
 ***************************************************************************/

// Qt
#include <QDir>
#include <QSet>

// Fcitx
#include <fcitx-config/xdg.h>

// self
#include "config.h"
#include "FcitxSubConfigPattern.h"
#include "FcitxSubConfigParser.h"

namespace Fcitx
{
FcitxSubConfigParser::FcitxSubConfigParser(const QString& subConfigString, QObject* parent) :
    QObject(parent)
{
    QStringList subConfigList = subConfigString.split(',');
    Q_FOREACH(const QString & str, subConfigList) {
        int i = str.indexOf(':');
        if (i < 0)
            continue;
        QString namestr = str.section(':', 0, 0);
        if (namestr.length() == 0)
            continue;
        QString typestr = str.section(':', 1, 1);
        if (typestr == "domain") {
            m_domain = namestr;
            continue;
        }
        SubConfigType type = parseType(typestr);
        if (type == SC_None)
            continue;
        if (m_subConfigMap.count(namestr) > 0)
            continue;
        QString patternstr = str.section(':', 2, -1);
        FcitxSubConfigPattern* pattern = FcitxSubConfigPattern::parsePattern(type, patternstr, this);
        if (pattern == NULL)
            continue;
        m_subConfigMap[namestr] = pattern;
    }
}

SubConfigType FcitxSubConfigParser::parseType(const QString& str)
{
    if (str == "native") {
        return SC_NativeFile;
    }
    if (str == "configfile") {
        return SC_ConfigFile;
    }
    return SC_None;
}

QStringList FcitxSubConfigParser::getSubConfigKeys()
{
    return m_subConfigMap.keys();
}

QSet<QString> FcitxSubConfigParser::getFiles(const QString& name)
{
    if (m_subConfigMap.count(name) != 1)
        return QSet<QString> ();
    FcitxSubConfigPattern* pattern = m_subConfigMap[name];
    size_t size;
    char** xdgpath = FcitxXDGGetPath(&size, "XDG_CONFIG_HOME", ".config" , PACKAGE , DATADIR, PACKAGE);

    QSet<QString> result;
    for (size_t i = 0; i < size; i ++) {
        QDir dir(xdgpath[i]);
        QStringList list = getFilesByPattern(dir, pattern, 0);
        Q_FOREACH(const QString & str, list) {
            result.insert(
                dir.relativeFilePath(str));
        }
    }

    FcitxXDGFreePath(xdgpath);

    return result;
}

QStringList FcitxSubConfigParser::getFilesByPattern(QDir& currentdir, FcitxSubConfigPattern* pattern, int index)
{
    QStringList result;
    if (!currentdir.exists())
        return result;

    const QString& filter = pattern->getPattern(index);
    QStringList filters;
    filters << filter;
    QDir::Filters filterflag;

    if (index + 1 == pattern->size()) {
        filterflag = QDir::Files;
    } else {
        filterflag = QDir::Dirs | QDir::NoDotAndDotDot;
    }

    QStringList list = currentdir.entryList(filters, filterflag);
    if (index + 1 == pattern->size()) {
        Q_FOREACH(const QString & item, list) {
            result << currentdir.absoluteFilePath(item);
        }
    } else {
        Q_FOREACH(const QString & item, list) {
            QDir dir(currentdir.absoluteFilePath(item));
            result << getFilesByPattern(dir, pattern, index + 1);
        }
    }
    return result;
}

FcitxSubConfig* FcitxSubConfigParser::getSubConfig(const QString& key)
{
    if (m_subConfigMap.count(key) != 1)
        return NULL;

    FcitxSubConfigPattern* pattern = m_subConfigMap[key];

    FcitxSubConfig* subconfig = NULL;

    switch (pattern->type()) {
    case SC_ConfigFile:
        subconfig = FcitxSubConfig::GetConfigFileSubConfig(key, pattern->configdesc(), this->getFiles(key));
        break;
    case SC_NativeFile:
        subconfig = FcitxSubConfig::GetNativeFileSubConfig(key, pattern->nativepath(), this->getFiles(key));
        break;
    default:
        break;
    }

    return subconfig;
}

const QString& FcitxSubConfigParser::domain() const
{
    return m_domain;
}

}

#include "moc_FcitxSubConfigParser.cpp"
