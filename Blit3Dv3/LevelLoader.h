#pragma once
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include "BrickEntity.h"

void LoadMap(std::string fileName, std::vector<BrickEntity *> &brickList)
{
	int posx;
	int posy;
	//clear the current brickList
	for (auto B : brickList) delete B;
	brickList.clear();

	//open file
	std::ifstream myfile;
	myfile.open(fileName);

	if (myfile.is_open())
	{
		//read in # of bricks
		int brickNum = 0;
		myfile >> brickNum;

		//read in each brick
		for (; brickNum > 0; --brickNum)
		{
			//make a brick
			
			int t = 0;
			myfile >> t;
			myfile >> posx;
			myfile >> posy;
			BrickEntity *B = MakeBrick((BrickEnum)t, posx, posy);
			brickList.push_back(B);
		}

		myfile.close();
	}
}