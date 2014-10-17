/*
 * JSON Tiled Plugin
 * Copyright 2011, Porfírio José Pereira Ribeiro <porfirioribeiro@gmail.com>
 * Copyright 2011, Thorbjørn Lindeijer <thorbjorn@lindeijer.nl>
 *
 * This file is part of Tiled.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef INIPLUGIN_H
#define INIPLUGIN_H

#include "ini_global.h"

#include "mapwriterinterface.h"
#include "mapreaderinterface.h"

#include <QObject>

namespace Tiled {
class Map;
}

namespace Ini {

class INISHARED_EXPORT IniPlugin
		: public QObject
		, public Tiled::MapReaderInterface
		, public Tiled::MapWriterInterface
{
	Q_OBJECT
	Q_INTERFACES(Tiled::MapReaderInterface)
	Q_INTERFACES(Tiled::MapWriterInterface)
#if QT_VERSION >= 0x050000
	Q_PLUGIN_METADATA(IID "org.mapeditor.MapWriterInterface" FILE "plugin.json")
	Q_PLUGIN_METADATA(IID "org.mapeditor.MapReaderInterface" FILE "plugin.json")
#endif

public:
	IniPlugin();
	~IniPlugin();

	// MapReaderInterface
	Tiled::Map *read(const QString &fileName);
	bool supportsFile(const QString &fileName) const;

	// MapWriterInterface
	bool write(const Tiled::Map *map, const QString &fileName);

	// Both interfaces
	QStringList nameFilters() const;
	QString errorString() const;

private:
	bool processLine(Tiled::Map *map, QStringList path, QString value);
	bool readUnknownElement(QStringList path, QString value);
	QString mError;
	//temporary variables
	int t_firstgid;
};

} // namespace Ini

#endif // INIPLUGIN_H
