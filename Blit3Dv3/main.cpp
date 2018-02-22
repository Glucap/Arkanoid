/*
Example program that demonstrates collision handling
*/
#include "Blit3D.h"

#include <random>
#include "LevelLoader.h"
#include "Physics.h"
#include "Entity.h"
#include "PaddleEntity.h"
#include "BallEntity.h"
#include "BrickEntity.h"
#include "GroundEntity.h"
#include "PowerUpEntity.h"
#include "EdgeEntity.h"

#include "MyContactListener.h" //for handling collisions

#include "CollisionMask.h"

#include "Particle.h" //particles, yay!

Blit3D *blit3D = NULL;

//this code sets up memory leak detection
#ifdef _DEBUG
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#include <stdlib.h>
#define DEBUG_NEW new(_CLIENT_BLOCK, __FILE__, __LINE__)
#define new DEBUG_NEW
#endif


//GLOBAL DATA
std::mt19937 rng;
std::uniform_real_distribution<float> plusMinus5Degrees(-5, +5);
std::uniform_real_distribution<float> plusMinus70Degrees(-70, +70);

b2Vec2 gravity; //defines our gravity vector
b2World *world; //our physics engine

				// Prepare for simulation. Typically we use a time step of 1/60 of a
				// second (60Hz) and ~10 iterations. This provides a high quality simulation
				// in most game scenarios.
int32 velocityIterations = 8;
int32 positionIterations = 3;
float timeStep = 1.f / 60.f; //one 60th of a second
float elapsedTime = 0; //used for calculating time passed

//contact listener to handle collisions between important objects
MyContactListener *contactListener;


double slowBallTimer;
bool slowBall;

float cursorX = 0;
PaddleEntity *paddleEntity = NULL;

enum GameState { START, PLAYING, GAMEOVER, EGG };
GameState gameState = START;
bool attachedBall = true; //is the ball ready to be launched from the paddle?
int lives = 3;
int score = 0;
int multiplyer = 0;
std::vector<BrickEntity *> brickEntityList; //bricks go here
std::vector<Entity *> ballEntityList; //track the balls seperately from everything else
std::vector<Entity *> entityList; //other entities in our game go here
std::vector<Entity *> deadEntityList; //dead entities

float currentBallSpeed; //defaultball speed
float DefaultBallSpeed;
float slowBallSpeed;
int level = 1; //current level of play

//Sprites 
Sprite *logo = NULL;
Sprite *gameOverLogo = NULL;
Sprite *ballSprite = NULL;
Sprite *redBrickSprite = NULL;
Sprite *yellowBrickSprite = NULL;
Sprite *blueBrickSprite = NULL;
Sprite *greenBrickSprite = NULL;
Sprite *greyBrickSprite = NULL;
Sprite *paddleSprite = NULL;
Sprite *purpleBrickSprite = NULL;

Sprite *backgroundSprite = NULL;

Sprite *multiBallSprite = NULL;
Sprite *extraLifeSprite = NULL;
Sprite *bigPaddleSprite = NULL;
Sprite *slowBallSprite = NULL;

//particle stuff
std::vector<Particle *> particleList;

Sprite *cloudSprite1 = NULL;
Sprite *cloudSprite2 = NULL;
Sprite *cloudSprite3 = NULL;

Sprite *explosionSprite1 = NULL;
Sprite *explosionSprite2 = NULL;
Sprite *explosionSprite3 = NULL;
Sprite *explosionSprite4 = NULL;
Sprite *explosionSprite5 = NULL;
Sprite *explosionSprite6 = NULL;
Sprite *explosionSprite7 = NULL;

Sprite *boinkSprite1 = NULL;
Sprite *boinkSprite2 = NULL;
Sprite *boinkSprite3 = NULL;

//font
AngelcodeFont *debugFont = NULL;

