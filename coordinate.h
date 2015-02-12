#ifndef COORDINATE_H
#define COORDINATE_H

class Coordinate {
	private:
		int x;
		int y;

	public:
		Coordinate(){
			x = 0;
			y = 0;
		}

		Coordinate(int x, int y){
			this->x = x;
			this->y = y;
		}

		int getX(){
			return x;
		}

		int getY(){
			return y;
		}

		void setX(int x){
			this->x = x;
		}

		void setY(int y){
			this->y = y;
		}

		void setCoordinate(int x, int y){
			this->x = x;
			this->y = y;
		}
};

#endif
