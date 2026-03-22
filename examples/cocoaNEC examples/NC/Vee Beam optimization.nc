real theta ;

//  Find optimal angle for a 1.5 wavelength Vee beam

model( "Vee Beam" ) 
{
	real height, y, d, dt, dy, dx, dx1, dy1 ;

	height = 12 ;
	d = 1" ;
	voltageFeed( wire( 0, -d, height, 0, d, height, #14, 3 ), 1.0, 0.0 ) ;

	//  0.75 wavelength nominal
	y = 15.97 ;

	dy = y*sind( theta ) ;
	dx = y*cosd( theta ) ;

	wire( 0, -d, height, dx, -dy, height, #14, 13 ) ;
	wire( 0, d, height, dx, dy, height, #14, 13 ) ;
}

control()
{
	real dTheta, previous ;

	theta = 50 ;
	dTheta = 4 ;
	previous = 0 ;

	repeat ( 16 ) {
		runModel() ;
		printf( "theta %.1f  directivity  %.1f dB\n", theta, directivity ) ;
		if ( directivity < previous ) {
			dTheta = -0.5*dTheta ;
		}
		previous = directivity ;
		theta = theta + dTheta ;
	}	
}