//______MAKE SOME BRICKS_______________________
void MakeLevel()
{
	//delete all particles
	for (auto p : particleList) delete p;
	particleList.clear();

	//delete any extra balls and attach ball
	for (int i = ballEntityList.size() - 1; i > 0; --i)
	{
		world->DestroyBody(ballEntityList[i]->body);
		delete ballEntityList[i];
		ballEntityList.erase(ballEntityList.begin() + i);
	}
	attachedBall = true;
	slowBall = false;

	//delete all extra entities, like powerups
	for (int i = entityList.size() - 1; i >= 0; --i)
	{
		if (entityList[i]->typeID != ENTITYEDGE 
			&& entityList[i]->typeID != ENTITYGROUND 
			&& entityList[i]->typeID != ENTITYPADDLE)
		{
			world->DestroyBody(entityList[i]->body);
			delete entityList[i];
			entityList.erase(entityList.begin() + i);
		}
	}

	
	DefaultBallSpeed = 20 + 10 * level; //balls move faster by default at higher levels
	slowBallSpeed = DefaultBallSpeed / 2;
	//remove any old bricks, just in case
	for (auto b : brickEntityList)
	{
		world->DestroyBody(b->body);
		delete b;
	}

	brickEntityList.clear();

	//make the new level
	 
	switch (level%3)
	{
	case 1:
		LoadMap("level1.txt", brickEntityList);
		break;

	case 2:
		LoadMap("level2.txt", brickEntityList);
		break;

	case 0:
		LoadMap("level3.txt", brickEntityList);
		break;

	default:
		LoadMap("rick.txt", brickEntityList);
		break;

	}

	//create a distribution that has less chance of rolling a zero  (false)
	//the higher the level gets
	std::uniform_int_distribution<int> brickRandom(0, level *5);

		for (auto b: brickEntityList)
		{
			if ((!brickRandom(rng)&& (b->colour == BrickEnum::BLUE || b->colour == BrickEnum::PURPLE))||b->colour == BrickEnum::YELLOW)
			{
				b->power= true;
			}
			
	
	}
}

//ensures that entities are only added ONCE to the deadEntityList
void AddToDeadList(Entity *e)
{
	bool unique = true;

	for (auto ent : deadEntityList)
	{
		if (ent == e)
		{
			unique = false;
			break;
		}
	}

	if (unique) deadEntityList.push_back(e);
}

