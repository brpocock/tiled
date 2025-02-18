/*
 * editablemanager.cpp
 * Copyright 2019, Thorbjørn Lindeijer <bjorn@lindeijer.nl>
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

#include "editablemanager.h"

#include "documentmanager.h"
#include "editablegrouplayer.h"
#include "editableimagelayer.h"
#include "editablemap.h"
#include "editableobjectgroup.h"
#include "editabletile.h"
#include "editabletilelayer.h"
#include "editabletileset.h"
#include "editablewangset.h"
#include "scriptmanager.h"
#include "wangset.h"

#include <QQmlEngine>

namespace Tiled {

static bool becomesNullValue(QObject *object)
{
    return ScriptManager::instance().engine()->newQObject(object).isNull();
}

std::unique_ptr<EditableManager> EditableManager::mInstance;

EditableManager::EditableManager(QObject *parent)
    : QObject(parent)
{
}

EditableManager &EditableManager::instance()
{
    if (!mInstance)
        mInstance.reset(new EditableManager);
    return *mInstance;
}

void EditableManager::deleteInstance()
{
    mInstance.reset();
}

void EditableManager::release(Layer *layer)
{
    if (EditableLayer *editable = find(layer))
        editable->hold();
    else
        delete layer;
}

void EditableManager::release(MapObject *mapObject)
{
    if (EditableMapObject *editable = find(mapObject))
        editable->hold();
    else
        delete mapObject;
}

/**
 * Releases the WangSet by either finding an EditableWangSet instance to take
 * ownership of it or deleting it.
 */
void EditableManager::release(std::unique_ptr<WangSet> wangSet)
{
    if (EditableWangSet *editable = find(wangSet.get())) {
        editable->hold();
        wangSet.release();
    }
}

EditableLayer *EditableManager::editableLayer(EditableMap *map, Layer *layer)
{
    if (!layer)
        return nullptr;

    Q_ASSERT(!map || layer->map() == map->map());

    EditableLayer* &editableLayer = mEditableLayers[layer];
    if (becomesNullValue(editableLayer)) {
        switch (layer->layerType()) {
        case Layer::TileLayerType:
            editableLayer = new EditableTileLayer(map, static_cast<TileLayer*>(layer));
            break;
        case Layer::ObjectGroupType:
            editableLayer = new EditableObjectGroup(map, static_cast<ObjectGroup*>(layer));
            break;
        case Layer::ImageLayerType:
            editableLayer = new EditableImageLayer(map, static_cast<ImageLayer*>(layer));
            break;
        case Layer::GroupLayerType:
            editableLayer = new EditableGroupLayer(map, static_cast<GroupLayer*>(layer));
            break;
        }
        QQmlEngine::setObjectOwnership(editableLayer, QQmlEngine::JavaScriptOwnership);
    }

    return editableLayer;
}

EditableObjectGroup *EditableManager::editableObjectGroup(EditableAsset *asset, ObjectGroup *objectGroup)
{
    if (!objectGroup)
        return nullptr;

    EditableLayer* &editableLayer = mEditableLayers[objectGroup];
    if (becomesNullValue(editableLayer)) {
        editableLayer = new EditableObjectGroup(asset, objectGroup);
        QQmlEngine::setObjectOwnership(editableLayer, QQmlEngine::JavaScriptOwnership);
    }

    return static_cast<EditableObjectGroup*>(editableLayer);
}

EditableMapObject *EditableManager::editableMapObject(EditableAsset *asset, MapObject *mapObject)
{
    if (!mapObject)
        return nullptr;

    Q_ASSERT(mapObject->objectGroup());

    EditableMapObject* &editableMapObject = mEditableMapObjects[mapObject];
    if (becomesNullValue(editableMapObject)) {
        editableMapObject = new EditableMapObject(asset, mapObject);
        QQmlEngine::setObjectOwnership(editableMapObject, QQmlEngine::JavaScriptOwnership);
    }

    return editableMapObject;
}

EditableTileset *EditableManager::editableTileset(Tileset *tileset)
{
    if (!tileset)
        return nullptr;

    if (auto document = TilesetDocument::findDocumentForTileset(tileset->sharedFromThis()))
        return document->editable();

    EditableTileset* &editableTileset = mEditableTilesets[tileset];
    if (becomesNullValue(editableTileset)) {
        editableTileset = new EditableTileset(tileset);
        QQmlEngine::setObjectOwnership(editableTileset, QQmlEngine::JavaScriptOwnership);
    }

    return editableTileset;
}

EditableTile *EditableManager::editableTile(Tile *tile)
{
    if (!tile)
        return nullptr;

    EditableTileset *tileset = editableTileset(tile->tileset());
    return editableTile(tileset, tile);
}

EditableTile *EditableManager::editableTile(EditableTileset *tileset, Tile *tile)
{
    Q_ASSERT(tile);
    Q_ASSERT(tile->tileset() == tileset->tileset());

    EditableTile* &editableTile = mEditableTiles[tile];
    if (becomesNullValue(editableTile)) {
        editableTile = new EditableTile(tileset, tile);
        QQmlEngine::setObjectOwnership(editableTile, QQmlEngine::JavaScriptOwnership);
    }

    return editableTile;
}

EditableWangSet *EditableManager::editableWangSet(WangSet *wangSet)
{
    if (!wangSet)
        return nullptr;

    EditableTileset *tileset = editableTileset(wangSet->tileset());
    return editableWangSet(tileset, wangSet);
}

EditableWangSet *EditableManager::editableWangSet(EditableTileset *tileset, WangSet *wangSet)
{
    Q_ASSERT(wangSet);
    Q_ASSERT(wangSet->tileset() == tileset->tileset());

    EditableWangSet* &editableWangSet = mEditableWangSets[wangSet];
    if (becomesNullValue(editableWangSet)) {
        editableWangSet = new EditableWangSet(tileset, wangSet);
        QQmlEngine::setObjectOwnership(editableWangSet, QQmlEngine::JavaScriptOwnership);
    }

    return editableWangSet;
}

} // namespace Tiled

#include "moc_editablemanager.cpp"
