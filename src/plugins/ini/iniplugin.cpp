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

#include "iniplugin.h"

#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QColor>

#if QT_VERSION >= 0x050100
#	define HAS_QSAVEFILE_SUPPORT
#endif

#ifdef HAS_QSAVEFILE_SUPPORT
#	include <QSaveFile>
#endif

#include <assert.h>

#include "imagelayer.h"
#include "map.h"
#include "mapobject.h"
#include "objectgroup.h"
#include "properties.h"
#include "tile.h"
#include "tilelayer.h"
#include "tileset.h"

using namespace Ini;

IniPlugin::IniPlugin()
{
	//map=new Map(Map::Orientation::Unknown, 0, 0, 0, 0);
}

IniPlugin::~IniPlugin() {
	//delete map;
}
Tiled::Map *IniPlugin::read(const QString &fileName)
{
	QFile file(fileName);
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
		mError = tr("Could not open file for reading.");
		return 0;
	}

	Tiled::Map *map=new Tiled::Map((Tiled::Map::Orientation)(Tiled::Map::Unknown), 0, 0, 0, 0);
	QTextStream in(&file);

	bool res=true;
	while (!in.atEnd()) {
		QString line = in.readLine();

		QString key = line.section('=', 0, 0);
		key = key.trimmed();
		QString value = line.section('=', 1);
		value = value.trimmed();
		QStringList path = key.split('.');

		res &= process_line(map, path, value);
		if(!res) return 0;
	}

	return map;
}

bool IniPlugin::process_line(Tiled::Map *map, QStringList path, QString value) {
	if(mError.size()>0) return false;
	if(path[0]=="main") {
		assert(path.size()==2);
		assert(path[1]=="version");
		assert(value=="0.0.1");
	}
	else if(path[0]=="map") {
		if(path.size()==2) { //map attributes
			if(path[1]=="version") {
				assert(value=="1.0");
			}
			else if(path[1]=="orientation") {
				map->setOrientation(Tiled::orientationFromString(value));
			}
			else if(path[1]=="width") {
				bool ok;
				int v=value.toInt(&ok);
				assert(ok);
				map->setWidth(v);
			}
			else if(path[1]=="height") {
				bool ok;
				int v=value.toInt(&ok);
				assert(ok);
				map->setHeight(v);
			}
			else if(path[1]=="tilewidth") {
				bool ok;
				int v=value.toInt(&ok);
				assert(ok);
				map->setTileWidth(v);
			}
			else if(path[1]=="tileheight") {
				bool ok;
				int v=value.toInt(&ok);
				assert(ok);
				map->setTileHeight(v);
			}
			else if(path[1]=="backgroundcolor") {
				QColor v=QColor(value);
				assert(v.isValid());
				map->setBackgroundColor(v);
			}
			else if(path[1]=="renderorder") {
				map->setRenderOrder(Tiled::renderOrderFromString(value));
			}
			else {
				mError = "Unknown map attribute "+path[1]+" with value "+value;
				return 0;
			}
		}
		else {
			//content (properties, tilesets, *layers)
			if(path[1]=="properties") { //map properties
				assert(path.size()==3);
				map->setProperty(path[2], value);
			}
			else if(path[1]=="tileset") {
				assert(path.size()>=4);
				int i=path[2].toInt();
				if(i>map->tilesetCount()) {
					//map->addTileset(new Tiled::Tileset("",0,0));//do it later
					t_firstgid=0;
					t_name="";
					t_tilewidth=0;
					t_tileheight=0;
					t_spacing=0;
					t_margin=0;//*/
				}
				if(path.size()==4) { //tileset attributes
					if(path[3]=="firstgid") {
						//t_firstgid=value.toInt();
					}
					else if(path[3]=="name") {
						t_name=value;
						//map->tilesetAt(i)->setName(value);
					}
					else if(path[3]=="tilewidth") {
						t_tilewidth=value.toInt();
						//map->tilesetAt(i)->mTileWidth=value.toInt();
					}
					else if(path[3]=="tileheight") {
						t_tileheight=value.toInt();
						//map->tilesetAt(i)->mTileHeight=value.toInt();
					}
					else if(path[3]=="spacing") {
						t_spacing=value.toInt();
						//map->tilesetAt(i)->mTileSpacing=value.toInt();
					}
					else if(path[3]=="margin") {
						t_margin=value.toInt();
						//map->tilesetAt(i)->mMargin=value.toInt();
					}
					else if(path[3]=="source") {
						if(i>map->tilesetCount()) {
							map->addTileset(new Tiled::Tileset(t_name, t_tilewidth, t_tileheight, t_spacing, t_margin));
						}//*/
						map->tilesetAt(i)->setFileName(value);
					}
				}
				else if(path.size()==5) {
					if(path[3]=="image") { //tileset image
						if(path[4]=="source") {
							//map->tilesetAt(i)->mImageSource=value;
							map->tilesetAt(i)->loadFromImage(value);
						}
						else if(path[4]=="trans") {
							QColor v=QColor(value);
							assert(v.isValid());
							map->tilesetAt(i)->setTransparentColor(v);
						}
						else if(path[4]=="width") {
							//map->tilesetAt(i)->mImageWidth=value;//NOTE: loadFromImage
						}
						else if(path[4]=="height") {
							//map->tilesetAt(i)->mImageHeight=value;//NOTE: loadFromImage
						}
					}
					if(path[3]=="tileoffset") {
						if(path[4]=="x") {
							//map->tilesetAt(i)->mTileOffset.setX(value.toInt());
							//t_to_x=value.toInt();
						}
						else if(path[4]=="y") {
							//map->tilesetAt(i)->mTileOffset.setY(value.toInt());
							//t_to_y=value.toInt();
						}
						//map->tilesetAt(i)->setTileOffset(QPoint(t_to_x, t_to_y));
					}
				}
			}
			else if(path[1]=="layer") {
				//
			}
			else if(path[1]=="objectgroup") {
				//
			}
			else if(path[1]=="imagelayer") {
				//
			}
			else {
				mError="Unknown map node/attribute ";
				mError += value;
				return 0;
			}
		}
	}
	return true;
}