void Init()
{
	//seed random generator
	std::random_device rd;
	rng.seed(rd());


	backgroundSprite = blit3D->MakeSprite(0, 0, 1920, 1080, "Media\\Logo.png");

	//turn off the cursor
	blit3D->ShowCursor(false);

	debugFont = blit3D->MakeAngelcodeFontFromBinary32("Media\\DebugFont_24.bin");

	//load the sprites
	redBrickSprite = blit3D->MakeSprite(0, 0, 64, 32, "Media\\element_red_rectangle_glossy.png");
	greyBrickSprite = blit3D->MakeSprite(0, 0, 64, 32, "Media\\element_grey_rectangle_glossy.png");
	greenBrickSprite = blit3D->MakeSprite(0, 0, 64, 32, "Media\\element_green_rectangle_glossy.png");
	blueBrickSprite = blit3D->MakeSprite(0, 0, 64, 32, "Media\\element_blue_rectangle_glossy.png");
	yellowBrickSprite = blit3D->MakeSprite(0, 0, 64, 32, "Media\\element_yellow_rectangle_glossy.png");
	purpleBrickSprite = blit3D->MakeSprite(0, 0, 64, 32, "Media\\element_purple_rectangle_glossy.png");

	//power up animated sprite lists
	multiBallSprite = blit3D->MakeSprite(0, 0, 22, 22, "Media\\POWMultiball.png");
	extraLifeSprite = blit3D->MakeSprite(0, 0, 22, 22, "Media\\POWLife.png");
	bigPaddleSprite = blit3D->MakeSprite(0, 0, 22, 22, "Media\\POWPaddle.png");
	slowBallSprite = blit3D->MakeSprite(0, 0, 22, 22, "Media\\POWslowball.png");


	logo = blit3D->MakeSprite(0, 0, 600, 309, "Media\\darrenoid.png");
	gameOverLogo = blit3D->MakeSprite(0, 0, 643, 480, "Media\\gameover.png");
	paddleSprite = blit3D->MakeSprite(0, 0, 119, 28, "Media\\paddle.png");
	
	ballSprite = blit3D->MakeSprite(0, 0, 24, 24, "Media\\ball.png");

	//particle sprites
	cloudSprite1 = blit3D->MakeSprite(0, 0, 41, 41, "media\\bang1.png");
	cloudSprite2 = blit3D->MakeSprite(0, 0, 41, 41, "media\\bang2.png");
	cloudSprite3 = blit3D->MakeSprite(0, 0, 41, 41, "media\\bang5.png");

	explosionSprite1 = blit3D->MakeSprite(0,0, 41, 41, "media\\yellowshot.png");
	explosionSprite2 = blit3D->MakeSprite(41, 0, 41, 41, "media\\yellowshot.png");
	explosionSprite3 = blit3D->MakeSprite(82,0,41,41,"media\\yellowshot.png");
	explosionSprite4 = blit3D->MakeSprite(123,0,41,41, "media\\yellowshot.png");
	explosionSprite5 = blit3D->MakeSprite(164,0,41,41, "media\\yellowshot.png");
	explosionSprite6 = blit3D->MakeSprite(205,0,41,41, "media\\yellowshot.png");
	explosionSprite7 = blit3D->MakeSprite(246,0,41,41, "media\\yellowshot.png");

	boinkSprite1 = blit3D->MakeSprite(0,0,60,59, "media\\boink1.png");
	boinkSprite2 = blit3D->MakeSprite(0,0,60,59, "media\\boink2.png");
	boinkSprite3 = blit3D->MakeSprite(0,0,60,57, "media\\boink3.png");

	//from here on, we are setting up the Box2D physics world model

	// Define the gravity vector.
	gravity.x = 0.f;
	gravity.y = 0.f;

	// Construct a world object, which will hold and simulate the rigid bodies.
	world = new b2World(gravity);
	//world->SetGravity(gravity); <-can call this to change gravity at any time
	world->SetAllowSleeping(true); //set true to allow the physics engine to 'sleep" objects that stop moving

								   //_________GROUND OBJECT_____________
								   //make an entity for the ground
	GroundEntity *groundEntity = new GroundEntity();
	//A bodyDef for the ground
	b2BodyDef groundBodyDef;
	// Define the ground body.
	groundBodyDef.position.Set(0, 0);

	// Call the body factory which allocates memory for the ground body
	// from a pool and creates the ground box shape (also from a pool).
	// The body is also added to the world.
	groundEntity->body = world->CreateBody(&groundBodyDef);

	//an EdgeShape object, for the ground
	b2EdgeShape groundBox;

	// Define the ground as 1 edge shape at the bottom of the screen.
	b2FixtureDef boxShapeDef;

	boxShapeDef.shape = &groundBox;

	//collison masking
	boxShapeDef.filter.categoryBits = CMASK_GROUND;  //this is the ground
	boxShapeDef.filter.maskBits = CMASK_BALL | CMASK_POWERUP;		//it collides wth balls and powerups

																	//bottom
	groundBox.Set(b2Vec2(534/PTM_RATIO, 0), b2Vec2((blit3D->screenWidth-534) / PTM_RATIO, 0));
	//Create the fixture
	groundEntity->body->CreateFixture(&boxShapeDef);
	//add the userdata
	groundEntity->body->SetUserData(groundEntity);
	//add to the entity list
	entityList.push_back(groundEntity);

	//now make the other 3 edges of the screen on a seperate entity/body
	EdgeEntity * edgeEntity = new EdgeEntity();
	edgeEntity->body = world->CreateBody(&groundBodyDef);

	boxShapeDef.filter.categoryBits = CMASK_EDGES;  //this is the edges/top
	boxShapeDef.filter.maskBits = CMASK_BALL;		//it collides wth balls

													//left
	groundBox.Set(b2Vec2(534/PTM_RATIO, (blit3D->screenHeight-38) / PTM_RATIO), b2Vec2(534/PTM_RATIO, 0));
	edgeEntity->body->CreateFixture(&boxShapeDef);

	//top
	groundBox.Set(b2Vec2(534 / PTM_RATIO, (blit3D->screenHeight-38) / PTM_RATIO),
		b2Vec2(blit3D->screenWidth / PTM_RATIO, (blit3D->screenHeight -38) / PTM_RATIO));
	edgeEntity->body->CreateFixture(&boxShapeDef);

	//right
	groundBox.Set(b2Vec2((blit3D->screenWidth-534) / PTM_RATIO,
		0), b2Vec2((blit3D->screenWidth-534) / PTM_RATIO, (blit3D->screenHeight -38) / PTM_RATIO));
	edgeEntity->body->CreateFixture(&boxShapeDef);

	edgeEntity->body->SetUserData(edgeEntity);
	entityList.push_back(edgeEntity);
	//______MAKE A BALL_______________________
	BallEntity *ball = MakeBall(ballSprite);

	//move the ball to the center bottomof the window
	b2Vec2 pos(blit3D->screenWidth / 2, 35);
	pos = Pixels2Physics(pos);//convert coordinates from pixels to physics world
	ball->body->SetTransform(pos, 0.f);

	//add the ball to our list of (ball) entities
	ballEntityList.push_back(ball);

	// Create contact listener and use it to collect info about collisions
	contactListener = new MyContactListener();
	world->SetContactListener(contactListener);

	//paddle
	cursorX = blit3D->screenWidth / 2;
	paddleEntity = MakePaddle(blit3D->screenWidth / 2, 30, paddleSprite);
	entityList.push_back(paddleEntity);
}

