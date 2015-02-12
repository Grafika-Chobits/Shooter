#ifndef COLORING_H
#define COLORING_H

#include <stdio.h>
#include "coordinate.h"
#include "color.h"
#include "frame.h"

class Coloring{
	private:
		Coordinate upleft;
		Coordinate downright;
		
		bool isSingularAtas(Frame * frm, Coordinate posisi)
		{
			int counter0 = 0;
			Color color(0,0,0);
			if(frm->getColor(posisi.getX()-1, 	posisi.getY()-1).isEqual(color))
				counter0++;
			if(frm->getColor(posisi.getX(), 	posisi.getY()-1).isEqual(color))
				counter0++;
			if(frm->getColor(posisi.getX()+1, 	posisi.getY()-1).isEqual(color))
				counter0++;
			return (counter0 == 1);
		}
			
		bool isSingularBawah(Frame * frm, Coordinate posisi)
		{
			int counter0 = 0;
			Color color(0,0,0);
			if(frm->getColor(posisi.getX()-1, 	posisi.getY()+1).isEqual(color))
				counter0++;
			if(frm->getColor(posisi.getX(), 	posisi.getY()+1).isEqual(color))
				counter0++;
			if(frm->getColor(posisi.getX()+1, 	posisi.getY()+1).isEqual(color))
				counter0++;
			
			return (counter0 == 1);
		}
		
		bool isGarisDatar(Frame * frm, Coordinate posisi)
		{
			return frm->getColor(posisi.getX() + 1, posisi.getY()).isEqual(frm->getColor(posisi.getX(), posisi.getY()));
		}
		
		bool isSingular(Frame * frm, Coordinate posisi)
		{
			return (isSingularAtas(frm, posisi) || isSingularBawah(frm, posisi));
		}
			
	public:
		Coloring(Coordinate _upleft, Coordinate _downright)
		{
			upleft 		= 	_upleft;
			downright	=	_downright;
		}
		void drawColor(Frame * frm, Color color) //
		{
			 //frm -> 
		}
};

#endif
