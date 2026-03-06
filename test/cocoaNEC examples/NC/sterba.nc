//  corner fed Sterba curtain

model( "20m Sterba curtain" )
{
	real s, offset0, offset1, q, z0, z1, feed, y ;
	real r ;

	s = 10.43 ;		//  nominal 1/2 wavelength at 14.080 MHz
	q = s*0.5 ;
	z0 = 5 ;			//  height of lower elements 
	z1 = z0 + s ;		//  height of upper elements ;
	
	y = 0 ;

	offset0 = -0.015 ;
	offset1 =  0.015 ;

	//  feed end
	feed = z0 + 2" ;
	//  feedpoint at corner of curtain
	voltageFeed( wire( offset0, 0, z0, offset0, 0, feed, #12, 3 ), 1, 0 ) ; 	wire( offset0, 0, feed, offset1, 0, z1, #12, 15 ) ;

	// sterba sections

	repeat ( 3 ) {

		wire( offset0, y, z0, offset1, y+q, z0, #12, 21 ) ;
		wire( offset1, y, z1, offset0, y+q, z1, #12, 21 ) ;

		wire( offset1, y+q, z0, offset1, y+q, z1, #12, 21 ) ;
		wire( offset0, y+q, z1, offset0, y+q, z0, #12, 21 ) ;

		y = y + q ;

		wire( offset1, y, z1, offset0, y+q, z1, #12, 21 ) ;
		wire( offset0, y, z0, offset1, y+q, z0, #12, 21 ) ;
	
		y = y + q ;

		offset0 = -offset0 ;
		offset1 = -offset1 ;
	}
	//  far end
	wire( offset0, y, z0, offset1, y, z1, #12, 21 ) ;


	// reflectors sections

	r = -3.4 ;
	y = 0 ;

	wire( offset0+r, 0, z0, offset1+r, 0, z1, #12, 15 ) ;

	repeat ( 3 ) {

		wire( offset0+r, y, z0, offset1+r, y+q, z0, #12, 21 ) ;
		wire( offset1+r, y, z1, offset0+r, y+q, z1, #12, 21 ) ;

		wire( offset1+r, y+q, z0, offset1+r, y+q, z1, #12, 21 ) ;
		wire( offset0+r, y+q, z1, offset0+r, y+q, z0, #12, 21 ) ;

		y = y + q ;

		wire( offset1+r, y, z1, offset0+r, y+q, z1, #12, 21 ) ;
		wire( offset0+r, y, z0, offset1+r, y+q, z0, #12, 21 ) ;
	
		y = y + q ;

		offset0 = -offset0 ;
		offset1 = -offset1 ;
	}
	//  far end
	wire( offset0+r, y, z0, offset1+r, y, z1, #12, 21 ) ;

}