void DeInit(void)
{
	//delete all particles
	for (auto p : particleList) delete p;
	particleList.clear();

	//delete all the entities
	for (auto e : entityList) delete e;
	entityList.clear();

	for (auto b : ballEntityList) delete b;
	ballEntityList.clear();

	for (auto b : brickEntityList) delete b;
	brickEntityList.clear();

	//delete the contact listener
	delete contactListener;

	//Free all physics game data we allocated
	delete world;

	//any sprites/fonts still allocated are freed automatcally by the Blit3D object when we destroy it
}

void Update(double seconds)
{
	switch (gameState)
	{
	case PLAYING:
	{



		//stop it from lagging hard if more than a small amount of time has passed
		if (seconds > 1.0 / 30) elapsedTime += 1.f / 30;
		else elapsedTime += (float)seconds;
		
		//did we finish the level?
		if (brickEntityList.size() == 0)
		{
			level++;
			MakeLevel();
		}

		//move paddle to where the cursor is
		b2Vec2 paddlePos;
		paddlePos.y = 30;
		if (cursorX < 593)
			paddlePos.x = 593;
		else if (cursorX > blit3D->screenWidth - 593)
			paddlePos.x = blit3D->screenWidth - 593;
		else
			paddlePos.x = cursorX;
		paddlePos = Pixels2Physics(paddlePos);
		paddleEntity->body->SetTransform(paddlePos, 0);

		//make sure the balls keep moving at the right speed, and delete any way out of bounds
		for (int i = ballEntityList.size() - 1; i >= 0; --i)
		{
			b2Vec2 dir = ballEntityList[i]->body->GetLinearVelocity();
			dir.Normalize();
			dir *= currentBallSpeed; //scale up the velocity tp correct ball speed
			if (dir.y < 0.01 && dir.y > -0.01)
				dir.y = 0.01;
			ballEntityList[i]->body->SetLinearVelocity(dir); //apply velocity to kinematic object

			b2Vec2 pos = ballEntityList[i]->body->GetPosition();
			if (pos.x * PTM_RATIO < 534*PTM_RATIO)
				pos.x = 524 / PTM_RATIO;
			if (pos.x * PTM_RATIO > blit3D->screenWidth - 534 * PTM_RATIO)
				pos.x = (blit3D->screenWidth - 544) / PTM_RATIO;
			if (pos.y * PTM_RATIO > blit3D->screenHeight - 38*PTM_RATIO)
				pos.x = (blit3D->screenHeight - 48) / PTM_RATIO;
			
			
			if(pos.y < 0)				
			{
				//kill this ball, it is out of bounds
				world->DestroyBody(ballEntityList[i]->body);
				delete ballEntityList[i];
				ballEntityList.erase(ballEntityList.begin() + i);
			}
		}

		//sanity check, should always have at least one ball!
		if (ballEntityList.size() == 0)
		{
			//welp, let's make a ball
			BallEntity *ball = MakeBall(ballSprite);
			ballEntityList.push_back(ball);
			attachedBall = true;
		}

		//if ball is attached to paddle, move ball to where paddle is
		if (attachedBall)
		{
			assert(ballEntityList.size() > 0); //make sure there is at least one ball
			b2Vec2 ballPos = paddleEntity->body->GetPosition();
			ballPos.y = 57 / PTM_RATIO;
			ballEntityList[0]->body->SetTransform(ballPos, 0);
			ballEntityList[0]->body->SetLinearVelocity(b2Vec2(0.f, 0.f));
		}

		//don't apply physics unless at least a timestep worth of time has passed
		while (elapsedTime >= timeStep)
		{
			//update the physics world
			world->Step(timeStep, velocityIterations, positionIterations);

			// Clear applied body forces. 
			world->ClearForces();

			elapsedTime -= timeStep;
			if (slowBall && slowBallTimer > 0)
			{
				slowBallTimer -= timeStep;
				currentBallSpeed = slowBallSpeed;

			}
			else
			{
				slowBall = false;
				currentBallSpeed = DefaultBallSpeed;
			}
			//update game logic/animation
			for (auto e : entityList) e->Update(timeStep);
			for (auto b : ballEntityList) b->Update(timeStep);
			for (auto b : brickEntityList) b->Update(timeStep);

			//update the particle list and remove dead particles
			for (int i = particleList.size() - 1; i >= 0; --i)
			{
				if (particleList[i]->Update(timeStep))
				{
					//time to die!
					delete particleList[i];
					particleList.erase(particleList.begin() + i);
				}
			}

			//loop over contacts
			for (int pos = 0; pos < contactListener->contacts.size(); ++pos)
			{
				MyContact contact = contactListener->contacts[pos];

				//fetch the entities from the body userdata
				Entity *A = (Entity *)contact.fixtureA->GetBody()->GetUserData();
				Entity *B = (Entity *)contact.fixtureB->GetBody()->GetUserData();

				if (A != NULL && B != NULL) //if there is an entity for these objects...
				{


					if (A->typeID == ENTITYBALL)
					{
						//swap A and B
						Entity *C = A;
						A = B;
						B = C;
					}

					if (B->typeID == ENTITYBALL && A->typeID == ENTITYEDGE)
					{
						multiplyer -= 1;
						if (multiplyer <= 0)
							multiplyer = 0;
						//add a particle effect
						Particle *p = new Particle();
						p->spriteList.push_back(boinkSprite1);
						p->spriteList.push_back(boinkSprite2);
						p->spriteList.push_back(boinkSprite2);
						p->spriteList.push_back(boinkSprite2);
						p->spriteList.push_back(boinkSprite3);
						p->spriteList.push_back(boinkSprite3);
						p->spriteList.push_back(boinkSprite3);
						p->spriteList.push_back(boinkSprite3);
						p->rotationSpeed = (float)(rand() % 181) - 90;
						p->angle = rand() % 360;

						p->direction = b2Vec2(0.f, 0.f);
						p->startingSpeed = 0;
						p->targetSpeed = 0;
						p->totalTimeToLive = 0.5f;
						//get coords of contact
						p->coords = Physics2Pixels(contact.contactPoint);
					
						particleList.push_back(p);
					}

					if (B->typeID == ENTITYBALL && A->typeID == ENTITYBRICK)
					{
						multiplyer++;

						score += 5*multiplyer*level;
						BrickEntity *be = (BrickEntity *)A;
						if (be->HandleCollision())
						{
							if (be->power)
							{
								std::uniform_int_distribution<int> randomPow(0, 3);
								b2Vec2 position = be->body->GetPosition();
								position = Physics2Pixels(position);
								PowerUpEntity *p = MakePowerUp(randomPow(rng), position.x, position.y);
								entityList.push_back(p);
							}
							//we need to remove this brick from the world, 
							//but can't do that until all collisions have been processed
							AddToDeadList(be);
						}


						//add a particle effect
						Particle *p = new Particle();
						p->spriteList.push_back(cloudSprite1);
						p->spriteList.push_back(cloudSprite2);
						p->spriteList.push_back(cloudSprite3);
						p->rotationSpeed = (float)(rand() % 181) - 90;
						p->angle = rand() % 360;
						//let's make it 'follow' after the ball
						b2Vec2 dir = B->body->GetLinearVelocity();
						float speed = dir.Length() * PTM_RATIO;
						dir.Normalize();
						p->direction = dir;
						p->startingSpeed = speed / 5;
						p->targetSpeed = speed / 10;
						p->totalTimeToLive = 0.2f;
						//get coords of contact
						p->coords = Physics2Pixels(contact.contactPoint);

						particleList.push_back(p);
					}

					if (B->typeID == ENTITYBALL && A->typeID == ENTITYPADDLE)
					{

							multiplyer = 0;
						PaddleEntity *pe = (PaddleEntity *)A;
						//bend the ball's flight
						pe->HandleCollision(B->body);
					}

					if (B->typeID == ENTITYBALL && A->typeID == ENTITYGROUND)
					{

						multiplyer = 0;
						
						if (ballEntityList.size() > 1)
						{	;
							//remove this ball, but we have others
							AddToDeadList(B);
							score -= 50 + 50 * level;

						}
						else
						{
							//lose a life
							lives--;
							score -= 500 + 200 * level;
							attachedBall = true; //attach the ball for launching
							ballEntityList[0]->body->SetLinearVelocity(b2Vec2(0.f, 0.f));
							if (lives < 1)
							{
								//gameover
								gameState = GAMEOVER;
							}
						}

						//add a particle effect
						Particle *p = new Particle();
						p->spriteList.push_back(explosionSprite4);
						p->spriteList.push_back(explosionSprite1);
						p->spriteList.push_back(explosionSprite1);
						p->spriteList.push_back(explosionSprite2);
						p->spriteList.push_back(explosionSprite3);
						p->spriteList.push_back(explosionSprite4);
						p->spriteList.push_back(explosionSprite5);
						p->spriteList.push_back(explosionSprite6);
						p->spriteList.push_back(explosionSprite7);
						p->spriteList.push_back(cloudSprite3);
						p->spriteList.push_back(cloudSprite2);
						p->spriteList.push_back(cloudSprite1);
						p->spriteList.push_back(cloudSprite1);
						p->spriteList.push_back(cloudSprite2);
						p->spriteList.push_back(cloudSprite3);
						p->rotationSpeed = (float)(rand() % 181) - 90;
						p->angle = rand() % 360;
						p->direction = b2Vec2(0.f, 0.f);;
						p->startingSpeed = 0;
						p->targetSpeed = 0;
						p->totalTimeToLive = 0.5f;
						//get coords of ball
						p->coords = Physics2Pixels(B->body->GetPosition());

						particleList.push_back(p);
					}

					//check and see if paddle or ground hit a powerup
					if (A->typeID == ENTITYPOWERUP)
					{
						//swap A and B
						Entity *C = A;
						A = B;
						B = C;
					}

					if (B->typeID == ENTITYPOWERUP)
					{
						AddToDeadList(B);

						if (A->typeID == ENTITYPADDLE)
						{
							score += 100;
							PowerUpEntity *p = (PowerUpEntity *)B;
							switch (p->powerUpType)
							{
							case PowerUpType::MULTIBALL:
								for (int j = ballEntityList.size() - 1; j >= 0; --j)
								{
									//make 2 new balls
									for (int i = 0; i < 2; ++i)
									{
										//add extra balls if we haven't reached pur limit
										if (ballEntityList.size() < 30)
										{
											BallEntity *b = MakeBall(ballSprite);
											b2Vec2 ballPos = ballEntityList[j]->body->GetPosition();
											ballPos.x += (i * 18 - 9) / PTM_RATIO;
											b->body->SetTransform(ballPos, 0);
											//kick the ball in a random direction upwards			
											b2Vec2 dir = deg2vec(90 + plusMinus70Degrees(rng)); //between 20-160 degrees

											//make the ball move
											dir *= currentBallSpeed; //scale up the velocity
											b->body->SetLinearVelocity(dir); //apply velocity to kinematic object	
																			 //add to the ballist
											ballEntityList.push_back(b);
										}
									}
								}
								break;

							case PowerUpType::BIGPADDLE:
								//break;

							case PowerUpType::EXTRALIFE:
								lives++;
								break;

							case PowerUpType::SLOWBALL:
								slowBall = true;
								currentBallSpeed = slowBallSpeed;
								slowBallTimer = 5;
								break;
							default:
								assert("Unknown PowerUp type" == 0);
								break;
							}//end switch on powerup type
						}
					}
				}//end of checking if they are both NULL userdata
			}//end of collison handling

			 //clean up dead entities
			for (auto e : deadEntityList)
			{
				//remove body from the physics world and free the body data
				world->DestroyBody(e->body);
				//remove the entity from the appropriate entityList
				if (e->typeID == ENTITYBALL)
				{
					for (int i = 0; i < ballEntityList.size(); ++i)
					{
						if (e == ballEntityList[i])
						{
							delete ballEntityList[i];
							ballEntityList.erase(ballEntityList.begin() + i);
							break;
						}
					}
				}
				else if (e->typeID == ENTITYBRICK)
				{
					for (int i = 0; i < brickEntityList.size(); ++i)
					{
						if (e == brickEntityList[i])
						{
							delete brickEntityList[i];
							brickEntityList.erase(brickEntityList.begin() + i);
							break;
						}
					}
				}
				else
				{
					for (int i = 0; i < entityList.size(); ++i)
					{
						if (e == entityList[i])
						{
							delete entityList[i];
							entityList.erase(entityList.begin() + i);
							break;
						}
					}
				}
			}

			deadEntityList.clear();
		}
	}
	break; //end case PLAYING
	
	case GAMEOVER:
		if (brickEntityList.size() != 0)
		{
			for (auto b : brickEntityList) delete b;
			brickEntityList.clear();
		}

	case START:
	default:
		//do nada here
		break;
	}//end switch(gameState)
}

