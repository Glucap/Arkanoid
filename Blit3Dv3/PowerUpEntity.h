#pragma once

#include "Entity.h"

enum class PowerUpType { MULTIBALL, EXTRALIFE, BIGPADDLE, SLOWBALL };

class PowerUpEntity : public Entity
{
public:
	PowerUpType powerUpType;
	PowerUpEntity();
};

PowerUpEntity * MakePowerUp(int type, float xpos, float ypos);