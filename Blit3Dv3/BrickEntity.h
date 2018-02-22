#pragma once

#include "Entity.h"

enum class BrickEnum { BLUE = 0, GREEN, GREY, PURPLE, RED, YELLOW };

//externed sprites
extern Sprite *blueBrickSprite;
extern Sprite *greenBrickSprite;
extern Sprite *greyBrickSprite;
extern Sprite *purpleBrickSprite;
extern Sprite *redBrickSprite;
extern Sprite *yellowBrickSprite;

class BrickEntity : public Entity
{
public:
	bool power = false;
	BrickEnum colour;
	BrickEntity()
	{
		typeID = ENTITYBRICK;
		colour = BrickEnum::RED;
	}

	bool HandleCollision();
};

BrickEntity * MakeBrick(BrickEnum type, float xpos, float ypos);