void Draw(void)
{
	glClearColor(0.8f, 0.6f, 0.7f, 0.0f);	//clear colour: r,g,b,a 	
											// wipe the drawing surface clear
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	std::string scoreString = "Score: " + std::to_string(score);
	std::string lifeString = "Lives: " + std::to_string(lives);
	std::string multiString = "Multiplyer: x" + std::to_string(multiplyer);

	switch (gameState)
	{
	case START:
		backgroundSprite->Blit(1920.f / 2, 1080.f / 2);
		logo->Blit(blit3D->screenWidth / 2, blit3D->screenHeight / 2);
		break;
	case EGG:
		backgroundSprite->Blit(1920.f / 2, 1080.f / 2);
		debugFont->BlitText(blit3D->screenWidth/2 - 250, blit3D->screenHeight/2 - 100, "LET'S GET RIGGITY RIGGITY WRECKED SON!!");
		debugFont->BlitText(blit3D->screenWidth-500, 900, "Design/Developper :  ");
		debugFont->BlitText(blit3D->screenWidth - 500, 870, "Frederic Melanson");
		debugFont->BlitText(blit3D->screenWidth - 500, 840, "Code by : ");
		debugFont->BlitText(blit3D->screenWidth - 500, 810, "Frederic Melanson, Darren Reid");
		debugFont->BlitText(blit3D->screenWidth - 500, 750, "Game Assets : Kenney");
		for (auto b : brickEntityList) b->Draw();
		break;
	case PLAYING:
		backgroundSprite->Blit(1920.f / 2, 1080.f / 2);
		//loop over all entities and draw them
		backgroundSprite->Blit(1920.f / 2, 1080.f / 2);
		for (auto e : entityList) e->Draw();
		for (auto b : brickEntityList) b->Draw();
		for (auto b : ballEntityList) b->Draw();
		for (auto p : particleList) p->Draw();
		
		debugFont->BlitText(20, 900, lifeString);
		debugFont->BlitText(20, 870, scoreString);
		debugFont->BlitText(20, 840, multiString);
		break;
	case GAMEOVER:
		backgroundSprite->Blit(1920.f / 2, 1080.f / 2);
		for (auto e : entityList) e->Draw();
		//for (auto b : brickEntityList) b->Draw();
		for (auto b : ballEntityList) b->Draw();
		for (auto p : particleList) p->Draw();
		debugFont->BlitText(20, 30, lifeString);
		gameOverLogo->Blit(blit3D->screenWidth / 2, blit3D->screenHeight / 2);
		break;


	}
}

