#include "PowerUpEntity.h"
#include "CollisionMask.h"

extern Sprite *multiBallSprite;
extern Sprite *extraLifeSprite;
extern Sprite *bigPaddleSprite;
extern Sprite *slowBallSprite;

extern b2World *world;

PowerUpEntity::PowerUpEntity()
{
	typeID = ENTITYPOWERUP;
}


PowerUpEntity * MakePowerUp(int type, float xpos, float ypos)
{
	PowerUpEntity *powerUpEntity = new PowerUpEntity();


	//set the sprites to draw with
	switch (type)
	{	
	case (int)PowerUpType::BIGPADDLE:
	case (int)PowerUpType::MULTIBALL:
		powerUpEntity->powerUpType = PowerUpType::MULTIBALL;
		powerUpEntity->sprite = bigPaddleSprite;

		break;

	case (int)PowerUpType::EXTRALIFE:
		powerUpEntity->powerUpType = PowerUpType::EXTRALIFE;
		powerUpEntity->sprite = extraLifeSprite;

		break;




	case (int)PowerUpType::SLOWBALL:
		powerUpEntity->powerUpType = PowerUpType::SLOWBALL;
		powerUpEntity->sprite = slowBallSprite;

		break;
	default:
		powerUpEntity->powerUpType = PowerUpType::EXTRALIFE;
		powerUpEntity->sprite = extraLifeSprite;
		break;
	}


	//make the physics body
	b2BodyDef powerUpBodyDef;

	//set the position of the center of the body, 
	//converting from pxel coords to physics measurements
	powerUpBodyDef.position.Set(xpos / PTM_RATIO, ypos / PTM_RATIO);
	powerUpBodyDef.type = b2_dynamicBody; //make it a dynamic body, so it can collide with the kinematic paddle

	powerUpEntity->body = world->CreateBody(&powerUpBodyDef); //create the body and add it to the world

	// Define a box shape for our dynamic body.
	b2CircleShape PowShape;
	//SetAsBox() takes as arguments the half-width and half-height of the box
	PowShape.m_radius = 22.f / (2 * PTM_RATIO);;

	b2FixtureDef powerUpFixtureDef;
	powerUpFixtureDef.shape = &PowShape;
	powerUpFixtureDef.density = 1.0f; 
	powerUpFixtureDef.restitution = 0;
	powerUpFixtureDef.friction = 0.1f;

	//collison masking
	powerUpFixtureDef.filter.categoryBits = CMASK_POWERUP;  //this is a power up
	powerUpFixtureDef.filter.maskBits = CMASK_PADDLE | CMASK_GROUND;		//it collides wth paddle and ground

	powerUpEntity->body->CreateFixture(&powerUpFixtureDef);

	//make the userdata point back to this entity
	powerUpEntity->body->SetUserData(powerUpEntity);

	//make the powerup drop
	
	powerUpEntity->body->SetLinearVelocity(b2Vec2(0.f, -30.f)); //apply velocity to kinematic object

	return powerUpEntity;
}
