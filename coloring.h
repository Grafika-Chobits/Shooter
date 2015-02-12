#ifndef COLORING_H
#define COLORING_H

#include <stdio.h>
#include "coordinate.h"
#include "color.h"

class Coloring{
	private:
		Coordinate upleft;
		Coordinate downright;
	public:
		Coloring(Coordinate _upleft, Coordinate _downright)
		{
			upleft 		= 	_upleft;
			downright	=	_downright;
		}
		void drawColor(Color color)
		{
			 
		}
};

#endif