//the key codes/actions/mods for DoInput are from GLFW: check its documentation for their values
void DoInput(int key, int scancode, int action, int mods)
{
	if(key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		blit3D->Quit(); //start the shutdown sequence

	//suicide button is K
	if (key == GLFW_KEY_K && action == GLFW_PRESS)
	{
		if (gameState == PLAYING||gameState == EGG)
		{
			//kill off the current first ball
			lives--;
			attachedBall = true; //attach the ball for launching

								 //safety check
			//assert(ballEntityList.size() > 0); //make sure there is at least one ball

			ballEntityList[0]->body->SetLinearVelocity(b2Vec2(0.f, 0.f));
			if (lives < 1)
			{
				//gameover
				gameState = GAMEOVER;
			}
		}
		if (gameState == GAMEOVER)
		{
			lives = 400;
			LoadMap("rick.txt", brickEntityList);
			gameState = EGG;
		}
	}
}

void DoCursor(double x, double y)
{
	//scale display size
	cursorX = static_cast<float>(x) * (blit3D->screenWidth / blit3D->trueScreenWidth);
}

void DoMouseButton(int button, int action, int mods)
{
	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
	{
		switch (gameState)
		{
		case START:
			gameState = PLAYING;
			attachedBall = true;
			lives = 3;
			level = 1;
			MakeLevel();
			break;

		case GAMEOVER:
			gameState = START;

		case EGG:

		case PLAYING:
			if (attachedBall)
			{
				attachedBall = false;
				//launch the ball

				//safety check
				assert(ballEntityList.size() >= 1); //make sure there is at least one ball	

				Entity *b = ballEntityList[0];
				//kick the ball in a random direction upwards			
				b2Vec2 dir = deg2vec(90 + plusMinus5Degrees(rng)); //between 85-95 degrees
														//make the ball move
				dir *= currentBallSpeed; //scale up the velocity
				b->body->SetLinearVelocity(dir); //apply velocity to kinematic object				
			}
			break;
		}
	}

}

int main(int argc, char *argv[])
{
	//memory leak detection
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

	blit3D = new Blit3D(Blit3DWindowModel::BORDERLESSFULLSCREEN_1080P, 1920, 1080);

	//set our callback funcs
	blit3D->SetInit(Init);
	blit3D->SetDeInit(DeInit);
	blit3D->SetUpdate(Update);
	blit3D->SetDraw(Draw);
	blit3D->SetDoInput(DoInput);
	blit3D->SetDoCursor(DoCursor);
	blit3D->SetDoMouseButton(DoMouseButton);

	//Run() blocks until the window is closed
	blit3D->Run(Blit3DThreadModel::SINGLETHREADED);
	if (blit3D) delete blit3D;
}