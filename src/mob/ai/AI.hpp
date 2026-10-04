#pragma once

class Mob;
class Entity;
class Level;

class AI {
public:
    int defaultLookAngle = 0;
    virtual ~AI() = default; 

    virtual void tick(Level* level, Mob* mob) = 0;
    virtual void beforeRemove() = 0;
    virtual void hurt(Entity* e, int dmg) = 0;

};