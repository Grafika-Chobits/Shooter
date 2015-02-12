#ifndef COLOR_H
#define COLOR_H

class Color {
	private:
		unsigned char r;
		unsigned char g;
		unsigned char b;

	public:
		Color(unsigned char r, unsigned char g, unsigned char b){
			this->r = r;
			this->g = g;
			this->b = b;
		}

		Color(){
			r = 0;
			g = 0;
			b = 0;
		}

		unsigned char getR(){
			return r;
		}

		unsigned char getG(){
			return g;
		}

		unsigned char getB(){
			return b;
		}

		void setR(unsigned char r){
			this->r = r;
		}

		void setG(unsigned char g){
			this->g = g;
		}

		void setB(unsigned char b){
			this->b = b;
		}

		void setColor(Color color){
			this->r = color.getR();
			this->g = color.getG();
			this->b = color.getB();
		}

		void multiply(Color color1, Color color2){
			this->r = color2.r * color1.r / 255;
			this->g = color2.g * color1.g / 255;
			this->b = color2.b * color1.b / 255;
		}

		//TODO: change to "screen"
		void darken(Color color1, Color color2){
			if(color2.r > color1.r)
				this->r = color1.r;
			else
				this->r = color2.r;

			if(color2.g > color1.g)
				this->g = color1.g;
			else
				this->g = color2.g;

			if(color2.b > color1.b)
				this->b = color1.b;
			else
				this->b = color2.b;
		}

};

#endif
