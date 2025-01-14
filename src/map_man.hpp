/**
 * Copyright (c) 2022 Alexander Lunsford
 *
 * This software is provided 'as-is', without any express or implied
 * warranty.  In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 **/

#ifndef MAP_MAN_H
#define MAP_MAN_H

#include <deque>
#include <cstdint>
#include <memory>
#include <filesystem>
namespace fs = std::filesystem;

#include "tile.hpp"
#include "ent.hpp"

//It's either a class that manages map data modification, saving/loading, and undo/redo operations, or a lame new Megaman boss.
class MapMan
{
public:
    class Action 
    {
    public:
        virtual void Do(MapMan &map) const = 0;
        virtual void Undo(MapMan &map) const = 0;
    };

    class TileAction : public Action
    {
    public:
        inline TileAction(size_t i, size_t j, size_t k, TileGrid prevState, TileGrid newState)
            : _i(i), _j(j), _k(k), _prevState(prevState), _newState(newState)
        {
        }
        
        inline virtual void Do(MapMan &map) const override
        {
            map._tileGrid.CopyTiles(_i, _j, _k, _newState);
        }

        inline virtual void Undo(MapMan &map) const override
        {
            map._tileGrid.CopyTiles(_i, _j, _k, _prevState);
        }

        size_t _i, _j, _k;
        TileGrid _prevState;
        TileGrid _newState;
    };

    class EntAction : public Action
    {
    public:
        inline EntAction(size_t i, size_t j, size_t k, bool overwrite, bool removed, Ent oldEnt, Ent newEnt)
            : _i(i), _j(j), _k(k), _overwrite(overwrite), _removed(removed), _oldEnt(oldEnt), _newEnt(newEnt)
        {
        }

        inline virtual void Do(MapMan &map) const override
        {
            if (_removed)
            {
                map._entGrid.RemoveEnt(_i, _j, _k);
            }
            else
            {
                map._entGrid.AddEnt(_i, _j, _k, _newEnt);
            }
        }

        inline virtual void Undo(MapMan &map) const override
        {
            if (_overwrite || _removed)
            {
                map._entGrid.AddEnt(_i, _j, _k, _oldEnt);
            }
            else
            {
                map._entGrid.RemoveEnt(_i, _j, _k);
            }
        }
    protected:
        size_t _i, _j, _k;
        bool _overwrite; //Indicates if there was an entity underneath the one placed that must be restored when undoing.
        bool _removed; //Indicates if the new cel value is empty.
        Ent  _oldEnt;
        Ent  _newEnt;
    };

    
    inline void NewMap(int width, int height, int length) 
    {
        _tileGrid = TileGrid(width, height, length);
        _entGrid = EntGrid(width, height, length);
        _undoHistory.clear();
        _redoHistory.clear();
    }

    inline const TileGrid& Tiles() const { return _tileGrid; }
    inline const EntGrid& Ents() const { return _entGrid; }

    inline void DrawMap(Camera &camera, int fromY, int toY) 
    {
        _tileGrid.Draw(Vector3Zero(), fromY, toY);
        _entGrid.Draw(camera, fromY, toY);
    }

    inline void Draw2DElements(Camera &camera, int fromY, int toY)
    {
        _entGrid.DrawLabels(camera, fromY, toY);
    }

    //Regenerates the map, extending one of the grid's dimensions on the given axis. Returns false if the change would result in an invalid map size.
    inline void ExpandMap(Direction axis, int amount)
    {
        int newWidth  = _tileGrid.GetWidth();
        int newHeight = _tileGrid.GetHeight();
        int newLength = _tileGrid.GetLength();
        int ofsx, ofsy, ofsz;
        ofsx = ofsy = ofsz = 0;

        switch (axis)
        {
            case Direction::Z_NEG: newLength += amount; ofsz += amount; break;
            case Direction::Z_POS: newLength += amount; break;
            case Direction::X_NEG: newWidth += amount; ofsx += amount; break;
            case Direction::X_POS: newWidth += amount; break;
            case Direction::Y_NEG: newHeight += amount; ofsy += amount; break;
            case Direction::Y_POS: newHeight += amount; break;
        }

        _undoHistory.clear();
        _redoHistory.clear();
        TileGrid oldTiles = _tileGrid;
        EntGrid oldEnts = _entGrid;
        _tileGrid = TileGrid(newWidth, newHeight, newLength);
        _entGrid = EntGrid(newWidth, newHeight, newLength);       
        _tileGrid.CopyTiles(ofsx, ofsy, ofsz, oldTiles, false);
        _entGrid.CopyEnts(ofsx, ofsy, ofsz, oldEnts);
    }

