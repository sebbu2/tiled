/*
 * INI Tiled Plugin
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
#include "terrain.h"

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

		res &= processLine(map, path, value);
		if(!res) return 0;
	}

	return map;
}

bool IniPlugin::readUnknownElement(QStringList path, QString value) {
	mError = "Unknown element "+path.join(' ')+" with value "+value+".";
	return 0;
}

bool IniPlugin::processLayer(Tiled::Map *map, QStringList path, int i, QString value) {
	if(path[3]=="name") {
		map->layerAt(i)->setName(value);
	}
	else if(path[3]=="opacity") {
		bool ok;
		float v=value.toFloat(&ok);
		assert(v>=0&&v<=1);
		assert(ok);
		map->layerAt(i)->setOpacity(v);
	}
	else if(path[3]=="visible") {
		bool ok;
		short v=value.toShort(&ok);
		assert(v>=0&&v<=1);
		assert(ok);
		map->layerAt(i)->setVisible(v);
	}
	else if(path[3]=="x") {
		bool ok;
		int v=value.toInt(&ok);
		assert(ok);
		map->layerAt(i)->setX(v);
	}
	else if(path[3]=="y") {
		bool ok;
		int v=value.toInt(&ok);
		assert(ok);
		map->layerAt(i)->setY(v);
	}
	else if(path[3]=="width") {
		bool ok;
		int v=value.toInt(&ok);
		assert(ok);
		map->layerAt(i)->setWidth(v);
	}
	else if(path[3]=="height") {
		bool ok;
		int v=value.toInt(&ok);
		assert(ok);
		map->layerAt(i)->setHeight(v);
	}
	else return false;
	return true;
}

Tiled::Cell IniPlugin::cellFromGID(Tiled::Map *map, QString value) {
	QStringList t = value.split(',');
	bool ok;
	const int tid=t[0].toInt(&ok);
	assert(ok);
	assert(tid<map->tilesetCount());
	const int lid=t[1].toInt(&ok);
	assert(ok);
	assert(lid<map->tilesetAt(tid)->tileCount());
	const int flips=t[2].toInt(&ok);
	assert(ok);
	assert(flips>=0 && flips<=7);
	Tiled::Cell c=Tiled::Cell(map->tilesetAt(tid)->tileAt(lid));
	c.flippedHorizontally=(flips>>2);
	c.flippedVertically=(flips>>1 & 0x1);
	c.flippedAntiDiagonally=(flips & 0x1);
	return c;
}

bool IniPlugin::processLine(Tiled::Map *map, QStringList path, QString value) {
	if(mError.size()>0) return false;
	if(path[0]=="main") {
		assert(path.size()==2);
		assert(path[1]=="version");
		assert(value=="0.0.1"||value=="0.0.2");
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
			else return readUnknownElement(path, value);
		}
		else {
			//content (properties, tilesets, *layers)
			if(path[1]=="properties") { //map properties
				assert(path.size()==3);
				map->setProperty(path[2], value);
			}
			else if(path[1]=="tileset") {
				assert(path.size()>=4 && path.size()<=7);
				bool ok;
				int i=path[2].toInt(&ok);
				assert(ok);
				while(i>=map->tilesetCount()) {
					//map->addTileset(new Tiled::Tileset("",0,0));
					map->addTileset(new Tiled::Tileset());
					t_firstgid=0;
				}
				if(path.size()==4) { //tileset attributes
					if(path[3]=="firstgid") {
						bool ok;
						t_firstgid=value.toInt(&ok);
						assert(ok);
					}
					else if(path[3]=="name") {
						map->tilesetAt(i)->setName(value);
					}
					else if(path[3]=="tilewidth") {
						bool ok;
						int v=value.toInt(&ok);
						assert(ok);
						map->tilesetAt(i)->setTileWidth(v);
					}
					else if(path[3]=="tileheight") {
						bool ok;
						int v=value.toInt(&ok);
						assert(ok);
						map->tilesetAt(i)->setTileHeight(v);
					}
					else if(path[3]=="spacing") {
						bool ok;
						int v=value.toInt(&ok);
						assert(ok);
						map->tilesetAt(i)->setTileSpacing(v);
					}
					else if(path[3]=="margin") {
						bool ok;
						int v=value.toInt(&ok);
						assert(ok);
						map->tilesetAt(i)->setMargin(v);
					}
					else if(path[3]=="source") {
						map->tilesetAt(i)->setFileName(value);
					}
					else return readUnknownElement(path, value);
				}
				else if(path.size()==5) {
					if(path[3]=="image") { //tileset image
						if(path[4]=="source") {
							map->tilesetAt(i)->loadFromImage(value);
						}
						else if(path[4]=="trans") {
							QColor v=QColor(value);
							assert(v.isValid());
							map->tilesetAt(i)->setTransparentColor(v);
						}
						else if(path[4]=="width") {
							bool ok;
							int v=value.toInt(&ok);
							assert(ok);
							map->tilesetAt(i)->setImageWidth(v);//NOTE: loadFromImage
						}
						else if(path[4]=="height") {
							bool ok;
							int v=value.toInt(&ok);
							assert(ok);
							map->tilesetAt(i)->setImageHeight(v);//NOTE: loadFromImage
						}
						else return readUnknownElement(path, value);
					}
					else if(path[3]=="tileoffset") {
						if(path[4]=="x") {
							bool ok;
							int v=value.toInt(&ok);
							assert(ok);
							map->tilesetAt(i)->setTileOffsetX(v);
						}
						else if(path[4]=="y") {
							bool ok;
							int v=value.toInt(&ok);
							assert(ok);
							map->tilesetAt(i)->setTileOffsetY(v);
						}
						else return readUnknownElement(path, value);
					}
					else if(path[3]=="properties") { //tileset properties
						assert(path.size()==5);
						map->tilesetAt(i)->setProperty(path[4], value);
					}
					else return readUnknownElement(path, value);
				}
				else if(path.size()==6) {
					bool ok;
					int i2=path[4].toInt(&ok);
					assert(ok);
					if(path[3]=="terraintypes") {
						while(i2>=map->tilesetAt(i)->terrainCount()) {
							map->tilesetAt(i)->addTerrain("",0);
						}
						for(int a=0;a<map->tilesetAt(i)->terrainCount();++a) {
							map->tilesetAt(i)->terrain(a)->setId(a);
						}
						if(path[5]=="name") {
								map->tilesetAt(i)->terrain(i2)->setName(value);
							}
						else if(path[5]=="tid") {
								bool ok;
								int v=value.toInt(&ok);
								assert(ok);
								map->tilesetAt(i)->terrain(i2)->setImageTileId(v);
							}
						else return readUnknownElement(path, value);
					}
					else if(path[3]=="tile") {
						if(path[5]=="terrain") {
							QStringList quadrants = value.split(QLatin1String(","));
							if (quadrants.size() == 4) {
								for (int a = 0; a < 4; ++a) {
									int t = quadrants[a].isEmpty() ? -1 : quadrants[a].toInt();
									map->tilesetAt(i)->tileAt(i2)->setCornerTerrain(i, t);
								}
							}
						}
						else if(path[5]=="probability") {
							bool ok;
							float v=value.toFloat(&ok);
							assert(ok);
							map->tilesetAt(i)->tileAt(i2)->setTerrainProbability(v);
						}
						else return readUnknownElement(path, value);
					}
					else return readUnknownElement(path, value);
				}
				else if(path.size()==7) { //map tileset (terraintype|tile) properties
					bool ok;
					int i2=path[4].toInt(&ok);
					assert(ok);
					if(path[3]=="terraintype") { // map tileset terraintype properties
						while(i2>=map->tilesetAt(i)->terrainCount()) {
							map->tilesetAt(i)->addTerrain("",0);
						}
						for(int a=0;a<map->tilesetAt(i)->terrainCount();++a) {
							map->tilesetAt(i)->terrain(a)->setId(a);
						}
						if(path[5]=="properties") {
							map->tilesetAt(i)->terrain(i2)->setProperty(path[6], value);
						}
						else return readUnknownElement(path, value);
					}
					else if(path[3]=="tile") { //map tileset tile properties
						if(path[5]=="properties") {
							map->tilesetAt(i)->tileAt(i2)->setProperty(path[6], value);
						}
						else return readUnknownElement(path, value);
					}
					else return readUnknownElement(path, value);
				}
				else return readUnknownElement(path, value);
			}
			else if(path[1]=="layer"||path[1]=="tilelayer") {
				assert(path.size()>=4 && path.size()<=6);
				bool ok;
				int i=path[2].toInt(&ok);
				assert(ok);
				while(i>=map->layerCount()) {
					map->addLayer(new Tiled::TileLayer("",0,0,0,0));
				}
				if(!map->layerAt(i)->isTileLayer()) {
					map->takeLayerAt(i);
					map->insertLayer(i,new Tiled::TileLayer("",0,0,0,0));
				}
				if(path.size()==4) { //tilelayer attribute
					bool l=processLayer(map, path, i, value);
					if(l) {}
					else if(path[3]=="encoding") {
							//TODO: ??
					}
					else if(path[3]=="compression") {
						//TODO: ??
					}
					else return readUnknownElement(path, value);
				}
				else if(path.size()==6) { //layer data
					assert(path[3]=="tile");
					bool ok;
					const int y=path[4].toInt(&ok);
					assert(ok);
					const int x=path[5].toInt(&ok);
					assert(ok);
					Tiled::Cell c=cellFromGID(map, value);
					map->layerAt(i)->asTileLayer()->setCell(x, y, c);
				}
				else return readUnknownElement(path, value);
			}
			else if(path[1]=="objectgroup"||path[1]=="objectlayer") {
				assert(path.size()>=4 && path.size()<=6);
				bool ok;
				int i=path[2].toInt(&ok);
				assert(ok);
				while(i>=map->layerCount()) {
					map->addLayer(new Tiled::ObjectGroup("",0,0,0,0));
				}
				if(!map->layerAt(i)->isObjectGroup()) {
					map->takeLayerAt(i);
					map->insertLayer(i,new Tiled::ObjectGroup("",0,0,0,0));
				}
				if(path.size()==4) { //objectlayer attribute
					bool l=processLayer(map, path, i, value);
					if(l) {}
					else if(path[3]=="color") {
						QColor v(value);
						assert(v.isValid());
						map->layerAt(i)->asObjectGroup()->setColor(v);
					}
					else return readUnknownElement(path, value);
				}
				else if(path.size()==6) { //objects
					assert(path[3]=="object");
					int i2=path[4].toInt(&ok);
					assert(ok);
					while(i2>=map->layerAt(i)->asObjectGroup()->objectCount()) {
						map->layerAt(i)->asObjectGroup()->addObject(new Tiled::MapObject());
					}
					if(path[5]=="name") {
						map->layerAt(i)->asObjectGroup()->objectAt(i2)->setName(value);
					}
					else if(path[5]=="type") {
						map->layerAt(i)->asObjectGroup()->objectAt(i2)->setType(value);
					}
					else if(path[5]=="x") {
						bool ok;
						int v=value.toInt(&ok);
						assert(ok);
						map->layerAt(i)->asObjectGroup()->objectAt(i2)->setX(v);
					}
					else if(path[5]=="y") {
						bool ok;
						int v=value.toInt(&ok);
						assert(ok);
						map->layerAt(i)->asObjectGroup()->objectAt(i2)->setY(v);
					}
					else if(path[5]=="width") {
						bool ok;
						int v=value.toInt(&ok);
						assert(ok);
						map->layerAt(i)->asObjectGroup()->objectAt(i2)->setWidth(v);
					}
					else if(path[5]=="height") {
						bool ok;
						int v=value.toInt(&ok);
						assert(ok);
						map->layerAt(i)->asObjectGroup()->objectAt(i2)->setHeight(v);
					}
					else if(path[5]=="visible") {
						bool ok;
						int v=value.toInt(&ok);
						assert(ok);
						map->layerAt(i)->asObjectGroup()->objectAt(i2)->setVisible(v);
					}
					else if(path[5]=="gid") {
						Tiled::Cell c=cellFromGID(map, value);
						map->layerAt(i)->asObjectGroup()->objectAt(i2)->setCell(c);
					}
				}
				else return readUnknownElement(path, value);
			}
			else if(path[1]=="imagelayer") {
				//
				assert(path.size()==4);
				int i=path[2].toInt();
				while(i>=map->layerCount()) {
					map->addLayer(new Tiled::ImageLayer("",0,0,0,0));
				}
				if(!map->layerAt(i)->isImageLayer()) {
					map->takeLayerAt(i);
					map->insertLayer(i,new Tiled::ImageLayer("",0,0,0,0));
				}
				if(path.size()==4) { //imagelayer attribute
					bool l=processLayer(map, path, i, value);
					if(l) {}
					else if(path[3]=="source") {
						map->layerAt(i)->asImageLayer()->setSource(value);
					}
					else if(path[3]=="trans") {
						QColor v(value);
						assert(v.isValid());
						map->layerAt(i)->asImageLayer()->setTransparentColor(v);
					}
					else return readUnknownElement(path, value);
				}
				else return readUnknownElement(path, value);
			}
			else return readUnknownElement(path, value);
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
		out << "main.version = 0.0.2\n";
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
		if(map->tilesetCount()>0) { //tilesets
			for(int i=0;i<map->tilesetCount();++i) {
				out << "map.tileset." << i << ".name" << " = " << map->tilesetAt(i)->name() << endl;
				out << "map.tileset." << i << ".tilewidth" << " = " << map->tilesetAt(i)->tileWidth() << endl;
				out << "map.tileset." << i << ".tileheight" << " = " << map->tilesetAt(i)->tileHeight() << endl;
				out << "map.tileset." << i << ".spacing" << " = " << map->tilesetAt(i)->tileSpacing() << endl;
				out << "map.tileset." << i << ".margin" << " = " << map->tilesetAt(i)->margin() << endl;
				out << "map.tileset." << i << ".source" << " = " << map->tilesetAt(i)->fileName() << endl;
				out << "map.tileset." << i << ".image.trans" << " = " << map->tilesetAt(i)->transparentColor().name() << endl;
				out << "map.tileset." << i << ".image.width" << " = " << map->tilesetAt(i)->imageWidth() << endl;
				out << "map.tileset." << i << ".image.height" << " = " << map->tilesetAt(i)->imageHeight() << endl;
				out << "map.tileset." << i << ".image.source" << " = " << map->tilesetAt(i)->imageSource() << endl;
				out << "map.tileset." << i << ".tileoffset.x" << " = " << map->tilesetAt(i)->tileOffset().x() << endl;
				out << "map.tileset." << i << ".tileoffset.y" << " = " << map->tilesetAt(i)->tileOffset().y() << endl;
				if(!map->tilesetAt(i)->properties().empty()) { //tileset properties
					Tiled::Properties::const_iterator end=map->tilesetAt(i)->properties().constEnd();
					for(Tiled::Properties::const_iterator it=map->tilesetAt(i)->properties().constBegin();it!=end;++it) {
						out << "map.tileset." << i << ".properties." << it.key() << " = " << it.value() << endl;
					}
				}
				if(map->tilesetAt(i)->terrainCount()>0) { //tileset terrain
					for(int i2=0;i2<map->tilesetAt(i)->terrainCount();++i2) {
						out << "map.tileset." << i << ".terrain." << i2 << ".name" << " = " << map->tilesetAt(i)->terrain(i2)->name() << endl;
						out << "map.tileset." << i << ".terrain." << i2 << ".tid" << " = " << map->tilesetAt(i)->terrain(i2)->imageTileId() << endl;
						if(!map->tilesetAt(i)->terrain(i2)->properties().empty()) { //terrain properties
							Tiled::Properties::const_iterator end=map->tilesetAt(i)->terrain(i2)->properties().constEnd();
							for(Tiled::Properties::const_iterator it=map->tilesetAt(i)->terrain(i2)->properties().constBegin();it!=end;++it) {
								out << "map.tileset." << i << ".terrain." << i2 << ".properties." << it.key() << " = " << it.value() << endl;
							}
						}
					}
				}
				if(map->tilesetAt(i)->tileCount()>0) { //tileset tile
					for(int i2=0;i2<map->tilesetAt(i)->tileCount();++i2) {
						Tiled::Tile* t=map->tilesetAt(i)->tileAt(i2);
						if(t->terrain()!=-1) { //tile terrain
							out << "map.tileset." << i << ".tile." << i2 << ".terrain" << " = " << t->terrain() << endl;
							out << "map.tileset." << i << ".tile." << i2 << ".probability" << " = " << t->terrainProbability() << endl;
						}
						if(!map->tilesetAt(i)->tileAt(i2)->properties().empty()) { //tile properties
							Tiled::Properties::const_iterator end=map->tilesetAt(i)->tileAt(i2)->properties().constEnd();
							for(Tiled::Properties::const_iterator it=map->tilesetAt(i)->tileAt(i2)->properties().constBegin();it!=end;++it) {
								out << "map.tileset." << i << ".tile." << i2 << ".properties." << it.key() << " = " << it.value() << endl;
							}
						}
					}
				}
			}
		}
		if(map->layerCount()>0) { //layers
			for(int i=0;i<map->layerCount();++i) {
				QString type;
				if(map->layerAt(i)->layerType()==Tiled::Layer::TileLayerType) type="tile";
				else if(map->layerAt(i)->layerType()==Tiled::Layer::ImageLayerType) type="image";
				else if(map->layerAt(i)->layerType()==Tiled::Layer::ObjectGroupType) type="object";
				else return false;
				out << "map."<<type<<"layer." << i << ".name" << " = " << map->layerAt(i)->name() << endl;
				out << "map."<<type<<"layer." << i << ".opacity" << " = " << map->layerAt(i)->opacity() << endl;
				out << "map."<<type<<"layer." << i << ".visible" << " = " << (map->layerAt(i)->isVisible()?1:0) << endl;
				out << "map."<<type<<"layer." << i << ".x" << " = " << map->layerAt(i)->x() << endl;
				out << "map."<<type<<"layer." << i << ".y" << " = " << map->layerAt(i)->y() << endl;
				out << "map."<<type<<"layer." << i << ".width" << " = " << map->layerAt(i)->width() << endl;
				out << "map."<<type<<"layer." << i << ".height" << " = " << map->layerAt(i)->height() << endl;
				if(map->layerAt(i)->layerType()==Tiled::Layer::TileLayerType) {
					//
					for(int y=0;y<map->layerAt(i)->height();++y) {
						for(int x=0;x<map->layerAt(i)->width();++x) {
							const Tiled::Cell& c=map->layerAt(i)->asTileLayer()->cellAt(QPoint(x, y));
							if(c.isEmpty()) {
								out << "map."<<type<<"layer." << i << ".tile." << y << "." << x << " = " << "0" << endl;
							}
							else {
								out << "map."<<type<<"layer." << i << ".tile." << y << "." << x << " = " <<
											 map->indexOfTileset(c.tile->tileset()) << "," << c.tile->id() << "," <<
											 (c.flippedHorizontally<<2|c.flippedVertically<<1|c.flippedAntiDiagonally)
										<< endl;
							}
						}
					}
				}
				else if(map->layerAt(i)->layerType()==Tiled::Layer::ImageLayerType) {
					//
					out << "map."<<type<<"layer." << i << ".source" << " = " << map->layerAt(i)->asImageLayer()->imageSource() << endl;
					out << "map."<<type<<"layer." << i << ".trans" << " = " << map->layerAt(i)->asImageLayer()->transparentColor().name() << endl;
				}
				else if(map->layerAt(i)->layerType()==Tiled::Layer::ObjectGroupType) {
					//
					if(map->layerAt(i)->asObjectGroup()->color().isValid()) {
						out << "map."<<type<<"layer." << i << ".width" << " = " << map->layerAt(i)->asObjectGroup()->color().name() << endl;
					}
					for(int j=0;j<map->layerAt(i)->asObjectGroup()->objectCount();++j) {
						out << "map."<<type<<"layer." << i << ".object." << j << ".name" << " = " << map->layerAt(i)->asObjectGroup()->objectAt(j)->name() << endl;
						out << "map."<<type<<"layer." << i << ".object." << j << ".x" << " = " << map->layerAt(i)->asObjectGroup()->objectAt(j)->x() << endl;
						out << "map."<<type<<"layer." << i << ".object." << j << ".y" << " = " << map->layerAt(i)->asObjectGroup()->objectAt(j)->y() << endl;
						out << "map."<<type<<"layer." << i << ".object." << j << ".width" << " = " << map->layerAt(i)->asObjectGroup()->objectAt(j)->width() << endl;
						out << "map."<<type<<"layer." << i << ".object." << j << ".height" << " = " << map->layerAt(i)->asObjectGroup()->objectAt(j)->height() << endl;
						const Tiled::Cell& c=map->layerAt(i)->asObjectGroup()->objectAt(j)->cell();
						if(!c.isEmpty()) {
							out << "map."<<type<<"layer." << i << ".object." << j << ".gid" << " = " <<
										 map->indexOfTileset(c.tile->tileset()) << "," <<
										 c.tile->id() << "," <<
										 (c.flippedHorizontally<<2|c.flippedVertically<<1|c.flippedAntiDiagonally) << endl;
						}
					}
				}
				else return false;
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