bool IniPlugin::write(const Tiled::Map *map, const QString &fileName)
{
#ifdef HAS_QSAVEFILE_SUPPORT
	QSaveFile file(fileName);
#else
	QFile file(fileName);
#endif
	if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
		mError = tr("Could not open file for writing.");
		return 0;
	}
	QTextStream out(&file);
	if(true) { //write content
		out << "main.version = 0.0.1\n";
		if(true) { //map attributes
			out << "map.version = 1.0\n";
			out << "map.orientation" << " = " << Tiled::orientationToString(map->orientation()) << endl;
			out << "map.width" << " = " << map->width() << endl;
			out << "map.height" << " = " << map->height() << endl;
			out << "map.tilewidth" << " = " << map->tileWidth() << endl;
			out << "map.tileheight" << " = " << map->tileHeight() << endl;
			QColor color=map->backgroundColor();
			if(color.isValid()) {
				out << "map.backgroundcolor" << " = " << color.name() << endl;
			}
			out << "map.renderorder" << " = " << Tiled::renderOrderToString(map->renderOrder()) << endl;
		}
		if(!map->properties().empty()) { //map properties
			Tiled::Properties::const_iterator end=map->properties().constEnd();
			for(Tiled::Properties::const_iterator it=map->properties().constBegin();it!=end;++it) {
				out << "map.properties." << it.key() << " = " << it.value() << endl;
			}
		}
		if(map->tilesetCount()>0) {
			for(int i=0;i<map->tilesetCount();++i) {
				out << "map.tileset." << i << ".name" << " = " << map->tilesetAt(i)->name() << endl;
				out << "map.tileset." << i << ".tilewidth" << " = " << map->tilesetAt(i)->tileWidth() << endl;
				out << "map.tileset." << i << ".tileheight" << " = " << map->tilesetAt(i)->tileHeight() << endl;
				out << "map.tileset." << i << ".spacing" << " = " << map->tilesetAt(i)->tileSpacing() << endl;
				out << "map.tileset." << i << ".margin" << " = " << map->tilesetAt(i)->margin() << endl;
				out << "map.tileset." << i << ".source" << " = " << map->tilesetAt(i)->fileName() << endl;
				out << "map.tileset." << i << ".image.source" << " = " << map->tilesetAt(i)->imageSource() << endl;
				out << "map.tileset." << i << ".image.trans" << " = " << map->tilesetAt(i)->transparentColor().name() << endl;
				//out << "map.tileset." << i << ".image.width" << " = " << map->tilesetAt(i)->imageWidth() << endl;
				//out << "map.tileset." << i << ".image.height" << " = " << map->tilesetAt(i)->imageHeight() << endl;
				//out << "map.tileset." << i << ".tileoffset.x" << " = " << map->tilesetAt(i)->tileOffset().x() << endl;
				//out << "map.tileset." << i << ".tileoffset.y" << " = " << map->tilesetAt(i)->tileOffset().y() << endl;
			}
		}
	}
	out.flush();

	if (file.error() != QFile::NoError) {
		mError = tr("Error while writing file:\n%1").arg(file.errorString());
		return false;
	}

#ifdef HAS_QSAVEFILE_SUPPORT
	if (!file.commit()) {
		mError = file.errorString();
		return false;
	}
#endif

	return true;
}

QStringList IniPlugin::nameFilters() const
{
	QStringList filters;
	filters.append(tr("INI files (*.ini)"));
	return filters;
}

bool IniPlugin::supportsFile(const QString &fileName) const
{
	return fileName.endsWith(QLatin1String(".ini"), Qt::CaseInsensitive);
}

QString IniPlugin::errorString() const
{
	return mError;
}

#if QT_VERSION < 0x050000
	Q_EXPORT_PLUGIN2(Ini, IniPlugin)
#endif
