/*
 * CSV Tiled Plugin
 * Copyright 2014, Thorbjørn Lindeijer <thorbjorn@lindeijer.nl>
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

#include "csvplugin.h"

#include "map.h"
#include "tile.h"
#include "tilelayer.h"

#include <QFile>

#if QT_VERSION >= 0x050100
#define HAS_QSAVEFILE_SUPPORT
#endif

#ifdef HAS_QSAVEFILE_SUPPORT
#include <QSaveFile>
#endif

using namespace Tiled;

CsvPlugin::CsvPlugin()
{
}

bool CsvPlugin::write(const Map *map, const QString &fileName)
{
#ifdef HAS_QSAVEFILE_SUPPORT
    QSaveFile file(fileName);
#else
    QFile file(fileName);
#endif
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        mError = tr("Could not open file for writing.");
        return false;
    }

    const TileLayer *tileLayer = 0;

    // Take the first tile layer
    foreach (const Layer *layer, map->layers()) {
        if (layer->layerType() == Layer::TileLayerType) {
            tileLayer = static_cast<const TileLayer*>(layer);
            break;
        }
    }

    if (!tileLayer) {
        mError = tr("No tile layer found.");
        return false;
    }

    // Write out tiles either by ID or their name, if given. -1 is "empty"
    for (int y = 0; y < tileLayer->height(); ++y) {
        for (int x = 0; x < tileLayer->width(); ++x) {
            if (x > 0)
                file.write(",", 1);

            const Cell &cell = tileLayer->cellAt(x, y);
            const Tile *tile = cell.tile;
            if (tile && tile->hasProperty(QLatin1String("name"))) {
                file.write(tile->property(QLatin1String("name")).toUtf8());
            } else {
                const int id = tile ? tile->id() : -1;
                file.write(QByteArray::number(id));
            }
        }

        file.write("\n", 1);
    }

    if (file.error() != QFile::NoError) {
        mError = file.errorString();
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

QString CsvPlugin::nameFilter() const
{
    return tr("CSV files (*.csv)");
}

QString CsvPlugin::errorString() const
{
    return mError;
}
