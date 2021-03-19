#ifndef MAP_H
#define MAP_H

#include <vector>
#include <iostream>
#include <map>

#define CHUNKSIZE 32

class Tile
{
public:
	enum Type { Grass, Stone };

	Tile()
	{
		_type = Type::Grass;
	}

	Tile(Type type)
	{
		_type = type;
	}

	void Draw() const;

	bool GetAttr(int attr) const
	{
		return (_attributes >> attr) & 0x1;
	}

	void SetAttr(int attr, bool value)
	{
		if (value) {
			_attributes |= (1 << attr);
		} else {
			_attributes &= ~(1 << attr);
		}
	}

	Type GetType()
	{
		return _type;
	}

private:
	Type _type;
	uint32_t _attributes;
};

class Layer
{
public:
	Layer():
		_tiles(CHUNKSIZE * CHUNKSIZE)
	{ }

	void SetTile(size_t x, size_t y, const Tile& tile)
	{
		_tiles[y * CHUNKSIZE + x] = tile;
	}
	
	void PrintLayer()
	{
		for (int i = 0; i < CHUNKSIZE; i++) {
			for (int j = 0; j < CHUNKSIZE; j++) {
				if (_tiles[j * CHUNKSIZE + i].GetType() ==
						Tile::Grass) {
					std::cout << "/ ";
				} else {
					std::cout << "0 ";
				}
			}
			
			std::cout << std::endl;
		}
	}

private:
	std::vector<Tile> _tiles;
};

class Chunk
{
public:
	Chunk():
		_layers(0)
	{ }

	~Chunk()
	{
		for (Layer* layer : _layers) {
			delete layer;
        }

	}

	void AddLayer(Layer* layer)
	{
		_layers.push_back(layer);
	}
	
	void PrintChunk()
	{
		for (Layer* layer : _layers) {
			layer->PrintLayer();
		}
	}

private:
	std::vector<Layer*> _layers;
};


class Map
{
public:
	~Map()
	{
		for (std::pair<std::pair<int32_t, int32_t>, Chunk*> chunk : _chunks) {
			delete chunk.second;
		}
	}

	void AddChunk(int32_t x, int32_t y, Chunk* chunk)
	{
		_chunks[std::pair<int32_t, int32_t>(x, y)] = chunk;
	}

private:
	std::map<std::pair<int32_t, int32_t>, Chunk*> _chunks;
};

#endif
