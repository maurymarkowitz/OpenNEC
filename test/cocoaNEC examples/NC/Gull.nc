real theta0, theta1, rho ;


//  W7AY Gull
//
//  This is a 1.5 wavelength Vee Beam that has an extra bend on it on each arm of the Vee (which can be
//	made from an extra piece of rope).
//
//	It can also be viewed as a piecewise linear version of one element of a Landstorfer-Sacher beam.
//
//	It has about 0.8 dB better directivity than a Vee beam of the same length, and has about 0.6 dB forward //	gain than the equivalent Vee Beam while having approximately the same 3 dB beamwidth.


model( "Gull" ) 
{
	real height, y, d, dt, dy, dx, dx1, dy1 ;

	height = 12 ;
	d = 1" ;
	voltageFeed( wire( 0, -d, height, 0, d, height, #14, 3 ), 1.0, 0.0 ) ;

	//  0.75 wavelength nominal
	y = 16.03 ;
	dt = rho*y ;
	
	dy = dt*sind( theta0 ) ;
	dx = dt*cosd( theta0 ) ;

	wire( 0, -d, height, dx, -dy, height, #14, 13 ) ;
	wire( 0, d, height, dx, dy, height, #14, 13 ) ;

	dt = (1.0-rho)*y ;
	dy1 = dy + dt*sind( theta1 ) ;
	dx1 = dx + dt*cosd( theta1 ) ;

	wire( dx, -dy, height, dx1, -dy1, height, #14, 13 ) ;
	wire( dx, dy, height, dx1, dy1, height, #14, 13 ) ;
	
	useSommerfeldGround(1) ;
}

control()
{
	theta0 = 42 ;
	theta1 = 81 ;
	rho = 0.57 ;
	runModel() ;
}
