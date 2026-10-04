#include "Data.hpp"

std::vector<const Tile*> Data::allowedTiles;
bool Data::survival = true;
bool Data::showHitboxes = false;

void Data::initAllowedTiles() {
    Data::allowedTiles.clear();

    Data::allowedTiles.push_back(Tile::cobblestone);
    Data::allowedTiles.push_back(Tile::rock);
    Data::allowedTiles.push_back(Tile::dirt);
    Data::allowedTiles.push_back(Tile::wood);
    Data::allowedTiles.push_back(Tile::log);
    Data::allowedTiles.push_back(Tile::leaves);
    Data::allowedTiles.push_back(Tile::glass);
    Data::allowedTiles.push_back(Tile::sand);
    Data::allowedTiles.push_back(Tile::gravel);
    Data::allowedTiles.push_back(Tile::redFlower);
    Data::allowedTiles.push_back(Tile::yellowFlower);
    Data::allowedTiles.push_back(Tile::redMushroom);
    Data::allowedTiles.push_back(Tile::brownMushroom);
    Data::allowedTiles.push_back(Tile::sponge);
    Data::allowedTiles.push_back(Tile::bush);

    Data::allowedTiles.push_back(Tile::wool1);
    Data::allowedTiles.push_back(Tile::wool2);
    Data::allowedTiles.push_back(Tile::wool3);
    Data::allowedTiles.push_back(Tile::wool4);
    Data::allowedTiles.push_back(Tile::wool5);
    Data::allowedTiles.push_back(Tile::wool6);
    Data::allowedTiles.push_back(Tile::wool7);
    Data::allowedTiles.push_back(Tile::wool8);
    Data::allowedTiles.push_back(Tile::wool9);
    Data::allowedTiles.push_back(Tile::wool10);
    Data::allowedTiles.push_back(Tile::wool11);
    Data::allowedTiles.push_back(Tile::wool12);
    Data::allowedTiles.push_back(Tile::wool13);
    Data::allowedTiles.push_back(Tile::wool14);
    Data::allowedTiles.push_back(Tile::wool15);
    Data::allowedTiles.push_back(Tile::wool16);

    Data::allowedTiles.push_back(Tile::goldBlock);
    Data::allowedTiles.push_back(Tile::ironBlock);
    Data::allowedTiles.push_back(Tile::mossStone);
    Data::allowedTiles.push_back(Tile::slab);
    Data::allowedTiles.push_back(Tile::bookshelf);
    Data::allowedTiles.push_back(Tile::tnt);
    Data::allowedTiles.push_back(Tile::obsidian);

}