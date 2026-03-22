//	phased array of vertical dipoles

model( "phased vertical dipoles" ) 
{
	real lower, upper, sep, theta, mag ;

	lower = 1.0 ;
	upper = lower + 10.2 ;

	currentFeed( wire( 0, 0, lower, 0, 0, upper, 1", 21 ), 1.0, 0.0 ) ;

	theta = -90 ;
	mag = 1.0 ;
	sep = 5.5 ;

	currentFeed( wire( sep, 0, lower, sep, 0, upper, 1", 21 ), mag*cosd( theta ), mag*sind( theta ) ) ;

	azimuthPlotForElevationAngle( 15.0 ) ;
	//freespace() ;
}