    //Reduces the size of the grid until it fits perfectly around all the non-empty cels in the map.
    inline void ShrinkMap()
    {
        size_t minX, minY, minZ;
        size_t maxX, maxY, maxZ;
        minX = minY = minZ = UINT64_MAX;
        maxX = maxY = maxZ = 0;
        for (size_t x = 0; x < _tileGrid.GetWidth(); ++x)
        {
            for (size_t y = 0; y < _tileGrid.GetHeight(); ++y)
            {
                for (size_t z = 0; z < _tileGrid.GetLength(); ++z)
                {
                    if (_tileGrid.GetTile(x, y, z) || _entGrid.HasEnt(x, y, z))
                    {
                        if (x < minX) minX = x;
                        if (y < minY) minY = y;
                        if (z < minZ) minZ = z;
                        if (x > maxX) maxX = x;
                        if (y > maxY) maxY = y;
                        if (z > maxZ) maxZ = z;
                    }
                }
            }
        }
        if (minX > maxX || minY > maxY || minZ > maxZ)
        {
            //If there aren't any tiles, just make it 1x1x1.
            _tileGrid = TileGrid(1, 1, 1);
            _entGrid = EntGrid(1, 1, 1);
        }
        else
        {
            _tileGrid = _tileGrid.Subsection(minX, minY, minZ, maxX - minX + 1, maxY - minY + 1, maxZ - minZ + 1);
            _entGrid = _entGrid.Subsection(minX, minY, minZ, maxX - minX + 1, maxY - minY + 1, maxZ - minZ + 1);
        }
    }

    //Saves the map as a .te3 file at the given path. Returns false if there was an error.
    bool SaveTE3Map(fs::path filePath);

    //Loads a .te3 map from the given path. Returns false if there was an error.
    bool LoadTE3Map(fs::path filePath);

    //Exports the map as a .gltf file, returning false on error.
    //If separateGeometry is true, then the geometry will be put into separate
    //GLTF nodes according to their tile shape and texture.
    bool ExportGLTFScene(fs::path filePath, bool separateGeometry);

    //Executes a undoable tile action for filling an area with one tile
    void ExecuteTileAction(size_t i, size_t j, size_t k, size_t w, size_t h, size_t l, Tile newTile);
    //Executes a undoable tile action for filling an area using a brush
    void ExecuteTileAction(size_t i, size_t j, size_t k, size_t w, size_t h, size_t l, TileGrid brush);
    //Executes an undoable entity action for placing an entity
    void ExecuteEntPlacement(int i, int j, int k, Ent newEnt);
    //Executes an undoable entity action for removing an entity.
    void ExecuteEntRemoval(int i, int j, int k);

    inline void Undo()
    {
        if (!_undoHistory.empty())
        {
            _undoHistory.back()->Undo(*this);
            _redoHistory.push_back(_undoHistory.back());
            _undoHistory.pop_back();
        }
    }

    inline void Redo()
    {
        if (!_redoHistory.empty())
        {
            _redoHistory.back()->Do(*this);
            _undoHistory.push_back(_redoHistory.back());
            _redoHistory.pop_back();
        }
    }
private:
    void _Execute(std::shared_ptr<Action> action);

    TileGrid _tileGrid;
    EntGrid _entGrid;

    //Stores recently executed actions to be undone on command.
    std::deque<std::shared_ptr<Action>> _undoHistory;
    //Stores recently undone actions to be redone on command, unless the history is altered.
    std::deque<std::shared_ptr<Action>> _redoHistory;
};

#endif
